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

    // 弹窗
    ftxui::Color dialog_bg;
    ftxui::Color dialog_fg;
    ftxui::Color dialog_title_bg;
    ftxui::Color dialog_title_fg;
    ftxui::Color dialog_border;
};

class Theme {
  public:
    Theme();

    // 预设主题
    static ThemeColors Monokai();                // 经典Monokai主题
    static ThemeColors Dracula();                // Dracula主题
    static ThemeColors SolarizedDark();          // Solarized Dark
    static ThemeColors SolarizedLight();         // Solarized Light
    static ThemeColors OneDark();                // One Dark
    static ThemeColors Nord();                   // Nord
    static ThemeColors Gruvbox();                // Gruvbox
    static ThemeColors TokyoNight();             // Tokyo Night
    static ThemeColors Catppuccin();             // Catppuccin
    static ThemeColors Material();               // Material
    static ThemeColors Ayu();                    // Ayu
    static ThemeColors GitHub();                 // GitHub Light
    static ThemeColors GitHubDark();             // GitHub Dark
    static ThemeColors GitHubDarkDimmed();       // GitHub Dark Dimmed - 柔和暗色主题
    static ThemeColors GitHubDarkHighContrast(); // GitHub Dark High Contrast - 高对比度主题
    static ThemeColors MarkdownDark();           // Markdown Dark
    static ThemeColors VSCodeDark();             // VS Code Dark+
    static ThemeColors NightOwl();               // Night Owl
    static ThemeColors Palenight();              // Material Palenight
    static ThemeColors OceanicNext();            // Oceanic Next
    static ThemeColors Kanagawa();               // Kanagawa
    static ThemeColors TomorrowNight();          // Tomorrow Night
    static ThemeColors TomorrowNightBlue();      // Tomorrow Night Blue
    static ThemeColors Cobalt();                 // Cobalt
    static ThemeColors Zenburn();                // Zenburn
    static ThemeColors Base16Dark();             // Base16 Dark
    static ThemeColors PaperColor();             // PaperColor Dark
    static ThemeColors RosePine();               // Rose Pine
    static ThemeColors Everforest();             // Everforest
    static ThemeColors Jellybeans();             // Jellybeans
    static ThemeColors Desert();                 // Desert
    static ThemeColors Slate();                  // Slate
    static ThemeColors AtomOneLight();           // Atom One Light
    static ThemeColors TokyoNightDay();          // Tokyo Night Day
    static ThemeColors BlueLight();              // Blue Light - 浅蓝主色，白字黑底
    static ThemeColors Cyberpunk();              // Cyberpunk - 赛博朋克霓虹风
    static ThemeColors Hacker();                 // Hacker - 黑客/Matrix 终端风
    static ThemeColors HatsuneMiku();            // 初音未来 - 葱色/粉/金黄
    static ThemeColors Minions();                // 小黄人 - 黄蓝配色
    static ThemeColors Batman();                 // 蝙蝠侠 - 黑金/哥谭风
    static ThemeColors SpongeBob();              // 海绵宝宝 - 海底黄蓝珊瑚风
    static ThemeColors ModusVivendi();           // Modus Vivendi - 高对比度护眼深色
    static ThemeColors ModusOperandi();          // Modus Operandi - 高对比度护眼浅色
    static ThemeColors Horizon();                // Horizon - 暖色深色粉橙风
    static ThemeColors Oxocarbon();              // Oxocarbon - IBM Carbon 风格深灰蓝
    static ThemeColors Poimandres();             // Poimandres - 紫青深色主题
    static ThemeColors Terafox();                // Terafox - 暖棕深色主题
    static ThemeColors Mellow();                 // Mellow - 柔和 pastel 深色
    static ThemeColors Fleet();                  // Fleet - JetBrains Fleet 风格深色
    static ThemeColors Luna();                   // Luna - 柔和紫蓝深色
    static ThemeColors Retro();                  // Retro - 复古 CRT 琥珀荧光屏
    static ThemeColors Sunset();                 // Sunset - 日落暖橙红风
    static ThemeColors Forest();                 // Forest - 森林绿深色
    static ThemeColors Ocean();                  // Ocean - 深海蓝青
    static ThemeColors TangoDark();              // Tango Dark - Tango 调色板深色
    static ThemeColors Synthwave();              // Synthwave - 80 年代霓虹紫青粉
    static ThemeColors Decay();                  // Decay - 冷灰蓝低饱和
    static ThemeColors RiderDark();              // Rider Dark - JetBrains 灰底紫蓝
    static ThemeColors ParchmentDark();          // Parchment Dark - 深褐羊皮纸风
    static ThemeColors Crimson();                // Crimson - 深红黑绯红主色
    static ThemeColors Frost();                  // Frost - 冷冽冰蓝深色
    static ThemeColors Lavender();               // Lavender - 薰衣草浅色
    static ThemeColors Matcha();                 // Matcha - 日式抹茶绿主题
    static ThemeColors Aurora();                 // Aurora - 极光青紫粉霓虹主题
    static ThemeColors Amber();                  // Amber - 暖黑琥珀金
    static ThemeColors Mint();                   // Mint - 深灰薄荷青终端风
    static ThemeColors Obsidian();               // Obsidian - 深黑蓝灰冷静风
    static ThemeColors Coffee();                 // Coffee - 咖啡棕深色护眼
    static ThemeColors Ink();                    // Ink - 墨色深靛蓝+金/米色
    static ThemeColors Sakura();                 // Sakura - 樱浅粉白+玫瑰灰
    static ThemeColors SakuraDark();             // Sakura Dark - 樱暗色版深玫底
    static ThemeColors Monochrome();             // Monochrome - 黑白灰无彩色
    static ThemeColors NeonNoir();               // Neon Noir - 霓虹 noir 深黑+青/品红
    static ThemeColors WarmSepia();              // Warm Sepia - 暖棕褐旧报纸风
    static ThemeColors Colorful();               // Colorful - 彩色多色高饱和语法高亮
    static ThemeColors Microsoft();              // Microsoft - 微软/Fluent 深灰+蓝主色
    static ThemeColors Google();                 // Google - 谷歌品牌色蓝/红/黄/绿浅色
    static ThemeColors Meta();                   // Meta - Meta/Facebook 深色+蓝主色
    static ThemeColors IntelliJDark(); // IntelliJ Dark - JetBrains IDEA 经典深色主题
    static ThemeColors DoomOne();      // Doom One - Emacs Doom 默认主题
    static ThemeColors VSCodeLight();  // VSCode Light - Visual Studio Code 默认浅色主题
    static ThemeColors Andromeda();    // Andromeda - 深蓝紫基底流行 VSCode 主题

    void setTheme(const std::string& name);

    // 从配置加载自定义主题
    bool loadCustomTheme(const std::string& name, const ThemeColors& colors);

    // 移除自定义主题
    bool removeCustomTheme(const std::string& name);

    // 清除所有自定义主题
    void clearCustomThemes();

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
                             const std::vector<int>& info, const std::vector<int>& success,
                             const std::vector<int>& dialog_bg, const std::vector<int>& dialog_fg,
                             const std::vector<int>& dialog_title_bg,
                             const std::vector<int>& dialog_title_fg,
                             const std::vector<int>& dialog_border);

    const ThemeColors& getColors() const {
        return colors_;
    }
    std::string getCurrentThemeName() const {
        return current_theme_;
    }

    // 获取所有可用的主题名称
    static std::vector<std::string> getAvailableThemes();

    // 获取自定义主题名称
    std::vector<std::string> getCustomThemeNames() const;

    // 获取当前主题的 Logo 渐变颜色
    std::vector<ftxui::Color> getGradientColors() const;

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
