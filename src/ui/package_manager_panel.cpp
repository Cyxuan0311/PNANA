#include "ui/package_manager_panel.h"
#include "ui/icons.h"
#include "ui/package_detail_dialog.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

PackageManagerPanel::PackageManagerPanel(Theme& theme)
    : theme_(theme), visible_(false), search_focused_(false), selected_index_(0), scroll_offset_(0),
      search_filter_(""), detail_dialog_(theme), install_dialog_(theme),
      cached_filter_timestamp_(std::chrono::steady_clock::now() - FILTER_CACHE_TIMEOUT_) {
    // 初始化时选择第一个可用的管理器（使用缓存）
    auto available = getCachedAvailableManagers();
    if (!available.empty()) {
        current_manager_name_ = available[0]->getName();
    }

    main_component_ = Renderer([this] {
        return render();
    });
}

Element PackageManagerPanel::render() {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();

    // 如果安装对话框可见，优先渲染安装对话框
    if (install_dialog_.isVisible()) {
        return install_dialog_.render();
    }

    // 如果详情弹窗可见，只渲染详情弹窗
    if (detail_dialog_.isVisible()) {
        return detail_dialog_.render();
    }

    Elements content;

    content.push_back(renderHeader());
    content.push_back(separator());
    content.push_back(renderTabs());
    content.push_back(separator());
    content.push_back(renderSearchBox());
    content.push_back(separator());
    content.push_back(renderCurrentTab());
    content.push_back(separator());
    content.push_back(renderStatusBar());
    content.push_back(separator());
    content.push_back(renderHelpBar());

    return window(text(" Package Manager ") | color(colors.success) | bold,
                  vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, 90) | size(HEIGHT, GREATER_THAN, 25) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border);
}

Component PackageManagerPanel::getComponent() {
    return main_component_;
}

void PackageManagerPanel::show() {
    visible_ = true;
    search_focused_ = false;
    detail_dialog_.hide();
    install_dialog_.hide();
    selected_index_ = 0;
    scroll_offset_ = 0;
    search_filter_.clear();
    cached_filter_key_.clear(); // 清除过滤缓存

    // 确保有选中的管理器（使用缓存）
    auto available = getCachedAvailableManagers();
    if (!available.empty() && current_manager_name_.empty()) {
        current_manager_name_ = available[0]->getName();
    }

    // 触发包列表加载
    auto manager = getCurrentManager();
    if (manager) {
        manager->getInstalledPackages();
    }
}

void PackageManagerPanel::hide() {
    visible_ = false;
    search_focused_ = false;
    detail_dialog_.hide();
    install_dialog_.hide();
    search_filter_.clear();
}

