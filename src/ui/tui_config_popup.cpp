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

TUIConfigPopup::TUIConfigPopup(Theme& theme)
    : theme_(theme), is_open_(false), original_configs_(), filtered_configs_(), selected_index_(0),
      scroll_offset_(0), input_("") {}

void TUIConfigPopup::setData(bool is_open, const std::vector<features::TUIConfig>& tui_configs,
                             size_t selected_index) {
    is_open_ = is_open;
    original_configs_ = tui_configs;
    updateFilteredConfigs();
    selected_index_ = selected_index;
    if (selected_index_ >= filtered_configs_.size()) {
        selected_index_ = 0;
    }
    scroll_offset_ = 0;
    adjustScrollOffset();
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

    // 搜索输入框
    dialog_content.push_back(text(""));
    dialog_content.push_back(renderInputBox());

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 配置文件列表
    dialog_content.push_back(renderConfigList());

    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());

    // 帮助栏
    dialog_content.push_back(renderHelpBar());

    int height =
        std::min(22, int(15 + static_cast<int>(std::min(filtered_configs_.size(), size_t(12)))));

    return window(text(""), vbox(dialog_content)) | size(WIDTH, EQUAL, 85) |
           size(HEIGHT, EQUAL, height) | bgcolor(colors.dialog_bg) |
           borderWithColor(colors.dialog_border);
}

bool TUIConfigPopup::handleInput(ftxui::Event event) {
    if (!is_open_) {
        return false;
    }

    if (event == ftxui::Event::Escape) {
        if (!input_.empty()) {
            // 如果有搜索内容，先清除搜索
            setInput("");
        } else {
            close();
        }
        return true;
    } else if (event == ftxui::Event::Return) {
        if (config_open_callback_ && selected_index_ < filtered_configs_.size()) {
            config_open_callback_(filtered_configs_[selected_index_]);
        }
        close();
        return true;
    } else if (event == ftxui::Event::Backspace) {
        if (!input_.empty()) {
            setInput(input_.substr(0, input_.size() - 1));
        }
        return true;
    } else if (event == ftxui::Event::ArrowDown) {
        if (!filtered_configs_.empty()) {
            if (selected_index_ + 1 < filtered_configs_.size()) {
                selected_index_++;
                adjustScrollOffset();
            }
        }
        return true;
    } else if (event == ftxui::Event::ArrowUp) {
        if (!filtered_configs_.empty()) {
            if (selected_index_ > 0) {
                selected_index_--;
                adjustScrollOffset();
            }
        }
        return true;
    } else if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1 && ch[0] >= 32 && ch[0] < 127) {
            setInput(input_ + ch);
        }
        return true;
    }

    return false;
}

void TUIConfigPopup::open() {
    is_open_ = true;
    input_ = "";
    selected_index_ = 0;
    scroll_offset_ = 0;
    updateFilteredConfigs();
}

void TUIConfigPopup::close() {
    is_open_ = false;
    input_ = "";
    selected_index_ = 0;
    scroll_offset_ = 0;
}

void TUIConfigPopup::setConfigOpenCallback(
    std::function<void(const features::TUIConfig&)> callback) {
    config_open_callback_ = callback;
}

void TUIConfigPopup::updateFilteredConfigs() {
    if (input_.empty()) {
        filtered_configs_ = original_configs_;
    } else {
        filtered_configs_.clear();
        std::string lower_input = input_;
        std::transform(lower_input.begin(), lower_input.end(), lower_input.begin(), ::tolower);

        for (const auto& config : original_configs_) {
            std::string lower_name = config.name;
            std::string lower_display = config.display_name;
            std::string lower_desc = config.description;
            std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
            std::transform(lower_display.begin(), lower_display.end(), lower_display.begin(),
                           ::tolower);
            std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);

            if (lower_name.find(lower_input) != std::string::npos ||
                lower_display.find(lower_input) != std::string::npos ||
                lower_desc.find(lower_input) != std::string::npos) {
                filtered_configs_.push_back(config);
            }
        }
    }
}

void TUIConfigPopup::setInput(const std::string& input) {
    input_ = input;
    updateFilteredConfigs();
    selected_index_ = 0;
    scroll_offset_ = 0;
    adjustScrollOffset();
}

void TUIConfigPopup::adjustScrollOffset() {
    if (filtered_configs_.empty()) {
        scroll_offset_ = 0;
        return;
    }

    // 确保选中的项目可见
    size_t max_visible = 12; // 最大可见项目数
    if (selected_index_ < scroll_offset_) {
        scroll_offset_ = selected_index_;
    } else if (selected_index_ >= scroll_offset_ + max_visible) {
        scroll_offset_ = selected_index_ - max_visible + 1;
    }

    // 确保滚动偏移不会超出范围
    if (scroll_offset_ > filtered_configs_.size() - max_visible) {
        scroll_offset_ = std::max(size_t(0), filtered_configs_.size() - max_visible);
    }
}

