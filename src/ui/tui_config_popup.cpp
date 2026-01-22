#include "ui/tui_config_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <filesystem>
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

TUIConfigPopup::TUIConfigPopup(Theme& theme) : theme_(theme), is_open_(false), selected_index_(0) {}

void TUIConfigPopup::setData(bool is_open, const std::vector<features::TUIConfig>& tui_configs,
                             size_t selected_index) {
    is_open_ = is_open;
    tui_configs_ = tui_configs;
    selected_index_ = selected_index;
}

ftxui::Element TUIConfigPopup::render() {
    if (!is_open_) {
        return text("");
    }

    const auto& colors = theme_.getColors();
    Elements dialog_content;

    // 标题
    dialog_content.push_back(renderTitle());

    dialog_content.push_back(separator());

    // 配置文件列表
    dialog_content.push_back(renderConfigList());

    dialog_content.push_back(separator());

    // 帮助栏
    dialog_content.push_back(renderHelpBar());

    int height =
        std::min(25, int(12 + static_cast<int>(std::min(tui_configs_.size(), size_t(15)))));

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 90) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

bool TUIConfigPopup::handleInput(ftxui::Event event) {
    if (!is_open_) {
        return false;
    }

    if (event == ftxui::Event::Escape) {
        close();
        return true;
    } else if (event == ftxui::Event::Return) {
        if (config_open_callback_ && selected_index_ < tui_configs_.size()) {
            config_open_callback_(tui_configs_[selected_index_]);
        }
        close();
        return true;
    } else if (event == ftxui::Event::ArrowDown) {
        if (!tui_configs_.empty()) {
            selected_index_ = (selected_index_ + 1) % tui_configs_.size();
        }
        return true;
    } else if (event == ftxui::Event::ArrowUp) {
        if (!tui_configs_.empty()) {
            if (selected_index_ == 0) {
                selected_index_ = tui_configs_.size() - 1;
            } else {
                selected_index_--;
            }
        }
        return true;
    }

    return false;
}

void TUIConfigPopup::open() {
    is_open_ = true;
    selected_index_ = 0;
}

void TUIConfigPopup::close() {
    is_open_ = false;
    selected_index_ = 0;
}

void TUIConfigPopup::setConfigOpenCallback(
    std::function<void(const features::TUIConfig&)> callback) {
    config_open_callback_ = callback;
}

Element TUIConfigPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::FILE_TEXT) | color(colors.success),
                 text(" TUI Configuration Files "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element TUIConfigPopup::renderConfigList() const {
    const auto& colors = theme_.getColors();
    Elements list_elements;

    if (tui_configs_.empty()) {
        list_elements.push_back(hbox(
            {text("  "), text("No TUI configuration files found") | color(colors.comment) | dim}));
    } else {
        // 按类别分组显示
        std::map<std::string, std::vector<size_t>> category_groups;

        for (size_t i = 0; i < tui_configs_.size(); ++i) {
            const auto& config = tui_configs_[i];
            category_groups[config.category.empty() ? "other" : config.category].push_back(i);
        }

        // 定义类别显示顺序
        std::vector<std::string> category_order = {"terminal",    "editor",  "file_manager",
                                                   "multiplexer", "shell",   "version_control",
                                                   "system",      "utility", "other"};

        for (const auto& category : category_order) {
            auto it = category_groups.find(category);
            if (it == category_groups.end())
                continue;

            // 添加类别标题
            std::string category_title = getCategoryDisplayName(category);
            if (!category_title.empty()) {
                list_elements.push_back(text(""));
                list_elements.push_back(
                    hbox({text("  "), text(category_title) | color(colors.success) | bold}));
                list_elements.push_back(separator());
            }

            // 添加该类别的配置项
            for (size_t config_index : it->second) {
                const auto& config = tui_configs_[config_index];
                bool is_selected = (config_index == selected_index_);

                Elements config_elements;
                config_elements.push_back(text("  "));

                // 选中标记
                if (is_selected) {
                    config_elements.push_back(text("► ") | color(colors.success) | bold);
                } else {
                    config_elements.push_back(text("  "));
                }

                // 工具名称
                config_elements.push_back(
                    text(config.display_name) |
                    (is_selected ? color(colors.dialog_fg) | bold : color(colors.foreground)));

                // 配置文件路径
                std::string path_display = getConfigPathDisplay(config);
                config_elements.push_back(filler());
                config_elements.push_back(text(path_display) | color(colors.comment) | dim);

                Element config_line = hbox(config_elements);
                if (is_selected) {
                    config_line = config_line | bgcolor(colors.selection);
                }

                list_elements.push_back(config_line);

                // 如果选中，显示描述
                if (is_selected && !config.description.empty()) {
                    list_elements.push_back(hbox(
                        {text("    "), text(config.description) | color(colors.comment) | dim}));
                }
            }
        }
    }

    return vbox(list_elements);
}

Element TUIConfigPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  "), text("↑↓") | color(colors.helpbar_key) | bold, text(": Navigate  "),
                 text("Enter") | color(colors.helpbar_key) | bold, text(": Open  "),
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel")}) |
           bgcolor(colors.helpbar_bg) | color(colors.helpbar_fg) | dim;
}

std::string TUIConfigPopup::getConfigPathDisplay(const features::TUIConfig& config) const {
    // 显示第一个存在的配置文件路径，并简化显示
    for (const auto& path : config.config_paths) {
        try {
            std::filesystem::path config_path = expandPath(path);
            if (std::filesystem::exists(config_path)) {
                std::string path_str = config_path.string();

                // 简化家目录显示
                const char* home = std::getenv("HOME");
                if (home && path_str.find(home) == 0) {
                    path_str = "~" + path_str.substr(std::string(home).size());
                }

                return path_str;
            }
        } catch (...) {
            continue;
        }
    }
    return "Not found";
}

std::string TUIConfigPopup::getCategoryDisplayName(const std::string& category) const {
    static const std::map<std::string, std::string> category_names = {
        {"terminal", "Terminal Emulators"},
        {"editor", "Text Editors"},
        {"file_manager", "File Managers"},
        {"multiplexer", "Terminal Multiplexers"},
        {"shell", "Shells"},
        {"version_control", "Version Control"},
        {"system", "System Tools"},
        {"utility", "Utilities"},
        {"other", "Other Tools"}};

    auto it = category_names.find(category);
    return it != category_names.end() ? it->second : "";
}

std::filesystem::path TUIConfigPopup::expandPath(const std::string& path) const {
    std::string expanded = path;

    // 展开家目录
    if (expanded.find("~") == 0) {
        const char* home = std::getenv("HOME");
        if (home) {
            expanded = std::string(home) + expanded.substr(1);
        }
    }

    // 展开环境变量
    size_t start = 0;
    while ((start = expanded.find("$", start)) != std::string::npos) {
        size_t end = start + 1;
        while (end < expanded.size() && (std::isalnum(expanded[end]) || expanded[end] == '_')) {
            ++end;
        }

        if (end > start + 1) {
            std::string var_name = expanded.substr(start + 1, end - start - 1);
            const char* var_value = std::getenv(var_name.c_str());
            if (var_value) {
                expanded.replace(start, end - start, var_value);
                start += std::string(var_value).size();
            } else {
                start = end;
            }
        } else {
            ++start;
        }
    }

    return std::filesystem::path(expanded);
}

} // namespace ui
} // namespace pnana