bool PackageManagerPanel::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    // 如果安装对话框可见，优先处理安装对话框的输入
    if (install_dialog_.isVisible()) {
        return install_dialog_.handleInput(event);
    }

    // 如果详情弹窗可见，优先处理详情弹窗的输入
    if (detail_dialog_.isVisible()) {
        return detail_dialog_.handleInput(event);
    }

    // 如果搜索框获得焦点，先处理搜索输入
    if (search_focused_) {
        if (event == Event::Escape) {
            search_focused_ = false;
            if (search_filter_.empty()) {
                // 如果搜索框为空，按 Escape 关闭面板
                hide();
            } else {
                // 否则只清除搜索
                search_filter_.clear();
                cached_filter_key_.clear(); // 清除过滤缓存
                selected_index_ = 0;
                scroll_offset_ = 0;
            }
            return true;
        } else if (event == Event::Return || event == Event::Tab) {
            // Enter 或 Tab 退出搜索模式
            search_focused_ = false;
            return true;
        } else if (event == Event::Backspace) {
            if (!search_filter_.empty()) {
                search_filter_.pop_back();
                cached_filter_key_.clear(); // 清除过滤缓存
                selected_index_ = 0;
                scroll_offset_ = 0;
            }
            return true;
        } else if (event.is_character()) {
            std::string ch = event.character();
            if (ch.length() == 1 && ch[0] >= 32 && ch[0] < 127) {
                search_filter_ += ch;
                cached_filter_key_.clear(); // 清除过滤缓存
                selected_index_ = 0;
                scroll_offset_ = 0;
            }
            return true;
        }
        return true; // 搜索框获得焦点时，消耗所有输入
    }

    if (event == Event::Escape) {
        hide();
        return true;
    } else if (event == Event::ArrowDown) {
        auto manager = getCurrentManager();
        if (manager) {
            auto filtered = getFilteredPackages(manager);
            if (!filtered.empty()) {
                selected_index_ = (selected_index_ + 1) % filtered.size();
                updateScrollOffset();
            }
        }
        return true;
    } else if (event == Event::ArrowUp) {
        auto manager = getCurrentManager();
        if (manager) {
            auto filtered = getFilteredPackages(manager);
            if (!filtered.empty()) {
                if (selected_index_ == 0) {
                    selected_index_ = filtered.size() - 1;
                } else {
                    selected_index_--;
                }
                updateScrollOffset();
            }
        }
        return true;
    } else if (event == Event::Tab || event == Event::ArrowRight) {
        // 切换到下一个管理器（使用缓存）
        auto available = getCachedAvailableManagers();
        if (!available.empty()) {
            size_t current_idx = 0;
            for (size_t i = 0; i < available.size(); ++i) {
                if (available[i]->getName() == current_manager_name_) {
                    current_idx = i;
                    break;
                }
            }
            current_idx = (current_idx + 1) % available.size();
            switchTab(available[current_idx]->getName());
        }
        return true;
    } else if (event == Event::ArrowLeft) {
        // 切换到上一个管理器（使用缓存）
        auto available = getCachedAvailableManagers();
        if (!available.empty()) {
            size_t current_idx = 0;
            for (size_t i = 0; i < available.size(); ++i) {
                if (available[i]->getName() == current_manager_name_) {
                    current_idx = i;
                    break;
                }
            }
            current_idx = (current_idx == 0) ? available.size() - 1 : current_idx - 1;
            switchTab(available[current_idx]->getName());
        }
        return true;
    } else if (event == Event::Character('/')) {
        // 开始搜索模式
        search_focused_ = true;
        return true;
    } else if (event == Event::CtrlD) {
        // Ctrl+D: 打开安装对话框
        auto manager = getCurrentManager();
        if (manager) {
            install_dialog_.show(manager);
        }
        return true;
    } else if (event == Event::Character('r') || event == Event::F5) {
        // 刷新包列表
        auto manager = getCurrentManager();
        if (manager) {
            manager->refreshPackages();
            manager->getInstalledPackages();
            selected_index_ = 0;
            scroll_offset_ = 0;
        }
        return true;
    } else if (event == Event::Return) {
        // Enter 键：打开选中包的详情弹窗
        auto manager = getCurrentManager();
        if (manager) {
            auto filtered = getFilteredPackages(manager);
            if (!filtered.empty() && selected_index_ < filtered.size()) {
                detail_dialog_.show(filtered[selected_index_], manager);
            }
        }
        return true;
    }

    return false;
}

Element PackageManagerPanel::renderHeader() const {
    auto& colors = theme_.getColors();
    Elements header_elements;

    header_elements.push_back(text(" ") | color(colors.menubar_fg));
    header_elements.push_back(text(icons::PACKAGE) | color(colors.success));
    header_elements.push_back(text(" Package Manager ") | color(colors.menubar_fg) | bold);

    return hbox(std::move(header_elements)) | bgcolor(colors.menubar_bg);
}

