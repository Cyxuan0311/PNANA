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

// 语言特定缩进配置
struct LanguageIndentConfig {
    int indent_size = 4;
    bool insert_spaces = true;
    bool smart_indent = true;
    std::vector<std::string> file_extensions; // 支持的文件后缀列表
};

// 显示配置结构
struct DisplayConfig {
    bool show_line_numbers = true;
    bool relative_line_numbers = false;
    bool highlight_current_line = true;
    bool show_whitespace = false;
    bool show_helpbar = true;

    // 光标配置
    std::string cursor_style = "block";       // block, underline, bar, hollow
    std::string cursor_color = "255,255,255"; // RGB格式，逗号分隔
    int cursor_blink_rate = 500;              // 闪烁频率（毫秒），0表示不闪烁
    bool cursor_smooth = false;               // 流动光标效果

    // Logo 配置
    bool logo_gradient = true;        // Logo 是否使用渐变颜色
    std::string logo_style = "block"; // Logo 样式：block, roman, box, unicode, script, big, diagram

    // 欢迎屏幕配置
    bool show_welcome_logo = true;   // 是否显示欢迎屏幕 Logo
    int welcome_logo_top_margin = 2; // Logo 距离顶部的空行数
    bool welcome_screen_flex = true; // 是否启用 flex 布局（启用时 welcome_logo_top_margin 无效）
    bool show_welcome_version = true;     // 是否显示版本信息
    bool show_welcome_start_hint = true;  // 是否显示"Press i to start editing"提示
    bool show_welcome_quick_start = true; // 是否显示快速开始快捷键
    bool show_welcome_features = true;    // 是否显示功能介绍
    bool show_welcome_tips = true;        // 是否显示提示信息

    // Tab 栏配置
    bool show_tab_close_indicator = true; // 是否在 tab 上显示关闭符号（×）

    // 文件浏览器配置
    bool file_browser_show_tree_style = true; // 是否显示树形结构样式（展开图标▼/▶和连接线│/├─/└─）
                                              // false 时使用空格代替，保持缩进但更简洁

    // 面板布局配置
    // file_browser_side: "left" 或 "right"，控制文件列表相对于代码区的位置
    // ai_panel_side: "left" 或 "right"，控制 AI 弹窗相对于代码区的位置
    std::string file_browser_side = "left";
    std::string ai_panel_side = "right";
    // terminal_side: "top" 或 "bottom"，控制终端相对于代码区的位置
    std::string terminal_side = "bottom";
    // statusbar_style: 状态栏样式名称，持久化到配置
    // 可选: default, neovim, vscode, minimal, classic, highlight, rounded, unicode
    std::string statusbar_style = "default";
};

// 文件配置结构
struct FileConfig {
    std::string encoding = "UTF-8";
    std::string line_ending = "LF";
    bool trim_trailing_whitespace = true;
    bool insert_final_newline = true;
    bool auto_save = false;
    int auto_save_interval = 60;
    // 超过此大小（MB）时打开前弹出确认对话框，0 表示不提示
    int max_file_size_before_prompt_mb = 50;
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

// 插件配置结构
struct PluginConfig {
    std::vector<std::string> enabled_plugins; // 已启用的插件列表
};

// LSP 服务器配置项（用于配置文件）
struct LspServerConfigEntry {
    std::string name;
    std::string command;
    std::string language_id;
    std::vector<std::string> extensions;
    std::vector<std::string> args;
    std::map<std::string, std::string> env;
};

// LSP 配置结构
struct LspConfig {
    bool enabled = true;
    bool completion_popup_enabled = true;      // 是否显示代码补全提示弹窗
    std::vector<LspServerConfigEntry> servers; // 为空时使用内置默认配置
};

// 欢迎页动画配置
struct AnimationConfig {
    bool enabled = false;
    std::string effect =
        "none"; // none, pulse, scanner, shimmer, wave, strobe, rainbow, flame, etc.
    int refresh_interval_ms = 50;

    // 通用参数
    float pulse_speed = 4.8f;
};

// 历史版本清理配置
struct HistoryConfig {
    bool enable = true;
    int max_entries = 50;
    int max_age_days = 30;
    std::string max_total_size = "1GB";
    bool keep_critical_versions = true;
    int critical_change_threshold = 50;
    int critical_time_interval = 86400;
};

// UI 配置结构
struct UIConfig {
    bool toast_enabled = false;          // 是否启用 Toast 弹窗通知
    std::string toast_style = "classic"; // classic/minimal/solid/accent/outline
    int toast_duration_ms = 3000;        // 默认显示时长（0=不自动消失）
    int toast_max_width = 50;            // 默认最大宽度
    bool toast_show_icon = true;         // 默认是否显示图标
    bool toast_bold_text = false;        // 默认是否粗体文本

    // 最近项目数量限制
    int max_recent_files = 8;   // 最近打开的文件最大数量
    int max_recent_folders = 4; // 最近打开的文件夹最大数量
};

// 自定义 Logo 配置
struct CustomLogoConfig {
    std::string id;                 // 唯一 ID，用于 logo_style 持久化
    std::string display_name;       // 菜单展示名
    std::vector<std::string> lines; // Logo 文本行
};

// 完整配置结构
struct AppConfig {
    EditorConfig editor;
    DisplayConfig display;
    FileConfig files;
    SearchConfig search;
    PluginConfig plugins;
    LspConfig lsp;
    AnimationConfig animation;
    HistoryConfig history;
    UIConfig ui;

    // Logo 配置
    std::vector<CustomLogoConfig> custom_logos; // 用户自定义 logo 列表

    // 语言特定缩进配置
    std::map<std::string, LanguageIndentConfig> language_indent;

    // 主题配置
    std::string current_theme = "monokai";
    std::map<std::string, ThemeColorConfig> custom_themes; // 用户自定义主题
    std::vector<std::string> available_themes;             // 供主题面板显示的主题名称
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
    bool parsePluginConfig(const std::map<std::string, std::string>& data);

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
