#include "core/config_manager.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace pnana {
namespace core {

ConfigManager::ConfigManager() : loaded_(false) {
    config_path_ = getUserConfigPath();
    resetToDefaults();
}

ConfigManager::~ConfigManager() = default;

std::string ConfigManager::getDefaultConfigPath() {
    // 返回默认配置文件路径（在项目目录中）
    return "config/default_config.json";
}

std::string ConfigManager::getUserConfigPath() {
    // 获取用户配置目录
    const char* home = std::getenv("HOME");
    if (home) {
        std::string config_dir = std::string(home) + "/.config/pnana";
        // 确保目录存在
        fs::create_directories(config_dir);
        return config_dir + "/config.json";
    }
    // 如果无法获取 HOME，使用当前目录
    return "config.json";
}

// 展开路径中的 ~ 符号
static std::string expandPath(const std::string& path) {
    if (path.empty()) {
        return path;
    }

    // 如果路径以 ~ 开头，展开为用户主目录
    if (path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) {
            if (path.length() == 1 || path[1] == '/') {
                // ~ 或 ~/...
                return std::string(home) + (path.length() > 1 ? path.substr(1) : "");
            }
        }
    }

    return path;
}

void ConfigManager::resetToDefaults() {
    config_ = AppConfig();
    loaded_ = false;
}