Element PackageManagerPanel::renderTabs() const {
    auto& colors = theme_.getColors();

    // 使用缓存的管理器列表，避免每次渲染都查询
    auto available = getCachedAvailableManagers();

    auto makeTab = [&](const std::string& label, const std::string& /*name*/, bool active) {
        if (active) {
            return text("[" + label + "]") | bgcolor(colors.selection) | color(colors.foreground) |
                   bold;
        } else {
            return text(" " + label + " ") | color(colors.menubar_fg);
        }
    };

    Elements elements;
    elements.reserve(available.size() * 2); // 预分配空间
    for (size_t i = 0; i < available.size(); ++i) {
        const auto& manager = available[i];
        bool is_active = (manager->getName() == current_manager_name_);
        elements.push_back(makeTab(manager->getDisplayName(), manager->getName(), is_active));
        if (i < available.size() - 1) {
            elements.push_back(text(" │ ") | color(colors.comment));
        }
    }

    return hbox(std::move(elements)) | center;
}

Element PackageManagerPanel::renderSearchBox() const {
    auto& colors = theme_.getColors();
    Elements search_elements;

    // 搜索图标
    search_elements.push_back(text(icons::SEARCH) | color(colors.comment));
    search_elements.push_back(text(" Search: ") | color(colors.comment));

    // 搜索输入框
    if (search_focused_) {
        // 搜索模式：显示输入内容和光标
        if (search_filter_.empty()) {
            search_elements.push_back(text("_") | color(colors.foreground) |
                                      bgcolor(colors.selection) | bold);
        } else {
            // 显示搜索内容，最后一个是光标
            std::string display = search_filter_ + "_";
            search_elements.push_back(text(display) | color(colors.foreground) |
                                      bgcolor(colors.selection));
        }
    } else {
        // 非搜索模式：显示提示或当前搜索内容
        if (search_filter_.empty()) {
            search_elements.push_back(text("(Press / to search)") | color(colors.comment) | dim);
        } else {
            search_elements.push_back(text(search_filter_) | color(colors.foreground) |
                                      bgcolor(colors.selection));
            search_elements.push_back(text(" (Press / to edit)") | color(colors.comment) | dim);
        }
    }

    return hbox(std::move(search_elements)) | bgcolor(colors.menubar_bg);
}

Element PackageManagerPanel::renderCurrentTab() const {
    auto manager = getCurrentManager();
    if (!manager) {
        auto& colors = theme_.getColors();
        return hbox({text("  "),
                     text("No package manager available") | color(colors.comment) | center}) |
               center;
    }

    Elements tab_content;

    tab_content.push_back(renderPackageList(manager));

    return vbox(std::move(tab_content));
}

Element PackageManagerPanel::renderPackageList(
    std::shared_ptr<features::package_manager::PackageManagerBase> manager) const {
    auto& colors = theme_.getColors();
    Elements list_elements;

    // 获取过滤后的包列表（已优化，使用缓存）
    auto filtered = getFilteredPackages(manager);

    // 如果正在加载
    if (manager->isFetching() && filtered.empty()) {
        list_elements.push_back(
            hbox({text("  "), text("Loading packages...") | color(colors.comment) | center}) |
            center);
        return vbox(std::move(list_elements));
    }

    // 如果有错误
    if (manager->hasError()) {
        list_elements.push_back(hbox({text("  "), text(icons::ERROR) | color(colors.error),
                                      text(" " + manager->getError()) | color(colors.error)}) |
                                center);
        return vbox(std::move(list_elements));
    }

    // 检查是否真的没有包，还是搜索过滤导致没有结果
    if (filtered.empty()) {
        // 获取所有包（不经过搜索过滤）来判断是否真的没有包
        auto all_packages = manager->getInstalledPackages();

        std::string message;
        if (all_packages.empty()) {
            // 真的没有包
            message = "No package now";
        } else if (!search_filter_.empty()) {
            // 有包但搜索过滤后没有结果
            message = "No packages found matching \"" + search_filter_ + "\"";
        } else {
            // 其他情况（理论上不应该发生，但为了安全）
            message = "No package now";
        }

        list_elements.push_back(hbox({text("  "), text(message) | color(colors.comment) | dim}) |
                                center);
        return vbox(std::move(list_elements));
    }

    // 限制显示数量（最多20个）
    const size_t max_display = 20;
    size_t start_idx = scroll_offset_;
    size_t end_idx = std::min(start_idx + max_display, filtered.size());

    // 表头
    Elements header_row = {text("  "), text("Package") | bold | color(colors.keyword), filler(),
                           text("Version") | bold | color(colors.keyword), text("  ")};

    list_elements.push_back(hbox(std::move(header_row)));
    list_elements.push_back(separator());

    // 包列表
    for (size_t i = start_idx; i < end_idx; ++i) {
        bool is_selected = (i == selected_index_);
        list_elements.push_back(renderPackageItem(filtered[i], i, is_selected));
    }

    // 如果还有更多包，显示提示
    if (end_idx < filtered.size()) {
        list_elements.push_back(text(""));
        list_elements.push_back(
            hbox({text("  "),
                  text("... " + std::to_string(filtered.size() - end_idx) + " more packages") |
                      color(colors.comment) | dim}));
    }

    return vbox(std::move(list_elements));
}