Element TUIConfigPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(pnana::ui::icons::FILE_TEXT) | color(colors.success),
                 text(" TUI Configuration Files "), text(" ")}) |
           bold | bgcolor(colors.dialog_title_bg) | color(colors.dialog_title_fg) | center;
}

Element TUIConfigPopup::renderInputBox() const {
    const auto& colors = theme_.getColors();
    std::string input_display = input_.empty() ? "_" : input_ + "_";
    return hbox({text("  > "),
                 text(input_display) | bold | color(colors.dialog_fg) | bgcolor(colors.selection)});
}

Element TUIConfigPopup::renderConfigList() const {
    const auto& colors = theme_.getColors();
    Elements list_elements;

    if (filtered_configs_.empty()) {
        std::string no_result_text = input_.empty() ? "No TUI configuration files found"
                                                    : "No matching configurations found";
        list_elements.push_back(
            hbox({text("  "), text(no_result_text) | color(colors.comment) | dim}));
    } else {
        // 显示过滤后的配置项列表（最多12个）
        size_t max_display = std::min(filtered_configs_.size(), size_t(12));

        for (size_t i = 0; i < max_display && (scroll_offset_ + i) < filtered_configs_.size();
             ++i) {
            size_t config_index = scroll_offset_ + i;
            const auto& config = filtered_configs_[config_index];
            bool is_selected = (config_index == selected_index_);

            list_elements.push_back(renderConfigItem(config, config_index, is_selected));

            // 如果选中，显示描述
            if (is_selected && !config.description.empty()) {
                list_elements.push_back(
                    hbox({text("    "), text(config.description) | color(colors.comment) | dim}));
            }
        }

        // 如果还有更多配置，显示提示
        if (filtered_configs_.size() > max_display) {
            list_elements.push_back(text(""));
            std::string more_text = "... " +
                                    std::to_string(filtered_configs_.size() - max_display) +
                                    " more configurations";
            list_elements.push_back(
                hbox({text("  "), text(more_text) | color(colors.comment) | dim}));
        }
    }

    return vbox(list_elements);
}