bool ConfigManager::loadConfig(const std::string& config_path) {
    if (!config_path.empty()) {
        // 展开路径中的 ~ 符号
        config_path_ = expandPath(config_path);
    }

    // 如果指定的配置文件不存在，尝试从默认配置加载并创建新文件
    if (!fs::exists(config_path_)) {
        std::string default_path = getDefaultConfigPath();
        bool loaded_from_default = false;

        // 尝试从默认配置文件加载
        if (fs::exists(default_path)) {
            std::ifstream default_file(default_path);
            if (default_file.is_open()) {
                std::stringstream buffer;
                buffer << default_file.rdbuf();
                std::string content = buffer.str();
                default_file.close();

                if (parseJSON(content)) {
                    loaded_from_default = true;
                }
            }
        }

        // 如果从默认配置加载失败，使用内置默认值
        if (!loaded_from_default) {
            resetToDefaults();
        }

        // 保存配置到用户指定的路径（创建新文件）
        if (saveConfig(config_path_)) {
            loaded_ = true;
            return true;
        } else {
            // 如果保存失败，至少使用默认配置
            if (!loaded_from_default) {
                resetToDefaults();
            }
            loaded_ = true;
            return true;
        }
    }

    // 配置文件存在，正常加载
    std::ifstream file(config_path_);
    if (!file.is_open()) {
        resetToDefaults();
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    if (parseJSON(content)) {
        loaded_ = true;
        return true;
    }

    // 解析失败，使用默认配置并保存
    resetToDefaults();
    saveConfig(config_path_);
    loaded_ = true;
    return true;
}

bool ConfigManager::saveConfig(const std::string& config_path) {
    if (!config_path.empty()) {
        // 展开路径中的 ~ 符号
        config_path_ = expandPath(config_path);
    }

    // 确保目录存在
    fs::path path(config_path_);
    if (path.has_parent_path()) {
        fs::create_directories(path.parent_path());
    }

    std::ofstream file(config_path_);
    if (!file.is_open()) {
        return false;
    }

    file << generateJSON();
    file.close();
    return true;
}

// 简单的 JSON 解析器（专门用于配置文件）
bool ConfigManager::parseJSON(const std::string& json_content) {
    // 这是一个简化的 JSON 解析器，专门用于配置文件
    // 移除所有空白字符（除了字符串内的）
    std::string cleaned;
    bool in_string = false;
    for (char c : json_content) {
        if (c == '"') {
            in_string = !in_string;
            cleaned += c;
        } else if (in_string || (!std::isspace(c) && c != '\n' && c != '\r')) {
            cleaned += c;
        }
    }

    // 简单的键值对解析
    // 这里实现一个基本的解析逻辑
    // 由于 JSON 解析比较复杂，我们使用一个更简单的方法

    // 查找各个配置段
    size_t editor_pos = cleaned.find("\"editor\":{");
    size_t display_pos = cleaned.find("\"display\":{");
    size_t files_pos = cleaned.find("\"files\":{");
    size_t search_pos = cleaned.find("\"search\":{");
    size_t themes_pos = cleaned.find("\"themes\":{");
    size_t plugins_pos = cleaned.find("\"plugins\":{");

    // 辅助：从 section 内提取数字，section_end 为该段 "}" 位置
    auto extractInt = [&cleaned](const std::string& key, size_t start, size_t section_end,
                                 int default_val) -> int {
        size_t pos = cleaned.find("\"" + key + "\":", start);
        if (pos == std::string::npos || pos >= section_end)
            return default_val;
        pos += key.length() + 3;
        size_t end = cleaned.find_first_of(",}", pos);
        if (end == std::string::npos || end > section_end)
            return default_val;
        try {
            return std::stoi(cleaned.substr(pos, end - pos));
        } catch (...) {
            return default_val;
        }
    };
    auto extractBool = [&cleaned](const std::string& key, size_t start, size_t section_end,
                                  bool default_val) -> bool {
        size_t pos = cleaned.find("\"" + key + "\":", start);
        if (pos == std::string::npos || pos >= section_end)
            return default_val;
        pos += key.length() + 3;
        std::string val = cleaned.substr(pos, 5);
        if (val.find("true") == 0)
            return true;
        if (val.find("false") == 0)
            return false;
        return default_val;
    };
    auto extractStr = [&cleaned](const std::string& key, size_t start,
                                 size_t section_end) -> std::string {
        size_t pos = cleaned.find("\"" + key + "\":\"", start);
        if (pos == std::string::npos || pos >= section_end)
            return "";
        pos += key.length() + 4;
        size_t end = cleaned.find("\"", pos);
        if (end == std::string::npos || end > section_end)
            return "";
        return cleaned.substr(pos, end - pos);
    };

    size_t editor_end =
        (editor_pos != std::string::npos) ? cleaned.find("}", editor_pos + 1) : std::string::npos;
    if (editor_pos != std::string::npos && editor_end != std::string::npos) {
        config_.editor.theme = extractStr("theme", editor_pos, editor_end);
        if (!config_.editor.theme.empty()) {
            config_.current_theme = config_.editor.theme;
        }
        config_.editor.font_size = extractInt("font_size", editor_pos, editor_end, 12);
        config_.editor.tab_size = extractInt("tab_size", editor_pos, editor_end, 4);
        config_.editor.insert_spaces = extractBool("insert_spaces", editor_pos, editor_end, true);
        config_.editor.word_wrap = extractBool("word_wrap", editor_pos, editor_end, false);
        config_.editor.auto_indent = extractBool("auto_indent", editor_pos, editor_end, true);
    }

    // 解析 display 配置
    size_t display_end =
        (display_pos != std::string::npos) ? cleaned.find("}", display_pos + 1) : std::string::npos;
    if (display_pos != std::string::npos && display_end != std::string::npos) {
        config_.display.show_line_numbers =
            extractBool("show_line_numbers", display_pos, display_end, true);
        config_.display.relative_line_numbers =
            extractBool("relative_line_numbers", display_pos, display_end, false);
        config_.display.highlight_current_line =
            extractBool("highlight_current_line", display_pos, display_end, true);
        config_.display.show_whitespace =
            extractBool("show_whitespace", display_pos, display_end, false);

        std::string style = extractStr("cursor_style", display_pos, display_end);
        if (!style.empty())
            config_.display.cursor_style = style;
        std::string color = extractStr("cursor_color", display_pos, display_end);
        if (!color.empty())
            config_.display.cursor_color = color;
        int rate = extractInt("cursor_blink_rate", display_pos, display_end, 500);
        config_.display.cursor_blink_rate = rate;
        config_.display.cursor_smooth =
            extractBool("cursor_smooth", display_pos, display_end, false);
    }

    // 解析 files 配置
    size_t files_end =
        (files_pos != std::string::npos) ? cleaned.find("}", files_pos + 1) : std::string::npos;
    if (files_pos != std::string::npos && files_end != std::string::npos) {
        std::string enc = extractStr("encoding", files_pos, files_end);
        if (!enc.empty())
            config_.files.encoding = enc;
        std::string le = extractStr("line_ending", files_pos, files_end);
        if (!le.empty())
            config_.files.line_ending = le;
        config_.files.trim_trailing_whitespace =
            extractBool("trim_trailing_whitespace", files_pos, files_end, true);
        config_.files.insert_final_newline =
            extractBool("insert_final_newline", files_pos, files_end, true);
        config_.files.auto_save = extractBool("auto_save", files_pos, files_end, false);
        config_.files.auto_save_interval =
            extractInt("auto_save_interval", files_pos, files_end, 60);
    }

    // 解析 search 配置
    size_t search_end =
        (search_pos != std::string::npos) ? cleaned.find("}", search_pos + 1) : std::string::npos;
    if (search_pos != std::string::npos && search_end != std::string::npos) {
        config_.search.case_sensitive =
            extractBool("case_sensitive", search_pos, search_end, false);
        config_.search.whole_word = extractBool("whole_word", search_pos, search_end, false);
        config_.search.regex = extractBool("regex", search_pos, search_end, false);
        config_.search.wrap_around = extractBool("wrap_around", search_pos, search_end, true);
    }

    // 解析 themes 配置
    if (themes_pos != std::string::npos) {
        // 提取 current theme
        size_t current_pos = cleaned.find("\"current\":\"", themes_pos);
        if (current_pos != std::string::npos) {
            current_pos += 11;
            size_t current_end = cleaned.find("\"", current_pos);
            if (current_end != std::string::npos) {
                config_.current_theme = cleaned.substr(current_pos, current_end - current_pos);
                config_.editor.theme = config_.current_theme;
            }
        }

        // 解析自定义主题（简化处理）
        // 这里可以扩展解析 custom 主题
    }

    // 解析 plugins 配置
    if (plugins_pos != std::string::npos) {
        // 提取 enabled_plugins 数组
        size_t enabled_pos = cleaned.find("\"enabled_plugins\":", plugins_pos);
        if (enabled_pos != std::string::npos && enabled_pos < plugins_pos + 300) {
            enabled_pos += 18; // 跳过 "enabled_plugins":
            size_t array_start = cleaned.find("[", enabled_pos);
            if (array_start != std::string::npos) {
                size_t array_end = cleaned.find("]", array_start);
                if (array_end != std::string::npos) {
                    std::string array_content =
                        cleaned.substr(array_start + 1, array_end - array_start - 1);
                    // 简单的数组解析
                    size_t pos = 0;
                    while ((pos = array_content.find("\"", pos)) != std::string::npos) {
                        pos++; // 跳过引号
                        size_t end_pos = array_content.find("\"", pos);
                        if (end_pos != std::string::npos) {
                            std::string plugin_name = array_content.substr(pos, end_pos - pos);
                            if (!plugin_name.empty()) {
                                config_.plugins.enabled_plugins.push_back(plugin_name);
                            }
                            pos = end_pos + 1;
                        } else {
                            break;
                        }
                    }
                }
            }
        }
    }

    return true;
}

std::string ConfigManager::generateJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"editor\": {\n";
    oss << "    \"_comment\": \"Editor: theme, font, tab, indent, wrap\",\n";
    oss << "    \"theme\": \"" << config_.editor.theme << "\",\n";
    oss << "    \"font_size\": " << config_.editor.font_size << ",\n";
    oss << "    \"tab_size\": " << config_.editor.tab_size << ",\n";
    oss << "    \"insert_spaces\": " << (config_.editor.insert_spaces ? "true" : "false") << ",\n";
    oss << "    \"word_wrap\": " << (config_.editor.word_wrap ? "true" : "false") << ",\n";
    oss << "    \"auto_indent\": " << (config_.editor.auto_indent ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"display\": {\n";
    oss << "    \"_comment\": \"Display: line numbers, highlight, cursor style\",\n";
    oss << "    \"show_line_numbers\": " << (config_.display.show_line_numbers ? "true" : "false")
        << ",\n";
    oss << "    \"relative_line_numbers\": "
        << (config_.display.relative_line_numbers ? "true" : "false") << ",\n";
    oss << "    \"highlight_current_line\": "
        << (config_.display.highlight_current_line ? "true" : "false") << ",\n";
    oss << "    \"show_whitespace\": " << (config_.display.show_whitespace ? "true" : "false")
        << ",\n";
    oss << "    \"cursor_style\": \"" << config_.display.cursor_style << "\",\n";
    oss << "    \"cursor_color\": \"" << config_.display.cursor_color << "\",\n";
    oss << "    \"cursor_blink_rate\": " << config_.display.cursor_blink_rate << ",\n";
    oss << "    \"cursor_smooth\": " << (config_.display.cursor_smooth ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"files\": {\n";
    oss << "    \"_comment\": \"Files: encoding, line ending, auto save\",\n";
    oss << "    \"encoding\": \"" << config_.files.encoding << "\",\n";
    oss << "    \"line_ending\": \"" << config_.files.line_ending << "\",\n";
    oss << "    \"trim_trailing_whitespace\": "
        << (config_.files.trim_trailing_whitespace ? "true" : "false") << ",\n";
    oss << "    \"insert_final_newline\": "
        << (config_.files.insert_final_newline ? "true" : "false") << ",\n";
    oss << "    \"auto_save\": " << (config_.files.auto_save ? "true" : "false") << ",\n";
    oss << "    \"auto_save_interval\": " << config_.files.auto_save_interval << "\n";
    oss << "  },\n";
    oss << "  \"search\": {\n";
    oss << "    \"_comment\": \"Search: case, whole word, regex, wrap\",\n";
    oss << "    \"case_sensitive\": " << (config_.search.case_sensitive ? "true" : "false")
        << ",\n";
    oss << "    \"whole_word\": " << (config_.search.whole_word ? "true" : "false") << ",\n";
    oss << "    \"regex\": " << (config_.search.regex ? "true" : "false") << ",\n";
    oss << "    \"wrap_around\": " << (config_.search.wrap_around ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"themes\": {\n";
    oss << "    \"_comment\": \"Themes: current theme, available list for reference\",\n";
    oss << "    \"current\": \"" << config_.current_theme << "\",\n";
    oss << "    \"available\": [\n";
    for (size_t i = 0; i < config_.available_themes.size(); ++i) {
        oss << "      \"" << config_.available_themes[i] << "\"";
        if (i < config_.available_themes.size() - 1)
            oss << ",";
        oss << "\n";
    }
    oss << "    ],\n";
    oss << "    \"_comment_available_themes_1\": \"monokai, dracula, solarized-dark, "
        << "solarized-light, onedark, nord, gruvbox, tokyo-night, catppuccin, material\",\n";
    oss << "    \"_comment_available_themes_2\": \"ayu, github, github-dark, markdown-dark, "
        << "vscode-dark, night-owl, palenight, oceanic-next, kanagawa, tomorrow-night, "
        << "tomorrow-night-blue, cobalt\",\n";
    oss << "    \"_comment_available_themes_3\": \"zenburn, base16-dark, papercolor, rose-pine, "
        << "everforest, jellybeans, desert, slate, atom-one-light, tokyo-night-day, "
        << "blue-light, cyberpunk, hacker\"\n";
    oss << "  },\n";
    oss << "  \"plugins\": {\n";
    oss << "    \"_comment\": \"Plugins: enabled plugin names\",\n";
    oss << "    \"enabled_plugins\": [\n";
    for (size_t i = 0; i < config_.plugins.enabled_plugins.size(); ++i) {
        oss << "      \"" << config_.plugins.enabled_plugins[i] << "\"";
        if (i < config_.plugins.enabled_plugins.size() - 1)
            oss << ",";
        oss << "\n";
    }
    oss << "    ]\n";
    oss << "  }\n";
    oss << "}\n";
    return oss.str();
}

bool ConfigManager::parseEditorConfig(const std::map<std::string, std::string>& /* data */) {
    // 实现编辑器配置解析
    return true;
}

bool ConfigManager::parseDisplayConfig(const std::map<std::string, std::string>& /* data */) {
    // 实现显示配置解析
    return true;
}

bool ConfigManager::parseFileConfig(const std::map<std::string, std::string>& /* data */) {
    // 实现文件配置解析
    return true;
}

bool ConfigManager::parseSearchConfig(const std::map<std::string, std::string>& /* data */) {
    // 实现搜索配置解析
    return true;
}

bool ConfigManager::parseThemeConfig(const std::map<std::string, std::string>& /* data */) {
    // 实现主题配置解析
    return true;
}

bool ConfigManager::parsePluginConfig(const std::map<std::string, std::string>& /* data */) {
    // 插件配置解析（目前通过 parseJSON 直接处理）
    return true;
}

std::vector<int> ConfigManager::parseColorArray(const std::string& /* color_str */) {
    std::vector<int> result;
    // 解析 [r, g, b] 格式
    // 简化实现
    return result;
}

std::string ConfigManager::colorArrayToString(const std::vector<int>& color) const {
    if (color.size() >= 3) {
        return "[" + std::to_string(color[0]) + "," + std::to_string(color[1]) + "," +
               std::to_string(color[2]) + "]";
    }
    return "[0,0,0]";
}

bool ConfigManager::stringToBool(const std::string& str) {
    return str == "true" || str == "1";
}

int ConfigManager::stringToInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (...) {
        return 0;
    }
}

} // namespace core
} // namespace pnana