Element PackageManagerPanel::renderPackageItem(const features::package_manager::Package& pkg,
                                               size_t /*index*/, bool is_selected) const {
    auto& colors = theme_.getColors();

    Elements item_elements;

    // 选中标记
    if (is_selected) {
        item_elements.push_back(text("  ► ") | color(colors.success) | bold);
    } else {
        item_elements.push_back(text("    "));
    }

    // 包图标（根据管理器类型选择）
    std::string icon = icons::PACKAGE;
    if (current_manager_name_ == "pip") {
        icon = icons::PYTHON;
    } else if (current_manager_name_ == "apt") {
        icon = icons::LINUX;
    } else if (current_manager_name_ == "cargo") {
        icon = icons::PACKAGE;
    } else if (current_manager_name_ == "npm" || current_manager_name_ == "yarn") {
        icon = icons::PACKAGE;
    } else if (current_manager_name_ == "conda") {
        icon = icons::PYTHON;
    } else if (current_manager_name_ == "pacman" || current_manager_name_ == "yum") {
        icon = icons::LINUX;
    } else if (current_manager_name_ == "brew") {
        icon = icons::PACKAGE;
    }
    item_elements.push_back(text(icon) | color(colors.function));
    item_elements.push_back(text(" "));

    // 包名
    std::string name_display = pkg.name;
    if (name_display.length() > 30) {
        name_display = name_display.substr(0, 27) + "...";
    }
    item_elements.push_back(text(name_display) | (is_selected ? color(colors.foreground) | bold
                                                              : color(colors.foreground)));

    item_elements.push_back(filler());

    // 版本
    item_elements.push_back(text(pkg.version) | color(colors.comment));

    // 位置或状态（如果有）
    if (!pkg.location.empty() || !pkg.status.empty()) {
        item_elements.push_back(text("  ") | color(colors.comment));
        std::string info = !pkg.status.empty() ? pkg.status : pkg.location;
        if (info.length() > 25) {
            info = "..." + info.substr(info.length() - 22);
        }
        item_elements.push_back(text(info) | color(colors.comment) | dim);
    }

    item_elements.push_back(text("  "));

    Element item_line = hbox(std::move(item_elements));
    if (is_selected) {
        item_line = item_line | bgcolor(colors.selection);
    }

    return item_line;
}

