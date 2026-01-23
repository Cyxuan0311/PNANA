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
        // 直接显示所有配置项，按可用性排序（有配置文件的优先）
        std::vector<size_t> sorted_indices;
        for (size_t i = 0; i < tui_configs_.size(); ++i) {
            sorted_indices.push_back(i);
        }

        // 排序：有配置文件的排在前面
        std::sort(sorted_indices.begin(), sorted_indices.end(), [this](size_t a, size_t b) {
            bool a_has_config = !getConfigPathDisplay(tui_configs_[a]).empty() &&
                                getConfigPathDisplay(tui_configs_[a]) != "Not found";
            bool b_has_config = !getConfigPathDisplay(tui_configs_[b]).empty() &&
                                getConfigPathDisplay(tui_configs_[b]) != "Not found";
            if (a_has_config != b_has_config) {
                return a_has_config > b_has_config; // 有配置文件的排前面
            }
            return tui_configs_[a].display_name < tui_configs_[b].display_name; // 按名称排序
        });

        // 显示配置项列表
        for (size_t config_index : sorted_indices) {
            const auto& config = tui_configs_[config_index];
            bool is_selected = (config_index == selected_index_);

            list_elements.push_back(renderConfigItem(config, config_index, is_selected));

            // 如果选中，显示描述
            if (is_selected && !config.description.empty()) {
                list_elements.push_back(
                    hbox({text("    "), text(config.description) | color(colors.comment) | dim}));
            }
        }
    }

    return vbox(list_elements);
}

Element TUIConfigPopup::renderConfigItem(const features::TUIConfig& config, size_t /*config_index*/,
                                         bool is_selected) const {
    const auto& colors = theme_.getColors();

    // 获取工具图标
    std::string tool_icon = getToolIcon(config.name);
    Color tool_icon_color = getToolIconColor(config.category);

    // 配置状态指示器 - 检查是否有可用的配置文件
    std::string status_indicator;
    Color status_color = colors.comment;
    bool has_config =
        !getConfigPathDisplay(config).empty() && getConfigPathDisplay(config) != "Not found";

    if (has_config) {
        status_indicator = "●"; // 实心圆表示有配置文件
        status_color = colors.success;
    } else {
        status_indicator = "○"; // 空心圆表示无配置文件
        status_color = colors.comment;
    }

    // 配置文件路径显示
    std::string path_display = getConfigPathDisplay(config);
    if (path_display == "Not found") {
        path_display = "No config found";
    }

    // 参考命令面板的布局风格
    Elements cmd_elements;
    cmd_elements.push_back(text("  "));

    // 选中标记 - 参考命令面板的样式
    if (is_selected) {
        cmd_elements.push_back(text("► ") | color(colors.success) | bold);
    } else {
        cmd_elements.push_back(text("  "));
    }

    // 状态指示器和工具图标
    cmd_elements.push_back(text(status_indicator) | color(status_color) | bold);
    cmd_elements.push_back(text(" "));
    cmd_elements.push_back(text(tool_icon) | color(tool_icon_color));
    cmd_elements.push_back(text(" "));

    // 工具名称 - 参考命令面板的样式
    cmd_elements.push_back(text(config.display_name) | (is_selected ? color(colors.dialog_fg) | bold
                                                                    : color(colors.foreground)));

    // 配置文件路径 - 右对齐，参考命令面板的描述显示
    if (!path_display.empty()) {
        cmd_elements.push_back(filler());
        cmd_elements.push_back(text(path_display) | color(colors.comment) | dim);
    }

    Element cmd_line = hbox(std::move(cmd_elements));

    // 选中状态样式 - 参考命令面板
    if (is_selected) {
        cmd_line = cmd_line | bgcolor(colors.selection);
    }

    return cmd_line;
}

