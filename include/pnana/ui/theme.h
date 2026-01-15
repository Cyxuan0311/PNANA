#ifndef PNANA_UI_THEME_H
#define PNANA_UI_THEME_H

#include <ftxui/screen/color.hpp>
#include <map>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 主题颜色定义
struct ThemeColors {
    // UI元素
    ftxui::Color background;
    ftxui::Color foreground;
    ftxui::Color current_line;
    ftxui::Color selection;
    ftxui::Color line_number;
    ftxui::Color line_number_current;

    // 状态栏
    ftxui::Color statusbar_bg;
    ftxui::Color statusbar_fg;

    // 菜单和帮助栏
    ftxui::Color menubar_bg;
    ftxui::Color menubar_fg;
    ftxui::Color helpbar_bg;
    ftxui::Color helpbar_fg;
    ftxui::Color helpbar_key;

    // 语法高亮
    ftxui::Color keyword;
    ftxui::Color string;
    ftxui::Color comment;
    ftxui::Color number;
    ftxui::Color function;
    ftxui::Color type;
    ftxui::Color operator_color;

    // 特殊元素
    ftxui::Color error;
    ftxui::Color warning;
    ftxui::Color info;
    ftxui::Color success;
};

class Theme {
  public:
    Theme();

    // 预设主题
    static ThemeColors Monokai();           // 经典Monokai主题
    static ThemeColors Dracula();           // Dracula主题
    static ThemeColors SolarizedDark();     // Solarized Dark
    static ThemeColors SolarizedLight();    // Solarized Light
    static ThemeColors OneDark();           // One Dark
    static ThemeColors Nord();              // Nord
    static ThemeColors Gruvbox();           // Gruvbox
    static ThemeColors TokyoNight();        // Tokyo Night
    static ThemeColors Catppuccin();        // Catppuccin
    static ThemeColors Material();          // Material
    static ThemeColors Ayu();               // Ayu
    static ThemeColors GitHub();            // GitHub
    static ThemeColors VSCodeDark();        // VS Code Dark+
    static ThemeColors NightOwl();          // Night Owl
    static ThemeColors Palenight();         // Material Palenight
    static ThemeColors OceanicNext();       // Oceanic Next
    static ThemeColors Kanagawa();          // Kanagawa
    static ThemeColors TomorrowNight();     // Tomorrow Night
    static ThemeColors TomorrowNightBlue(); // Tomorrow Night Blue
    static ThemeColors Cobalt();            // Cobalt
    static ThemeColors Zenburn();           // Zenburn
    static ThemeColors Base16Dark();        // Base16 Dark
    static ThemeColors PaperColor();        // PaperColor Dark
    static ThemeColors RosePine();          // Rose Pine
    static ThemeColors Everforest();        // Everforest
    static ThemeColors Jellybeans();        // Jellybeans
    static ThemeColors Desert();            // Desert
    static ThemeColors Slate();             // Slate

    void setTheme(const std::string& name);

    // 从配置加载自定义主题
    bool loadCustomTheme(const std::string& name, const ThemeColors& colors);

    // 从颜色配置结构加载主题
    bool loadThemeFromConfig(const std::vector<int>& background, const std::vector<int>& foreground,
                             const std::vector<int>& current_line,
                             const std::vector<int>& selection, const std::vector<int>& line_number,
                             const std::vector<int>& line_number_current,
                             const std::vector<int>& statusbar_bg,
                             const std::vector<int>& statusbar_fg,
                             const std::vector<int>& menubar_bg, const std::vector<int>& menubar_fg,
                             const std::vector<int>& helpbar_bg, const std::vector<int>& helpbar_fg,
                             const std::vector<int>& helpbar_key, const std::vector<int>& keyword,
                             const std::vector<int>& string, const std::vector<int>& comment,
                             const std::vector<int>& number, const std::vector<int>& function,
                             const std::vector<int>& type, const std::vector<int>& operator_color,
                             const std::vector<int>& error, const std::vector<int>& warning,
                             const std::vector<int>& info, const std::vector<int>& success);

    const ThemeColors& getColors() const {
        return colors_;
    }
    std::string getCurrentThemeName() const {
        return current_theme_;
    }

    // 获取所有可用的主题名称
    static std::vector<std::string> getAvailableThemes();

  private:
    ThemeColors colors_;
    std::string current_theme_;

    // 自定义主题存储
    std::map<std::string, ThemeColors> custom_themes_;

    // 辅助方法：从 RGB 数组创建 Color
    static ftxui::Color rgbToColor(const std::vector<int>& rgb);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_THEME_H
