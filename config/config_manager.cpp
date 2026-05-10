#include "core/config_manager.h"
#include "utils/logger.h"
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
        last_modified_time_ = fs::last_write_time(config_path_);
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
    last_modified_time_ = fs::last_write_time(config_path_);
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
    size_t lsp_pos = cleaned.find("\"lsp\":{");
    size_t animation_pos = cleaned.find("\"animation\":{");
    size_t history_pos = cleaned.find("\"history\":{");
    size_t ui_pos = cleaned.find("\"ui\":{");
    size_t lang_indent_pos = cleaned.find("\"language_indent\":{");

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
        config_.display.show_helpbar = extractBool("show_helpbar", display_pos, display_end, true);
        config_.display.logo_gradient =
            extractBool("logo_gradient", display_pos, display_end, true);
        std::string logo_style = extractStr("logo_style", display_pos, display_end);
        if (!logo_style.empty()) {
            config_.display.logo_style = logo_style;
        }
        config_.display.show_welcome_logo =
            extractBool("show_welcome_logo", display_pos, display_end, true);
        config_.display.welcome_logo_top_margin =
            extractInt("welcome_logo_top_margin", display_pos, display_end, 2);
        config_.display.welcome_screen_flex =
            extractBool("welcome_screen_flex", display_pos, display_end, true);
        config_.display.show_welcome_version =
            extractBool("show_welcome_version", display_pos, display_end, true);
        config_.display.show_welcome_start_hint =
            extractBool("show_welcome_start_hint", display_pos, display_end, true);
        config_.display.show_welcome_quick_start =
            extractBool("show_welcome_quick_start", display_pos, display_end, true);
        config_.display.show_welcome_features =
            extractBool("show_welcome_features", display_pos, display_end, true);
        config_.display.show_welcome_tips =
            extractBool("show_welcome_tips", display_pos, display_end, true);
        config_.display.show_tab_close_indicator =
            extractBool("show_tab_close_indicator", display_pos, display_end, true);

        // 文件浏览器配置
        config_.display.file_browser_show_tree_style =
            extractBool("file_browser_show_tree_style", display_pos, display_end, true);

        // 面板布局：文件列表和 AI 面板左右位置，可选 "left"/"right"
        std::string fb_side = extractStr("file_browser_side", display_pos, display_end);
        if (fb_side == "left" || fb_side == "right") {
            config_.display.file_browser_side = fb_side;
        }
        std::string ai_side = extractStr("ai_panel_side", display_pos, display_end);
        if (ai_side == "left" || ai_side == "right") {
            config_.display.ai_panel_side = ai_side;
        }
        // 终端位置：可选 "top"/"bottom"
        std::string term_side = extractStr("terminal_side", display_pos, display_end);
        if (term_side == "top" || term_side == "bottom") {
            config_.display.terminal_side = term_side;
        }
        // 状态栏样式：default, neovim, vscode, minimal, classic, highlight
        std::string sb_style = extractStr("statusbar_style", display_pos, display_end);
        if (!sb_style.empty()) {
            config_.display.statusbar_style = sb_style;
        }
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
        config_.files.max_file_size_before_prompt_mb =
            extractInt("max_file_size_before_prompt_mb", files_pos, files_end, 50);
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

        // 解析 available 主题列表（可选，用于控制主题菜单展示顺序/子集）
        size_t available_pos = cleaned.find("\"available\":[", themes_pos);
        if (available_pos != std::string::npos) {
            available_pos += 12; // 跳过 "available":[
            size_t avail_end = cleaned.find("]", available_pos);
            if (avail_end != std::string::npos) {
                std::string array_content =
                    cleaned.substr(available_pos, avail_end - available_pos);
                size_t p = 0;
                config_.available_themes.clear();
                while ((p = array_content.find("\"", p)) != std::string::npos) {
                    p++;
                    size_t e = array_content.find("\"", p);
                    if (e == std::string::npos)
                        break;
                    std::string name = array_content.substr(p, e - p);
                    if (!name.empty())
                        config_.available_themes.push_back(name);
                    p = e + 1;
                }
            }
        }

        // 解析 custom 主题（可选）：themes.custom 是一个对象，每个 key 是主题名
        size_t custom_pos = cleaned.find("\"custom\":{", themes_pos);
        if (custom_pos != std::string::npos) {
            custom_pos += 9; // 跳过 "custom":{
            size_t custom_end = custom_pos;
            int depth = 1;
            while (depth > 0 && custom_end < cleaned.size()) {
                char c = cleaned[custom_end++];
                if (c == '{')
                    depth++;
                else if (c == '}')
                    depth--;
            }
            if (depth == 0) {
                std::string custom_content =
                    cleaned.substr(custom_pos, custom_end - custom_pos - 1);
                size_t p = 0;
                while ((p = custom_content.find("\"", p)) != std::string::npos) {
                    // 解析主题名
                    p++;
                    size_t key_end = custom_content.find("\"", p);
                    if (key_end == std::string::npos)
                        break;
                    std::string theme_name = custom_content.substr(p, key_end - p);
                    p = key_end + 1;
                    // 找到冒号和值对象
                    size_t colon = custom_content.find(":", p);
                    if (colon == std::string::npos)
                        break;
                    size_t val_start = custom_content.find("{", colon);
                    if (val_start == std::string::npos)
                        break;
                    int vdepth = 1;
                    size_t val_end = val_start + 1;
                    while (vdepth > 0 && val_end < custom_content.size()) {
                        char c = custom_content[val_end++];
                        if (c == '{')
                            vdepth++;
                        else if (c == '}')
                            vdepth--;
                    }
                    if (vdepth != 0)
                        break;
                    std::string theme_obj =
                        custom_content.substr(val_start, val_end - val_start); // 包含花括号

                    ThemeColorConfig tcfg;
                    auto extractRgb = [&theme_obj](const std::string& key, std::vector<int>& out) {
                        size_t kpos = theme_obj.find("\"" + key + "\":[");
                        if (kpos == std::string::npos)
                            return;
                        kpos += key.length() + 4; // 跳过 "key":[
                        size_t end = theme_obj.find("]", kpos);
                        if (end == std::string::npos)
                            return;
                        std::string nums = theme_obj.substr(kpos, end - kpos);
                        out.clear();
                        size_t p2 = 0;
                        while (p2 < nums.size()) {
                            size_t comma = nums.find(",", p2);
                            std::string part = nums.substr(
                                p2, comma == std::string::npos ? std::string::npos : comma - p2);
                            try {
                                int v = std::stoi(part);
                                out.push_back(v);
                            } catch (...) {
                            }
                            if (comma == std::string::npos)
                                break;
                            p2 = comma + 1;
                        }
                    };

                    extractRgb("background", tcfg.background);
                    extractRgb("foreground", tcfg.foreground);
                    extractRgb("current_line", tcfg.current_line);
                    extractRgb("selection", tcfg.selection);
                    extractRgb("line_number", tcfg.line_number);
                    extractRgb("line_number_current", tcfg.line_number_current);
                    extractRgb("statusbar_bg", tcfg.statusbar_bg);
                    extractRgb("statusbar_fg", tcfg.statusbar_fg);
                    extractRgb("menubar_bg", tcfg.menubar_bg);
                    extractRgb("menubar_fg", tcfg.menubar_fg);
                    extractRgb("helpbar_bg", tcfg.helpbar_bg);
                    extractRgb("helpbar_fg", tcfg.helpbar_fg);
                    extractRgb("helpbar_key", tcfg.helpbar_key);
                    extractRgb("keyword", tcfg.keyword);
                    extractRgb("string", tcfg.string);
                    extractRgb("comment", tcfg.comment);
                    extractRgb("number", tcfg.number);
                    extractRgb("function", tcfg.function);
                    extractRgb("type", tcfg.type);
                    extractRgb("operator", tcfg.operator_color);
                    extractRgb("error", tcfg.error);
                    extractRgb("warning", tcfg.warning);
                    extractRgb("info", tcfg.info);
                    extractRgb("success", tcfg.success);

                    config_.custom_themes[theme_name] = tcfg;
                    p = val_end;
                }
            }
        }
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

    // 解析 animation 配置
    if (animation_pos != std::string::npos) {
        size_t animation_end = cleaned.find("}", animation_pos + 1);
        if (animation_end != std::string::npos) {
            config_.animation.enabled = extractBool("enabled", animation_pos, animation_end, false);
            std::string effect = extractStr("effect", animation_pos, animation_end);
            if (!effect.empty())
                config_.animation.effect = effect;
            config_.animation.refresh_interval_ms =
                extractInt("refresh_interval_ms", animation_pos, animation_end, 50);
            config_.animation.pulse_speed =
                static_cast<float>(extractInt("pulse_speed", animation_pos, animation_end, 5));
        }
    }

    // 解析 lsp 配置
    if (lsp_pos != std::string::npos) {
        size_t lsp_end = cleaned.find("}", lsp_pos + 1);
        if (lsp_end != std::string::npos) {
            config_.lsp.enabled = extractBool("enabled", lsp_pos, lsp_end, true);
            config_.lsp.completion_popup_enabled =
                extractBool("completion_popup_enabled", lsp_pos, lsp_end, true);

            // 解析 servers 数组
            size_t servers_pos = cleaned.find("\"servers\":[", lsp_pos);
            if (servers_pos != std::string::npos && servers_pos < lsp_end) {
                size_t array_start = servers_pos + 11; // 跳过 "servers":[
                size_t array_end = cleaned.find("]", array_start);
                if (array_end != std::string::npos) {
                    size_t obj_start = array_start;
                    while (obj_start < array_end) {
                        size_t brace = cleaned.find("{", obj_start);
                        if (brace == std::string::npos || brace >= array_end)
                            break;
                        size_t brace_end = brace + 1;
                        int depth = 1;
                        while (depth > 0 && brace_end < cleaned.size()) {
                            char c = cleaned[brace_end++];
                            if (c == '{')
                                depth++;
                            else if (c == '}')
                                depth--;
                        }
                        if (depth == 0) {
                            size_t obj_end = brace_end - 1;
                            LspServerConfigEntry entry;
                            entry.name = extractStr("name", brace, obj_end);
                            entry.command = extractStr("command", brace, obj_end);
                            entry.language_id = extractStr("language_id", brace, obj_end);

                            // 解析 extensions 数组
                            size_t ext_pos = cleaned.find("\"extensions\":[", brace);
                            if (ext_pos != std::string::npos && ext_pos < obj_end) {
                                ext_pos += 14;
                                size_t ext_end = cleaned.find("]", ext_pos);
                                std::string ext_content =
                                    cleaned.substr(ext_pos, ext_end - ext_pos);
                                size_t p = 0;
                                while ((p = ext_content.find("\"", p)) != std::string::npos) {
                                    p++;
                                    size_t e = ext_content.find("\"", p);
                                    if (e != std::string::npos) {
                                        std::string ex = ext_content.substr(p, e - p);
                                        if (!ex.empty())
                                            entry.extensions.push_back(ex);
                                        p = e + 1;
                                    } else
                                        break;
                                }
                            }

                            // 解析 args 数组
                            size_t args_pos = cleaned.find("\"args\":[", brace);
                            if (args_pos != std::string::npos && args_pos < obj_end) {
                                args_pos += 8;
                                size_t args_end = cleaned.find("]", args_pos);
                                std::string args_content =
                                    cleaned.substr(args_pos, args_end - args_pos);
                                size_t p = 0;
                                while ((p = args_content.find("\"", p)) != std::string::npos) {
                                    p++;
                                    size_t e = args_content.find("\"", p);
                                    if (e != std::string::npos) {
                                        std::string ar = args_content.substr(p, e - p);
                                        entry.args.push_back(ar);
                                        p = e + 1;
                                    } else
                                        break;
                                }
                            }

                            // 解析 env 对象
                            size_t env_pos = cleaned.find("\"env\":{", brace);
                            if (env_pos != std::string::npos && env_pos < obj_end) {
                                env_pos += 7;
                                size_t env_end = env_pos;
                                int env_depth = 1;
                                while (env_depth > 0 && env_end < cleaned.size()) {
                                    char c = cleaned[env_end++];
                                    if (c == '{')
                                        env_depth++;
                                    else if (c == '}')
                                        env_depth--;
                                }
                                if (env_depth == 0) {
                                    std::string env_content =
                                        cleaned.substr(env_pos, env_end - env_pos - 1);
                                    size_t p = 0;
                                    while ((p = env_content.find("\"", p)) != std::string::npos) {
                                        p++;
                                        size_t key_end = env_content.find("\"", p);
                                        if (key_end == std::string::npos)
                                            break;
                                        std::string key = env_content.substr(p, key_end - p);
                                        p = key_end + 1;
                                        size_t colon = env_content.find(":", p);
                                        if (colon == std::string::npos)
                                            break;
                                        p = env_content.find("\"", colon);
                                        if (p == std::string::npos)
                                            break;
                                        p++;
                                        size_t val_end = env_content.find("\"", p);
                                        if (val_end == std::string::npos)
                                            break;
                                        std::string val = env_content.substr(p, val_end - p);
                                        entry.env[key] = val;
                                        p = val_end + 1;
                                    }
                                }
                            }

                            if (!entry.name.empty() && !entry.command.empty() &&
                                !entry.language_id.empty())
                                config_.lsp.servers.push_back(entry);
                        }
                        obj_start = brace_end;
                    }
                }
            }
        }
    }

    // 解析 history 配置
    if (history_pos != std::string::npos) {
        size_t history_end = cleaned.find("}", history_pos + 1);
        if (history_end != std::string::npos) {
            config_.history.enable = extractBool("enable", history_pos, history_end, true);
            config_.history.max_entries = extractInt("max_entries", history_pos, history_end, 50);
            config_.history.max_age_days = extractInt("max_age_days", history_pos, history_end, 30);
            std::string mts = extractStr("max_total_size", history_pos, history_end);
            if (!mts.empty())
                config_.history.max_total_size = mts;
            config_.history.keep_critical_versions =
                extractBool("keep_critical_versions", history_pos, history_end, true);
            config_.history.critical_change_threshold =
                extractInt("critical_change_threshold", history_pos, history_end, 50);
            config_.history.critical_time_interval =
                extractInt("critical_time_interval", history_pos, history_end, 86400);
        }
    }

    // 解析 custom_logos 配置
    config_.custom_logos.clear();
    size_t custom_logos_pos = cleaned.find("\"custom_logos\":[");
    if (custom_logos_pos != std::string::npos) {
        size_t array_start = custom_logos_pos + 16; // 跳过 "custom_logos":[

        // 找到 custom_logos 外层数组的真正结束位置（不能直接 find(']')，
        // 因为每个 logo 的 lines 本身也是数组）
        size_t array_end = array_start;
        int arr_depth = 1;
        while (arr_depth > 0 && array_end < cleaned.size()) {
            char c = cleaned[array_end++];
            if (c == '[')
                arr_depth++;
            else if (c == ']')
                arr_depth--;
        }

        if (arr_depth == 0) {
            const size_t array_content_end = array_end - 1; // 指向外层 ]
            size_t obj_start = array_start;
            while (obj_start < array_content_end) {
                size_t brace = cleaned.find("{", obj_start);
                if (brace == std::string::npos || brace >= array_content_end)
                    break;

                size_t brace_end = brace + 1;
                int depth = 1;
                while (depth > 0 && brace_end < cleaned.size()) {
                    char c = cleaned[brace_end++];
                    if (c == '{')
                        depth++;
                    else if (c == '}')
                        depth--;
                }

                if (depth == 0) {
                    size_t obj_end = brace_end - 1;
                    CustomLogoConfig logo;
                    logo.id = extractStr("id", brace, obj_end);
                    logo.display_name = extractStr("display_name", brace, obj_end);

                    size_t lines_pos = cleaned.find("\"lines\":[", brace);
                    if (lines_pos != std::string::npos && lines_pos < obj_end) {
                        size_t lines_start = lines_pos + 9; // 跳过 "lines":[
                        size_t lines_end = cleaned.find("]", lines_start);
                        if (lines_end != std::string::npos && lines_end <= obj_end) {
                            std::string lines_content =
                                cleaned.substr(lines_start, lines_end - lines_start);
                            size_t p = 0;
                            while ((p = lines_content.find("\"", p)) != std::string::npos) {
                                p++;
                                size_t e = lines_content.find("\"", p);
                                if (e == std::string::npos)
                                    break;
                                logo.lines.push_back(lines_content.substr(p, e - p));
                                p = e + 1;
                            }
                        }
                    }

                    if (!logo.id.empty() && !logo.display_name.empty() && !logo.lines.empty()) {
                        config_.custom_logos.push_back(logo);
                    }
                }

                obj_start = brace_end;
            }
        }
    }

    // 解析 ui 配置
    if (ui_pos != std::string::npos) {
        size_t ui_end = cleaned.find("}", ui_pos + 1);
        if (ui_end != std::string::npos) {
            config_.ui.toast_enabled = extractBool("toast_enabled", ui_pos, ui_end, false);

            std::string toast_style = extractStr("toast_style", ui_pos, ui_end);
            if (!toast_style.empty()) {
                config_.ui.toast_style = toast_style;
            }

            config_.ui.toast_duration_ms =
                extractInt("toast_duration_ms", ui_pos, ui_end, config_.ui.toast_duration_ms);
            config_.ui.toast_max_width =
                extractInt("toast_max_width", ui_pos, ui_end, config_.ui.toast_max_width);
            config_.ui.toast_show_icon =
                extractBool("toast_show_icon", ui_pos, ui_end, config_.ui.toast_show_icon);
            config_.ui.toast_bold_text =
                extractBool("toast_bold_text", ui_pos, ui_end, config_.ui.toast_bold_text);

            // 解析最近项目数量限制
            config_.ui.max_recent_files =
                extractInt("max_recent_files", ui_pos, ui_end, config_.ui.max_recent_files);
            config_.ui.max_recent_folders =
                extractInt("max_recent_folders", ui_pos, ui_end, config_.ui.max_recent_folders);
        }
    }

    // 解析 language_indent 配置
    if (lang_indent_pos != std::string::npos) {
        size_t lang_indent_start = lang_indent_pos + 19;
        size_t lang_indent_end = lang_indent_start;
        int depth = 1;
        while (depth > 0 && lang_indent_end < cleaned.size()) {
            char c = cleaned[lang_indent_end++];
            if (c == '{')
                depth++;
            else if (c == '}')
                depth--;
        }
        if (depth == 0) {
            size_t p = lang_indent_start;
            while (p < lang_indent_end) {
                size_t quote_pos = cleaned.find("\"", p);
                if (quote_pos == std::string::npos || quote_pos >= lang_indent_end)
                    break;
                p = quote_pos + 1;
                size_t key_end = cleaned.find("\"", p);
                if (key_end == std::string::npos || key_end >= lang_indent_end)
                    break;
                std::string lang_id = cleaned.substr(p, key_end - p);
                p = key_end + 1;
                size_t colon = cleaned.find(":", p);
                if (colon == std::string::npos || colon >= lang_indent_end)
                    break;
                size_t brace_pos = colon + 1;
                while (brace_pos < lang_indent_end &&
                       (cleaned[brace_pos] == ' ' || cleaned[brace_pos] == '\t' ||
                        cleaned[brace_pos] == '\n' || cleaned[brace_pos] == '\r')) {
                    brace_pos++;
                }
                if (brace_pos >= lang_indent_end || cleaned[brace_pos] != '{') {
                    size_t skip = colon + 1;
                    while (skip < lang_indent_end &&
                           (cleaned[skip] == ' ' || cleaned[skip] == '\t' ||
                            cleaned[skip] == '\n' || cleaned[skip] == '\r')) {
                        skip++;
                    }
                    if (skip < lang_indent_end && cleaned[skip] == '"') {
                        skip++;
                        while (skip < lang_indent_end && cleaned[skip] != '"') {
                            if (cleaned[skip] == '\\')
                                skip++;
                            skip++;
                        }
                        if (skip < lang_indent_end)
                            skip++;
                    } else {
                        while (skip < lang_indent_end && cleaned[skip] != ',' &&
                               cleaned[skip] != '}')
                            skip++;
                    }
                    if (skip < lang_indent_end && cleaned[skip] == ',')
                        p = skip + 1;
                    else
                        break;
                    continue;
                }
                size_t val_start = brace_pos;
                int vdepth = 1;
                size_t val_end = val_start + 1;
                while (vdepth > 0 && val_end < lang_indent_end) {
                    char c = cleaned[val_end++];
                    if (c == '{')
                        vdepth++;
                    else if (c == '}')
                        vdepth--;
                }
                if (vdepth != 0)
                    break;

                LanguageIndentConfig lang_cfg;
                lang_cfg.indent_size = extractInt("indent_size", val_start, val_end, 4);
                lang_cfg.insert_spaces = extractBool("insert_spaces", val_start, val_end, true);
                lang_cfg.smart_indent = extractBool("smart_indent", val_start, val_end, true);

                size_t ext_pos = cleaned.find("\"file_extensions\":[", val_start);
                if (ext_pos != std::string::npos && ext_pos < val_end) {
                    ext_pos += 18;
                    size_t ext_end = cleaned.find("]", ext_pos);
                    if (ext_end != std::string::npos && ext_end < val_end) {
                        std::string ext_content = cleaned.substr(ext_pos, ext_end - ext_pos);
                        size_t ep = 0;
                        while ((ep = ext_content.find("\"", ep)) != std::string::npos) {
                            ep++;
                            size_t ee = ext_content.find("\"", ep);
                            if (ee == std::string::npos)
                                break;
                            std::string ext = ext_content.substr(ep, ee - ep);
                            if (!ext.empty())
                                lang_cfg.file_extensions.push_back(ext);
                            ep = ee + 1;
                        }
                    }
                }

                config_.language_indent[lang_id] = lang_cfg;
                p = val_end;
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
    oss << "    \"_comment\": \"Display: line numbers, highlight, cursor style, side panels, "
           "terminal position\",\n";
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
    oss << "    \"cursor_smooth\": " << (config_.display.cursor_smooth ? "true" : "false") << ",\n";
    oss << "    \"show_helpbar\": " << (config_.display.show_helpbar ? "true" : "false") << ",\n";
    oss << "    \"logo_gradient\": " << (config_.display.logo_gradient ? "true" : "false") << ",\n";
    oss << "    \"logo_style\": \"" << config_.display.logo_style << "\",\n";
    oss << "    \"show_welcome_logo\": " << (config_.display.show_welcome_logo ? "true" : "false")
        << ",\n";
    oss << "    \"welcome_logo_top_margin\": " << config_.display.welcome_logo_top_margin << ",\n";
    oss << "    \"welcome_screen_flex\": "
        << (config_.display.welcome_screen_flex ? "true" : "false") << ",\n";
    oss << "    \"show_welcome_version\": "
        << (config_.display.show_welcome_version ? "true" : "false") << ",\n";
    oss << "    \"show_welcome_start_hint\": "
        << (config_.display.show_welcome_start_hint ? "true" : "false") << ",\n";
    oss << "    \"show_welcome_quick_start\": "
        << (config_.display.show_welcome_quick_start ? "true" : "false") << ",\n";
    oss << "    \"show_welcome_features\": "
        << (config_.display.show_welcome_features ? "true" : "false") << ",\n";
    oss << "    \"show_welcome_tips\": " << (config_.display.show_welcome_tips ? "true" : "false")
        << ",\n";
    oss << "    \"show_tab_close_indicator\": "
        << (config_.display.show_tab_close_indicator ? "true" : "false") << ",\n";
    oss << "    \"file_browser_show_tree_style\": "
        << (config_.display.file_browser_show_tree_style ? "true" : "false") << ",\n";
    oss << "    \"file_browser_side\": \"" << config_.display.file_browser_side << "\",\n";
    oss << "    \"ai_panel_side\": \"" << config_.display.ai_panel_side << "\",\n";
    oss << "    \"terminal_side\": \"" << config_.display.terminal_side << "\",\n";
    oss << "    \"statusbar_style\": \"" << config_.display.statusbar_style << "\"\n";
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
    oss << "    \"auto_save_interval\": " << config_.files.auto_save_interval << ",\n";
    oss << "    \"max_file_size_before_prompt_mb\": "
        << config_.files.max_file_size_before_prompt_mb << "\n";
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
    oss << "  },\n";
    oss << "  \"animation\": {\n";
    oss << "    \"_comment\": \"Welcome animation: effect and tuning\",\n";
    oss << "    \"enabled\": " << (config_.animation.enabled ? "true" : "false") << ",\n";
    oss << "    \"effect\": \"" << config_.animation.effect << "\",\n";
    oss << "    \"refresh_interval_ms\": " << config_.animation.refresh_interval_ms << ",\n";
    oss << "    \"pulse_speed\": " << static_cast<int>(config_.animation.pulse_speed) << "\n";
    oss << "  },\n";
    oss << "  \"lsp\": {\n";
    oss << "    \"_comment\": \"LSP: config overrides built-in for same language_id; empty "
           "fields fall back to built-in. Add servers with new language_id to extend\",\n";
    oss << "    \"enabled\": " << (config_.lsp.enabled ? "true" : "false") << ",\n";
    oss << "    \"completion_popup_enabled\": "
        << (config_.lsp.completion_popup_enabled ? "true" : "false") << ",\n";
    oss << "    \"servers\": [\n";
    for (size_t i = 0; i < config_.lsp.servers.size(); ++i) {
        const auto& s = config_.lsp.servers[i];
        oss << "      {\"name\":\"" << s.name << "\",\"command\":\"" << s.command
            << "\",\"language_id\":\"" << s.language_id << "\",\"extensions\":[";
        for (size_t j = 0; j < s.extensions.size(); ++j) {
            oss << "\"" << s.extensions[j] << "\"";
            if (j < s.extensions.size() - 1)
                oss << ",";
        }
        oss << "],\"args\":[";
        for (size_t j = 0; j < s.args.size(); ++j) {
            oss << "\"" << s.args[j] << "\"";
            if (j < s.args.size() - 1)
                oss << ",";
        }
        oss << "],\"env\":{";
        size_t env_idx = 0;
        for (const auto& [k, v] : s.env) {
            oss << "\"" << k << "\":\"" << v << "\"";
            if (++env_idx < s.env.size())
                oss << ",";
        }
        oss << "}}";
        if (i < config_.lsp.servers.size() - 1)
            oss << ",";
        oss << "\n";
    }
    oss << "    ]\n";
    oss << "  },\n";
    oss << "  \"history\": {\n";
    oss << "    \"_comment\": \"History retention policy\",\n";
    oss << "    \"enable\": " << (config_.history.enable ? "true" : "false") << ",\n";
    oss << "    \"max_entries\": " << config_.history.max_entries << ",\n";
    oss << "    \"max_age_days\": " << config_.history.max_age_days << ",\n";
    oss << "    \"max_total_size\": \"" << config_.history.max_total_size << "\",\n";
    oss << "    \"keep_critical_versions\": "
        << (config_.history.keep_critical_versions ? "true" : "false") << ",\n";
    oss << "    \"critical_change_threshold\": " << config_.history.critical_change_threshold
        << ",\n";
    oss << "    \"critical_time_interval\": " << config_.history.critical_time_interval << "\n";
    oss << "  },\n";
    oss << "  \"custom_logos\": [\n";
    for (size_t i = 0; i < config_.custom_logos.size(); ++i) {
        const auto& logo = config_.custom_logos[i];
        oss << "    {\"id\":\"" << logo.id << "\",\"display_name\":\"" << logo.display_name
            << "\",\"lines\":[";
        for (size_t j = 0; j < logo.lines.size(); ++j) {
            oss << "\"" << logo.lines[j] << "\"";
            if (j < logo.lines.size() - 1)
                oss << ",";
        }
        oss << "]}";
        if (i < config_.custom_logos.size() - 1)
            oss << ",";
        oss << "\n";
    }
    oss << "  ],\n";
    oss << "  \"ui\": {\n";
    oss << "    \"_comment\": \"UI settings (Toast: "
           "enabled/style/duration/max_width/icon/bold)\",\n";
    oss << "    \"_comment_toast_style\": \"Toast style: classic | minimal | solid | accent | "
           "outline\",\n";
    oss << "    \"_comment_toast_duration\": \"toast_duration_ms: 0 means stay until "
           "replaced/hidden\",\n";
    oss << "    \"_comment_recent_projects\": \"max_recent_files/folders: limits for recently "
           "opened files/folders\",\n";
    oss << "    \"toast_enabled\": " << (config_.ui.toast_enabled ? "true" : "false") << ",\n";
    oss << "    \"toast_style\": \"" << config_.ui.toast_style << "\",\n";
    oss << "    \"toast_duration_ms\": " << config_.ui.toast_duration_ms << ",\n";
    oss << "    \"toast_max_width\": " << config_.ui.toast_max_width << ",\n";
    oss << "    \"toast_show_icon\": " << (config_.ui.toast_show_icon ? "true" : "false") << ",\n";
    oss << "    \"toast_bold_text\": " << (config_.ui.toast_bold_text ? "true" : "false") << ",\n";
    oss << "    \"max_recent_files\": " << config_.ui.max_recent_files << ",\n";
    oss << "    \"max_recent_folders\": " << config_.ui.max_recent_folders << "\n";
    oss << "  },\n";
    oss << "  \"language_indent\": {\n";
    oss << "    \"_comment\": \"Language-specific indent configuration for auto-indent module\",\n";
    for (auto it = config_.language_indent.begin(); it != config_.language_indent.end(); ++it) {
        const auto& lang_id = it->first;
        const auto& lang_cfg = it->second;
        oss << "    \"" << lang_id << "\": {\n";
        oss << "      \"indent_size\": " << lang_cfg.indent_size << ",\n";
        oss << "      \"insert_spaces\": " << (lang_cfg.insert_spaces ? "true" : "false") << ",\n";
        oss << "      \"smart_indent\": " << (lang_cfg.smart_indent ? "true" : "false") << ",\n";
        oss << "      \"file_extensions\": [";
        for (size_t i = 0; i < lang_cfg.file_extensions.size(); ++i) {
            oss << "\"" << lang_cfg.file_extensions[i] << "\"";
            if (i < lang_cfg.file_extensions.size() - 1)
                oss << ", ";
        }
        oss << "]\n";
        oss << "    }";
        if (std::next(it) != config_.language_indent.end())
            oss << ",";
        oss << "\n";
    }
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

void ConfigManager::registerChangeCallback(ConfigChangeCallback callback) {
    change_callbacks_.push_back(std::move(callback));
}

bool ConfigManager::reloadIfChanged() {
    if (config_path_.empty() || !fs::exists(config_path_)) {
        return false;
    }

    auto current_mtime = fs::last_write_time(config_path_);
    if (current_mtime == last_modified_time_) {
        return false;
    }

    if (!loadConfig(config_path_)) {
        return false;
    }

    for (auto& cb : change_callbacks_) {
        if (cb) {
            cb();
        }
    }

    return true;
}

void ConfigManager::startWatching(int interval_ms) {
    if (watching_.load()) {
        return;
    }

    watch_interval_ms_ = interval_ms;
    watching_.store(true);

    watcher_thread_ = std::thread([this]() {
        while (watching_.load()) {
            reloadIfChanged();
            std::this_thread::sleep_for(std::chrono::milliseconds(watch_interval_ms_));
        }
    });
}

void ConfigManager::stopWatching() {
    watching_.store(false);
    if (watcher_thread_.joinable()) {
        watcher_thread_.join();
    }
}

} // namespace core
} // namespace pnana