Element PackageManagerPanel::renderHelpBar() const {
    auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("←→") | color(colors.helpbar_key) | bold, text(": Switch Tab  "),
                 text("/") | color(colors.helpbar_key) | bold, text(": Search  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Details  "),
                 text("Ctrl+D") | color(colors.helpbar_key) | bold, text(": Install  "),
                 text("R/F5") | color(colors.helpbar_key) | bold, text(": Refresh  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Close")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

Element PackageManagerPanel::renderStatusBar() const {
    auto& colors = theme_.getColors();
    Elements status_elements;

    auto manager = getCurrentManager();
    if (manager) {
        auto filtered = getFilteredPackages(manager);
        std::string status_text =
            manager->getDisplayName() + ": " + std::to_string(filtered.size()) + " packages";

        if (manager->isFetching()) {
            status_text += " (Loading...)";
        }

        status_elements.push_back(text("  ") | color(colors.comment));
        status_elements.push_back(text(status_text) | color(colors.comment) | dim);
    } else {
        status_elements.push_back(text("  ") | color(colors.comment));
        status_elements.push_back(text("No package manager selected") | color(colors.comment) |
                                  dim);
    }
    status_elements.push_back(filler());

    return hbox(std::move(status_elements));
}

void PackageManagerPanel::switchTab(const std::string& manager_name) {
    current_manager_name_ = manager_name;
    selected_index_ = 0;
    scroll_offset_ = 0;
    cached_filter_key_.clear(); // 清除过滤缓存
    // 触发包列表加载
    auto manager = getCurrentManager();
    if (manager) {
        manager->getInstalledPackages();
    }
}

void PackageManagerPanel::filterPackages() {
    // 可以在这里实现搜索过滤逻辑
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void PackageManagerPanel::updateScrollOffset() {
    const size_t max_display = 20;
    auto manager = getCurrentManager();
    if (!manager) {
        return;
    }

    auto filtered = getFilteredPackages(manager);

    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + max_display) {
        scroll_offset_ = selected_index_ - max_display + 1;
    }
}

std::vector<features::package_manager::Package> PackageManagerPanel::getFilteredPackages(
    std::shared_ptr<features::package_manager::PackageManagerBase> manager) const {
    if (!manager) {
        return {};
    }

    // 检查过滤缓存
    std::string filter_key = manager->getName() + "|" + search_filter_;
    auto now = std::chrono::steady_clock::now();
    if (filter_key == cached_filter_key_ &&
        (now - cached_filter_timestamp_) < FILTER_CACHE_TIMEOUT_ &&
        !cached_filtered_packages_.empty()) {
        // 缓存命中，直接返回
        return cached_filtered_packages_;
    }

    auto packages = manager->getInstalledPackages();

    // 如果搜索过滤器为空，返回所有包（优化：避免不必要的复制）
    if (search_filter_.empty()) {
        // 更新缓存
        cached_filter_key_ = filter_key;
        cached_filtered_packages_ = packages;
        cached_filter_timestamp_ = now;
        return packages;
    }

    // 过滤包（优化：预分配空间，避免多次重新分配）
    std::vector<features::package_manager::Package> filtered;
    filtered.reserve(packages.size() / 2); // 预分配空间，假设大约一半会匹配

    std::string lower_filter = search_filter_;
    std::transform(lower_filter.begin(), lower_filter.end(), lower_filter.begin(), ::tolower);

    for (const auto& pkg : packages) {
        // 优化：直接使用 find 而不创建临时字符串（如果可能）
        // 但为了大小写不敏感，还是需要转换
        std::string lower_name = pkg.name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);

        if (lower_name.find(lower_filter) != std::string::npos) {
            filtered.push_back(pkg);
        }
    }

    // 更新缓存
    cached_filter_key_ = filter_key;
    cached_filtered_packages_ = filtered;
    cached_filter_timestamp_ = now;

    return filtered;
}

std::shared_ptr<features::package_manager::PackageManagerBase>
PackageManagerPanel::getCurrentManager() const {
    return features::package_manager::PackageManagerRegistry::getInstance().getManager(
        current_manager_name_);
}

std::vector<std::shared_ptr<features::package_manager::PackageManagerBase>>
PackageManagerPanel::getCachedAvailableManagers() const {
    // 直接使用 PackageManagerRegistry 的缓存（30秒），不再在这里缓存
    // Registry 已经实现了高效的缓存机制，避免重复缓存
    return features::package_manager::PackageManagerRegistry::getInstance().getAvailableManagers();
}

void PackageManagerPanel::invalidateManagersCache() {
    // 不再需要，因为直接使用 Registry 的缓存
    // 如果需要强制刷新，可以调用 Registry 的 clearAllCaches()
}

} // namespace ui
} // namespace pnana