Element TUIConfigPopup::renderConfigItem(const features::TUIConfig& config, size_t config_index,
                                         bool is_selected) const {
    const auto& colors = theme_.getColors();
    (void)config_index; // avoid unused parameter warning; kept for future use

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

    // 类别标签
    std::string category_display = getCategoryDisplayName(config.category);
    if (!category_display.empty()) {
        cmd_elements.push_back(filler());
        cmd_elements.push_back(text("[" + category_display + "]") | color(colors.comment) | dim);
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
        {"delta", pnana::ui::icons::GIT_DIFF},

        // 新增的工具图标映射
        // 编辑器
        {"subl", pnana::ui::icons::CODE},
        {"atom", pnana::ui::icons::CODE},
        {"code-oss", pnana::ui::icons::CODE},
        {"vscodium", pnana::ui::icons::CODE},
        {"lite-xl", pnana::ui::icons::CODE},
        {"lapce", pnana::ui::icons::CODE},
        {"zed", pnana::ui::icons::CODE},
        {"vis", pnana::ui::icons::CODE},
        {"amp", pnana::ui::icons::CODE},
        {"ne", pnana::ui::icons::CODE},
        {"jed", pnana::ui::icons::CODE},
        {"joe", pnana::ui::icons::CODE},
        {"mg", pnana::ui::icons::CODE},
        {"le", pnana::ui::icons::CODE},

        // 文件管理器
        {"dolphin", pnana::ui::icons::FOLDER_OPEN},
        {"thunar", pnana::ui::icons::FOLDER_OPEN},
        {"pcmanfm", pnana::ui::icons::FOLDER_OPEN},
        {"nemo", pnana::ui::icons::FOLDER_OPEN},
        {"caja", pnana::ui::icons::FOLDER_OPEN},
        {"nautilus", pnana::ui::icons::FOLDER_OPEN},
        {"doublecmd", pnana::ui::icons::FOLDER_OPEN},
        {"fff", pnana::ui::icons::FOLDER_OPEN},
        {"clifm", pnana::ui::icons::FOLDER_OPEN},
        {"cfm", pnana::ui::icons::FOLDER_OPEN},
        {"noice", pnana::ui::icons::FOLDER_OPEN},

        // 版本控制
        {"mercurial", pnana::ui::icons::GIT_BRANCH},
        {"svn", pnana::ui::icons::GIT_BRANCH},
        {"fossil", pnana::ui::icons::GIT_BRANCH},
        {"pijul", pnana::ui::icons::GIT_BRANCH},
        {"darcs", pnana::ui::icons::GIT_BRANCH},

        // 系统工具
        {"iftop", pnana::ui::icons::SETTINGS},
        {"nload", pnana::ui::icons::SETTINGS},
        {"powertop", pnana::ui::icons::SETTINGS},
        {"nvtop", pnana::ui::icons::SETTINGS},
        {"s-tui", pnana::ui::icons::SETTINGS},
        {"radeontop", pnana::ui::icons::SETTINGS},
        {"atop", pnana::ui::icons::SETTINGS},
        {"slurm", pnana::ui::icons::SETTINGS},
        {"conky", pnana::ui::icons::SETTINGS},
        {"dstat", pnana::ui::icons::SETTINGS},
        {"collectl", pnana::ui::icons::SETTINGS},

        // 开发工具
        {"docker", pnana::ui::icons::PACKAGE},
        {"kubectl", pnana::ui::icons::PACKAGE},
        {"helm", pnana::ui::icons::PACKAGE},
        {"terraform", pnana::ui::icons::PACKAGE},
        {"ansible", pnana::ui::icons::PACKAGE},

        // 文本处理工具
        {"jq", pnana::ui::icons::FILE_TEXT},
        {"yq", pnana::ui::icons::FILE_TEXT},
        {"pandoc", pnana::ui::icons::FILE_TEXT},
        {"pandoc-citeproc", pnana::ui::icons::FILE_TEXT},

        // 网络工具
        {"curl", pnana::ui::icons::GLOBE},
        {"wget", pnana::ui::icons::GLOBE},
        {"httpie", pnana::ui::icons::GLOBE},
        {"aria2", pnana::ui::icons::GLOBE},

        // 数据库客户端
        {"sqlite3", pnana::ui::icons::DATABASE},
        {"mysql", pnana::ui::icons::DATABASE},
        {"psql", pnana::ui::icons::DATABASE},
        {"mongosh", pnana::ui::icons::DATABASE},
        {"redis-cli", pnana::ui::icons::DATABASE},

        // 编程语言包管理器
        {"pip", pnana::ui::icons::PACKAGE},
        {"npm", pnana::ui::icons::PACKAGE},
        {"yarn", pnana::ui::icons::PACKAGE},
        {"pnpm", pnana::ui::icons::PACKAGE},
        {"cargo", pnana::ui::icons::PACKAGE},
        {"go", pnana::ui::icons::PACKAGE},
        {"rustup", pnana::ui::icons::PACKAGE},

        // 更多实用工具
        {"tmuxp", pnana::ui::icons::SPLIT},
        {"byobu", pnana::ui::icons::SPLIT},
        {"abduco", pnana::ui::icons::SPLIT},
        {"dtach", pnana::ui::icons::TERMINAL},
        {"tig", pnana::ui::icons::GIT_BRANCH},
        {"gitui", pnana::ui::icons::GIT_BRANCH},
        {"lazygit", pnana::ui::icons::GIT_BRANCH},
        {"gh", pnana::ui::icons::GIT_BRANCH},
        {"glab", pnana::ui::icons::GIT_BRANCH},
        {"task", pnana::ui::icons::CHECKLIST},
        {"timewarrior", pnana::ui::icons::CLOCK},
        {"khal", pnana::ui::icons::CALENDAR},
        {"vdirsyncer", pnana::ui::icons::CALENDAR},
        {"newsboat", pnana::ui::icons::RSS},
        {"neomutt", pnana::ui::icons::MAIL},
        {"mutt", pnana::ui::icons::MAIL},
        {"alpine", pnana::ui::icons::MAIL},
        {"lynx", pnana::ui::icons::GLOBE},
        {"w3m", pnana::ui::icons::GLOBE},
        {"links", pnana::ui::icons::GLOBE},
        {"elinks", pnana::ui::icons::GLOBE},

        // 更多终端模拟器
        {"terminator", pnana::ui::icons::TERMINAL},
        {"tilix", pnana::ui::icons::TERMINAL},
        {"cool-retro-term", pnana::ui::icons::TERMINAL},
        {"hyper", pnana::ui::icons::TERMINAL},
        {"tabby", pnana::ui::icons::TERMINAL},
        {"st", pnana::ui::icons::TERMINAL},
        {"rxvt", pnana::ui::icons::TERMINAL},

        // 更多Shell
        {"dash", pnana::ui::icons::SHELL},
        {"ash", pnana::ui::icons::SHELL},
        {"ksh", pnana::ui::icons::SHELL},
        {"tcsh", pnana::ui::icons::SHELL},
        {"ion", pnana::ui::icons::SHELL},
        {"murex", pnana::ui::icons::SHELL},
        {"oil", pnana::ui::icons::SHELL},
        {"xonsh", pnana::ui::icons::SHELL},
        {"elvish", pnana::ui::icons::SHELL},

        // 更多多路复用器
        {"tmate", pnana::ui::icons::SPLIT},
        {"dvtm", pnana::ui::icons::SPLIT}};

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
                 text("Esc") | color(colors.helpbar_key) | bold, text(": Cancel  "),
                 text("Type") | color(colors.helpbar_key) | bold, text(": Filter")}) |
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
