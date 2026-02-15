#include "ui/plugin_manager_dialog.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include <algorithm>

using namespace ftxui;

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

namespace pnana {
namespace ui {

PluginManagerDialog::PluginManagerDialog(Theme& theme, plugins::PluginManager* plugin_manager)
    : theme_(theme), plugin_manager_(plugin_manager), visible_(false), selected_index_(0) {
    // 确保 visible_ 被正确初始化为 false
    visible_ = false;
}

void PluginManagerDialog::open() {
    visible_ = true;
    selected_index_ = 0;
    refreshPlugins();
}

void PluginManagerDialog::close() {
    visible_ = false;
}

void PluginManagerDialog::refreshPlugins() {
    if (plugin_manager_) {
        plugins_ = plugin_manager_->getAllPlugins();
    } else {
        plugins_.clear();
    }

    // 确保选中索引有效
    if (selected_index_ >= plugins_.size() && !plugins_.empty()) {
        selected_index_ = plugins_.size() - 1;
    }
}

void PluginManagerDialog::setPluginManager(plugins::PluginManager* plugin_manager) {
    plugin_manager_ = plugin_manager;
    refreshPlugins();
    // 确保设置插件管理器后不会自动打开对话框
    visible_ = false;
}

bool PluginManagerDialog::handleInput(ftxui::Event event) {
    if (!visible_ || !plugin_manager_)
        return false;

    if (event == Event::Escape) {
        close();
        return true;
    }

    if (event == Event::Return) {
        if (selected_index_ < plugins_.size()) {
            togglePlugin(selected_index_);
        }
        return true;
    }

    if (event == Event::ArrowUp) {
        selectPrevious();
        return true;
    }

    if (event == Event::ArrowDown) {
        selectNext();
        return true;
    }

    // 支持空格键切换
    if (event.is_character() && event.character() == " ") {
        if (selected_index_ < plugins_.size()) {
            togglePlugin(selected_index_);
        }
        return true;
    }

    return false;
}

Element PluginManagerDialog::render() {
    if (!visible_) {
        // 返回空文本元素，确保不会渲染任何内容
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements content;

    // 标题
    content.push_back(hbox({text(" Plugin Manager ") | bold | color(colors.foreground), filler(),
                            text("Alt+P") | color(colors.comment) | dim}) |
                      bgcolor(colors.menubar_bg));
    content.push_back(text(""));

    // 插件列表
    if (plugins_.empty()) {
        content.push_back(text("  No plugins found") | color(colors.comment) | center);
    } else {
        content.push_back(renderPluginList());
    }

    content.push_back(text(""));
    content.push_back(separator());

    // 提示信息
    content.push_back(
        hbox({text("  "), text("↑↓") | color(colors.function) | bold, text(": Navigate  "),
              text("Space/Enter") | color(colors.function) | bold, text(": Toggle  "),
              text("Esc") | color(colors.function) | bold, text(": Close"), filler()}) |
        bgcolor(colors.menubar_bg) | dim);

    return window(text(""), vbox(content)) | size(WIDTH, EQUAL, 80) | size(HEIGHT, EQUAL, 25) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

Element PluginManagerDialog::renderPluginList() {
    Elements items;

    for (size_t i = 0; i < plugins_.size(); ++i) {
        items.push_back(renderPluginItem(plugins_[i], i, i == selected_index_));
    }

    return vbox(items);
}

Element PluginManagerDialog::renderPluginItem(const plugins::PluginInfo& plugin, size_t /* index */,
                                              bool is_selected) {
    auto& colors = theme_.getColors();

    // 状态指示器
    std::string status = plugin.loaded ? "[ON]" : "[OFF]";
    Color status_color = plugin.loaded ? colors.success : colors.comment;

    // 插件名称和版本
    std::string name_version = plugin.name;
    if (!plugin.version.empty()) {
        name_version += " v" + plugin.version;
    }

    // 描述信息
    std::string desc = plugin.description.empty() ? "No description" : plugin.description;
    if (desc.length() > 50) {
        desc = desc.substr(0, 47) + "...";
    }

    // 作者信息
    std::string author_info = plugin.author.empty() ? "" : " by " + plugin.author;

    Elements item_content;

    // 第一行：名称、版本、状态
    item_content.push_back(hbox(
        {text("  "), (is_selected ? text("► ") | color(colors.function) : text("  ")),
         text(name_version) | color(is_selected ? colors.foreground : colors.comment) | bold,
         text(" ") | color(status_color), text(status) | bold | color(status_color), filler()}));

    // 第二行：描述
    item_content.push_back(hbox({text("    "), text(desc) | color(colors.comment) | dim,
                                 text(author_info) | color(colors.comment) | dim, filler()}));

    return vbox(item_content) |
           (is_selected ? bgcolor(colors.selection) : bgcolor(colors.background));
}

void PluginManagerDialog::selectNext() {
    if (plugins_.empty())
        return;

    if (selected_index_ < plugins_.size() - 1) {
        selected_index_++;
    } else {
        selected_index_ = 0;
    }
}

void PluginManagerDialog::selectPrevious() {
    if (plugins_.empty())
        return;

    if (selected_index_ > 0) {
        selected_index_--;
    } else {
        selected_index_ = plugins_.size() - 1;
    }
}

void PluginManagerDialog::togglePlugin(size_t index) {
    if (index >= plugins_.size() || !plugin_manager_)
        return;

    const auto& plugin = plugins_[index];

    if (plugin.loaded) {
        // 禁用插件
        if (plugin_manager_->disablePlugin(plugin.name)) {
            refreshPlugins();
        }
    } else {
        // 启用插件
        if (plugin_manager_->enablePlugin(plugin.name)) {
            refreshPlugins();
        }
    }
}

void PluginManagerDialog::apply() {
    close();
}

} // namespace ui
} // namespace pnana
