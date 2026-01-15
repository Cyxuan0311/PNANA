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
    /* size_t files_pos = cleaned.find("\"files\":{"); */
    /* size_t search_pos = cleaned.find("\"search\":{"); */
    size_t themes_pos = cleaned.find("\"themes\":{");

    // 解析 editor 配置
    if (editor_pos != std::string::npos) {
        // 提取 theme
        size_t theme_pos = cleaned.find("\"theme\":\"", editor_pos);
        if (theme_pos != std::string::npos) {
            theme_pos += 9; // 跳过 "theme":"
            size_t theme_end = cleaned.find("\"", theme_pos);
            if (theme_end != std::string::npos) {
                config_.editor.theme = cleaned.substr(theme_pos, theme_end - theme_pos);
                config_.current_theme = config_.editor.theme;
            }
        }

        // 提取其他 editor 配置（简化处理）
        // font_size, tab_size 等
    }

    // 解析 display 配置
    if (display_pos != std::string::npos) {
        // 提取 cursor_style
        size_t style_pos = cleaned.find("\"cursor_style\":\"", display_pos);
        if (style_pos != std::string::npos && style_pos < display_pos + 200) {
            style_pos += 16; // 跳过 "cursor_style":"
            size_t style_end = cleaned.find("\"", style_pos);
            if (style_end != std::string::npos) {
                config_.display.cursor_style = cleaned.substr(style_pos, style_end - style_pos);
            }
        }

        // 提取 cursor_color
        size_t color_pos = cleaned.find("\"cursor_color\":\"", display_pos);
        if (color_pos != std::string::npos && color_pos < display_pos + 200) {
            color_pos += 16; // 跳过 "cursor_color":"
            size_t color_end = cleaned.find("\"", color_pos);
            if (color_end != std::string::npos) {
                config_.display.cursor_color = cleaned.substr(color_pos, color_end - color_pos);
            }
        }

        // 提取 cursor_blink_rate
        size_t rate_pos = cleaned.find("\"cursor_blink_rate\":", display_pos);
        if (rate_pos != std::string::npos && rate_pos < display_pos + 200) {
            rate_pos += 20; // 跳过 "cursor_blink_rate":
            size_t rate_end = cleaned.find_first_of(",}", rate_pos);
            if (rate_end != std::string::npos) {
                std::string rate_str = cleaned.substr(rate_pos, rate_end - rate_pos);
                try {
                    config_.display.cursor_blink_rate = std::stoi(rate_str);
                } catch (...) {
                    config_.display.cursor_blink_rate = 500;
                }
            }
        }

        // 提取 cursor_smooth
        size_t smooth_pos = cleaned.find("\"cursor_smooth\":", display_pos);
        if (smooth_pos != std::string::npos && smooth_pos < display_pos + 200) {
            smooth_pos += 16;                                       // 跳过 "cursor_smooth":
            std::string smooth_str = cleaned.substr(smooth_pos, 4); // "true" 或 "fals"
            config_.display.cursor_smooth = (smooth_str.find("true") != std::string::npos);
        }
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

    return true;
}

std::string ConfigManager::generateJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"editor\": {\n";
    oss << "    \"theme\": \"" << config_.editor.theme << "\",\n";
    oss << "    \"font_size\": " << config_.editor.font_size << ",\n";
    oss << "    \"tab_size\": " << config_.editor.tab_size << ",\n";
    oss << "    \"insert_spaces\": " << (config_.editor.insert_spaces ? "true" : "false") << ",\n";
    oss << "    \"word_wrap\": " << (config_.editor.word_wrap ? "true" : "false") << ",\n";
    oss << "    \"auto_indent\": " << (config_.editor.auto_indent ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"display\": {\n";
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
    oss << "    \"case_sensitive\": " << (config_.search.case_sensitive ? "true" : "false")
        << ",\n";
    oss << "    \"whole_word\": " << (config_.search.whole_word ? "true" : "false") << ",\n";
    oss << "    \"regex\": " << (config_.search.regex ? "true" : "false") << ",\n";
    oss << "    \"wrap_around\": " << (config_.search.wrap_around ? "true" : "false") << "\n";
    oss << "  },\n";
    oss << "  \"themes\": {\n";
    oss << "    \"current\": \"" << config_.current_theme << "\",\n";
    oss << "    \"available\": [\n";
    for (size_t i = 0; i < config_.available_themes.size(); ++i) {
        oss << "      \"" << config_.available_themes[i] << "\"";
        if (i < config_.available_themes.size() - 1)
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
