#ifndef PNANA_UI_THEME_H
#define PNANA_UI_THEME_H

#include <ftxui/screen/color.hpp>
#include <string>
#include <map>

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
    static ThemeColors Monokai();      // 经典Monokai主题
    static ThemeColors Dracula();      // Dracula主题
    static ThemeColors SolarizedDark(); // Solarized Dark
    static ThemeColors SolarizedLight(); // Solarized Light
    static ThemeColors OneDark();      // One Dark
    static ThemeColors Nord();         // Nord
    static ThemeColors Gruvbox();      // Gruvbox
    static ThemeColors TokyoNight();   // Tokyo Night
    static ThemeColors Catppuccin();   // Catppuccin
    static ThemeColors Material();     // Material
    static ThemeColors Ayu();          // Ayu
    static ThemeColors GitHub();      // GitHub
    
    void setTheme(const std::string& name);
    const ThemeColors& getColors() const { return colors_; }
    std::string getCurrentThemeName() const { return current_theme_; }
    
private:
    ThemeColors colors_;
    std::string current_theme_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_THEME_H