std::string TUIConfigPopup::getToolIcon(const std::string& tool_name) const {
    static const std::map<std::string, std::string> tool_icons = {
        // 终端模拟器
        {"kitty", pnana::ui::icons::TERMINAL},
        {"ghostty", pnana::ui::icons::TERMINAL},
        {"alacritty", pnana::ui::icons::TERMINAL},
        {"wezterm", pnana::ui::icons::TERMINAL},
        {"foot", pnana::ui::icons::TERMINAL},
        {"konsole", pnana::ui::icons::TERMINAL},
        {"gnome-terminal", pnana::ui::icons::TERMINAL},
        {"xfce4-terminal", pnana::ui::icons::TERMINAL},

        // 编辑器
        {"neovim", pnana::ui::icons::CODE},
        {"vim", pnana::ui::icons::CODE},
        {"helix", pnana::ui::icons::CODE},
        {"kakoune", pnana::ui::icons::CODE},
        {"micro", pnana::ui::icons::CODE},
        {"nano", pnana::ui::icons::CODE},
        {"emacs", pnana::ui::icons::CODE},
        {"vscode", pnana::ui::icons::CODE},

        // 文件管理器
        {"yazi", pnana::ui::icons::FOLDER_OPEN},
        {"lf", pnana::ui::icons::FOLDER_OPEN},
        {"ranger", pnana::ui::icons::FOLDER_OPEN},
        {"nnn", pnana::ui::icons::FOLDER_OPEN},
        {"vifm", pnana::ui::icons::FOLDER_OPEN},
        {"mc", pnana::ui::icons::FOLDER_OPEN},

        // 多路复用器
        {"tmux", pnana::ui::icons::SPLIT},
        {"screen", pnana::ui::icons::SPLIT},
        {"zellij", pnana::ui::icons::SPLIT},

        // Shell
        {"zsh", pnana::ui::icons::SHELL},
        {"bash", pnana::ui::icons::SHELL},
        {"fish", pnana::ui::icons::SHELL},
        {"nushell", pnana::ui::icons::SHELL},

        // 版本控制
        {"git", pnana::ui::icons::GIT_BRANCH},

        // 系统工具
        {"htop", pnana::ui::icons::SETTINGS},
        {"btop", pnana::ui::icons::SETTINGS},
        {"top", pnana::ui::icons::SETTINGS},
        {"iotop", pnana::ui::icons::SETTINGS},

        // 工具
        {"fzf", pnana::ui::icons::SEARCH},
        {"ripgrep", pnana::ui::icons::SEARCH},
        {"fd", pnana::ui::icons::SEARCH},
        {"bat", pnana::ui::icons::FILE_TEXT},
        {"exa", pnana::ui::icons::FILE_TEXT},
        {"delta", pnana::ui::icons::GIT_DIFF}};

    auto it = tool_icons.find(tool_name);
    return it != tool_icons.end() ? it->second : pnana::ui::icons::GEAR;
}

Color TUIConfigPopup::getToolIconColor(const std::string& category) const {
    const auto& colors = theme_.getColors();

    static const std::map<std::string, Color> category_colors = {
        {"terminal", Color::RGB(138, 173, 244)},        // 蓝色 - 终端
        {"editor", Color::RGB(166, 227, 161)},          // 绿色 - 编辑器
        {"file_manager", Color::RGB(245, 194, 231)},    // 粉色 - 文件管理器
        {"multiplexer", Color::RGB(250, 179, 135)},     // 橙色 - 多路复用器
        {"shell", Color::RGB(203, 166, 247)},           // 紫色 - Shell
        {"version_control", Color::RGB(249, 226, 175)}, // 黄色 - 版本控制
        {"system", Color::RGB(137, 220, 235)},          // 青色 - 系统工具
        {"utility", Color::RGB(186, 194, 222)},         // 灰色 - 工具
        {"other", colors.comment}                       // 默认颜色
    };

    auto it = category_colors.find(category);
    return it != category_colors.end() ? it->second : colors.comment;
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
