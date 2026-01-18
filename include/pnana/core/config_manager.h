#ifndef PNANA_CORE_CONFIG_MANAGER_H
#define PNANA_CORE_CONFIG_MANAGER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 编辑器配置结构
struct EditorConfig {
    std::string theme = "monokai";
    int font_size = 12;
    int tab_size = 4;
    bool insert_spaces = true;
    bool word_wrap = false;
    bool auto_indent = true;
};

// 显示配置结构
struct DisplayConfig {
    bool show_line_numbers = true;
    bool relative_line_numbers = false;
    bool highlight_current_line = true;
    bool show_whitespace = false;

    // 光标配置
    std::string cursor_style = "block";       // block, underline, bar, hollow
    std::string cursor_color = "255,255,255"; // RGB格式，逗号分隔
    int cursor_blink_rate = 500;              // 闪烁频率（毫秒），0表示不闪烁
    bool cursor_smooth = false;               // 流动光标效果
};

// 文件配置结构
struct FileConfig {
    std::string encoding = "UTF-8";
    std::string line_ending = "LF";
    bool trim_trailing_whitespace = true;
    bool insert_final_newline = true;
    bool auto_save = false;
    int auto_save_interval = 60;
};

// 搜索配置结构
struct SearchConfig {
    bool case_sensitive = false;
    bool whole_word = false;
    bool regex = false;
    bool wrap_around = true;
};

// 主题颜色配置（RGB 值）
struct ThemeColorConfig {
    // UI元素
    std::vector<int> background = {39, 40, 34};
    std::vector<int> foreground = {248, 248, 242};
    std::vector<int> current_line = {73, 72, 62};
    std::vector<int> selection = {73, 72, 62};
    std::vector<int> line_number = {144, 144, 138};
    std::vector<int> line_number_current = {248, 248, 242};

    // 状态栏
    std::vector<int> statusbar_bg = {45, 45, 45};
    std::vector<int> statusbar_fg = {248, 248, 242};

    // 菜单和帮助栏
    std::vector<int> menubar_bg = {30, 31, 27};
    std::vector<int> menubar_fg = {248, 248, 242};
    std::vector<int> helpbar_bg = {45, 45, 45};
    std::vector<int> helpbar_fg = {117, 113, 94};
    std::vector<int> helpbar_key = {166, 226, 46};

    // 语法高亮
    std::vector<int> keyword = {249, 38, 114};
    std::vector<int> string = {230, 219, 116};
    std::vector<int> comment = {117, 113, 94};
    std::vector<int> number = {174, 129, 255};
    std::vector<int> function = {166, 226, 46};
    std::vector<int> type = {102, 217, 239};
    std::vector<int> operator_color = {249, 38, 114};

    // 特殊元素
    std::vector<int> error = {249, 38, 114};
    std::vector<int> warning = {253, 151, 31};
    std::vector<int> info = {102, 217, 239};
    std::vector<int> success = {166, 226, 46};
};

// 完整配置结构
struct AppConfig {
    EditorConfig editor;
    DisplayConfig display;
    FileConfig files;
    SearchConfig search;

    // 主题配置
    std::string current_theme = "monokai";
    std::map<std::string, ThemeColorConfig> custom_themes;
    std::vector<std::string> available_themes;
};

// 配置管理器
class ConfigManager {
  public:
    ConfigManager();
    ~ConfigManager();

    // 加载配置文件
    bool loadConfig(const std::string& config_path = "");

    // 保存配置文件
    bool saveConfig(const std::string& config_path = "");

    // 获取配置
    const AppConfig& getConfig() const {
        return config_;
    }
    AppConfig& getConfig() {
        return config_;
    }

    // 获取默认配置路径
    static std::string getDefaultConfigPath();

    // 获取用户配置路径
    static std::string getUserConfigPath();

    // 检查配置是否已加载
    bool isLoaded() const {
        return loaded_;
    }

    // 重置为默认配置
    void resetToDefaults();

  private:
    AppConfig config_;
    std::string config_path_;
    bool loaded_;

    // JSON 解析辅助方法
    bool parseJSON(const std::string& json_content);
    bool parseEditorConfig(const std::map<std::string, std::string>& data);
    bool parseDisplayConfig(const std::map<std::string, std::string>& data);
    bool parseFileConfig(const std::map<std::string, std::string>& data);
    bool parseSearchConfig(const std::map<std::string, std::string>& data);
    bool parseThemeConfig(const std::map<std::string, std::string>& data);

    // JSON 生成辅助方法
    std::string generateJSON() const;

    // 工具方法
    std::vector<int> parseColorArray(const std::string& color_str);
    std::string colorArrayToString(const std::vector<int>& color) const;
    bool stringToBool(const std::string& str);
    int stringToInt(const std::string& str);
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_CONFIG_MANAGER_H
