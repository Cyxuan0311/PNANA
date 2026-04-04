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
    static ThemeColors Monokai();        // Monokai Pro - 经典Monokai主题
    static ThemeColors MonokaiDark();    // Monokai Dark - 更深的暗色版本，增强对比度
    static ThemeColors MonokaiLight();   // Monokai Light - 浅色版本，适合明亮环境
    static ThemeColors MonokaiNeon();    // Monokai Neon - 霓虹版本，增强发光效果
    static ThemeColors MonokaiPastel();  // Monokai Pastel - 粉彩版本，柔和的马卡龙色系
    static ThemeColors SolarizedDark();  // Solarized Dark
    static ThemeColors SolarizedLight(); // Solarized Light
    static ThemeColors OneDark();        // One Dark
    static ThemeColors Nord();           // Nord Arctic - 经典 Nord，冷冽冰蓝北极风格
    static ThemeColors NordFrost();      // Nord Frost - 霜冻版本，更浅的冰蓝色调
    static ThemeColors NordAurora();     // Nord Aurora - 极光版本，强调极光色彩
    static ThemeColors NordDeep();       // Nord Deep - 深海版本，更深的蓝色调
    static ThemeColors NordLight();      // Nord Light - 浅色版本，雪白色调
    static ThemeColors NordStorm();      // Nord Storm - 风暴版本，更强烈的对比度
    static ThemeColors NordPolar();      // Nord Polar - 极地版本，更冷的色调
    static ThemeColors NordMidnight();   // Nord Midnight - 午夜版本，最深的蓝色调
    static ThemeColors NordGlacier();    // Nord Glacier - 冰川版本，更亮的冰蓝色调
    static ThemeColors PurpleDark();     // Purple Dark - 紫黑主题，深紫底色 + 亮紫点缀
    static ThemeColors Gruvbox();        // Gruvbox
    static ThemeColors TokyoNight();     // Tokyo Night
    static ThemeColors Catppuccin();     // Catppuccin Mocha - 深色版本，暖紫灰基底
    static ThemeColors CatppuccinLatte();     // Catppuccin Latte - 浅色版本，暖白基底
    static ThemeColors CatppuccinFrappe();    // Catppuccin Frappé - 中等深色，冷紫灰基底
    static ThemeColors CatppuccinMacchiato(); // Catppuccin Macchiato - 较深版本，紫灰基底
    static ThemeColors Doraemon();            // 叮当猫/哆啦 A 梦 - 蓝白红黄经典配色
    static ThemeColors Material();            // Material
    static ThemeColors Ayu();                 // Ayu
    static ThemeColors GitHub();              // GitHub Light
    static ThemeColors GitHubDark();          // GitHub Dark
    static ThemeColors GitHubDarkDimmed();    // GitHub Dark Dimmed - 柔和暗色主题
    static ThemeColors GitHubDarkHighContrast();  // GitHub Dark High Contrast - 高对比度主题
    static ThemeColors GitHubLightHighContrast(); // GitHub Light High Contrast - 高对比度亮色
    static ThemeColors GitHubColorblind();        // GitHub Colorblind - 色盲友好主题
    static ThemeColors GitHubTritanopia();        // GitHub Tritanopia - 红绿色盲友好
    static ThemeColors GitHubSoft();              // GitHub Soft - 柔和暗色主题
    static ThemeColors GitHubMidnight();          // GitHub Midnight - 深夜蓝色主题
    static ThemeColors MarkdownDark();            // Markdown Dark
    static ThemeColors VSCodeDark();              // VS Code Dark+
    static ThemeColors VSCodeLight();             // VS Code Light
    static ThemeColors VSCodeLightModern();       // VS Code Light Modern
    static ThemeColors VSCodeDarkModern();        // VS Code Dark Modern
    static ThemeColors VSCodeMonokai();           // VS Code Monokai
    static ThemeColors VSCodeDarkPlus();          // VS Code Dark Plus
    static ThemeColors DarkPlusMoonLight();       // Dark Plus Moon Light - 月光银白蓝紫渐变
    static ThemeColors NightOwl();                // Night Owl
    static ThemeColors Palenight();               // Material Palenight
    static ThemeColors OceanicNext();             // Oceanic Next
    static ThemeColors Kanagawa();                // Kanagawa
    static ThemeColors TomorrowNight();           // Tomorrow Night
    static ThemeColors TomorrowNightBlue();       // Tomorrow Night Blue
    static ThemeColors Cobalt();                  // Cobalt
    static ThemeColors Zenburn();                 // Zenburn
    static ThemeColors Base16Dark();              // Base16 Dark
    static ThemeColors PaperColor();              // PaperColor Dark
    static ThemeColors RosePine();                // Rose Pine
    static ThemeColors Everforest();              // Everforest
    static ThemeColors Jellybeans();              // Jellybeans
    static ThemeColors Desert();                  // Desert
    static ThemeColors Slate();                   // Slate
    static ThemeColors AtomOneLight();            // Atom One Light
    static ThemeColors TokyoNightDay();           // Tokyo Night Day
    static ThemeColors BlueLight();               // Blue Light - 浅蓝主色，白字黑底
    static ThemeColors Cyberpunk();               // Cyberpunk - 赛博朋克霓虹风
    static ThemeColors Hacker();                  // Hacker - 黑客/Matrix 终端风
    static ThemeColors HatsuneMiku();             // 初音未来 - 葱色/粉/金黄
    static ThemeColors Minions();                 // 小黄人 - 黄蓝配色
    static ThemeColors Batman();                  // 蝙蝠侠 - 黑金/哥谭风
    static ThemeColors SpongeBob();               // 海绵宝宝 - 海底黄蓝珊瑚风
    static ThemeColors ModusVivendi();            // Modus Vivendi - 高对比度护眼深色
    static ThemeColors ModusOperandi();           // Modus Operandi - 高对比度护眼浅色
    static ThemeColors Horizon();                 // Horizon - 暖色深色粉橙风
    static ThemeColors Oxocarbon();               // Oxocarbon - IBM Carbon 风格深灰蓝
    static ThemeColors Poimandres();              // Poimandres - 紫青深色主题
    static ThemeColors Terafox();                 // Terafox - 暖棕深色主题
    static ThemeColors Galaxy();                  // Galaxy - 深紫蓝星空主题
    static ThemeColors Lightning();               // Lightning - 闪电黄电光主题
    static ThemeColors Storm();                   // Storm - 风暴深灰蓝主题
    static ThemeColors Mellow();                  // Mellow - 柔和 pastel 深色
    static ThemeColors Fleet();                   // Fleet - JetBrains Fleet 风格深色
    static ThemeColors Luna();                    // Luna - 柔和紫蓝深色
    static ThemeColors Retro();                   // Retro - 复古 CRT 琥珀荧光屏
    static ThemeColors Sunset();                  // Sunset - 日落暖橙红风
    static ThemeColors Forest();                  // Forest - 森林绿深色
    static ThemeColors Ocean();                   // Ocean - 深海蓝青
    static ThemeColors TangoDark();               // Tango Dark - Tango 调色板深色
    static ThemeColors Synthwave();               // Synthwave - 80 年代霓虹紫青粉
    static ThemeColors RetroFuture();             // Retro-Future - 复古未来主义 CRT 荧光风
    static ThemeColors Decay();                   // Decay - 冷灰蓝低饱和
    static ThemeColors RiderDark();               // Rider Dark - JetBrains 灰底紫蓝
    static ThemeColors ParchmentDark();           // Parchment Dark - 深褐羊皮纸风
    static ThemeColors Crimson();                 // Crimson - 深红黑绯红主色
    static ThemeColors Frost();                   // Frost - 冷冽冰蓝深色
    static ThemeColors Lavender();                // Lavender - 薰衣草浅色
    static ThemeColors Matcha();                  // Matcha - 日式抹茶绿主题
    static ThemeColors Aurora();                  // Aurora - 极光青紫粉霓虹主题
    static ThemeColors Amber();                   // Amber - 暖黑琥珀金
    static ThemeColors Mint();                    // Mint - 深灰薄荷青终端风
    static ThemeColors Obsidian();                // Obsidian - 深黑蓝灰冷静风
    static ThemeColors Coffee();                  // Coffee - 咖啡棕深色护眼
    static ThemeColors Ink();                     // Ink - 墨色深靛蓝+金/米色
    static ThemeColors Sakura();                  // Sakura - 樱浅粉白+玫瑰灰
    static ThemeColors SakuraDark();              // Sakura Dark - 樱暗色版深玫底
    static ThemeColors Monochrome();              // Monochrome - 黑白灰无彩色
    static ThemeColors NeonNoir();                // Neon Noir - 霓虹 noir 深黑+青/品红
    static ThemeColors WarmSepia();               // Warm Sepia - 暖棕褐旧报纸风
    static ThemeColors Colorful();                // Colorful - 彩色多色高饱和语法高亮
    static ThemeColors Microsoft();               // Microsoft - 微软/Fluent 深灰 + 蓝主色
    static ThemeColors Google();                  // Google - 谷歌品牌色蓝/红/黄/绿浅色
    static ThemeColors Meta();                    // Meta - Meta/Facebook 深色 + 蓝主色
    static ThemeColors IntelliJDark(); // IntelliJ Dark - JetBrains IDEA 经典深色主题
    static ThemeColors DoomOne();      // Doom One - Emacs Doom 默认主题
    static ThemeColors Andromeda();    // Andromeda - 深蓝紫基底流行 VSCode 主题
    static ThemeColors DeepSpace(); // Deep Space - 深空宇宙主题，深蓝紫基底配合星云色彩
    static ThemeColors Volcanic(); // Volcanic - 火山岩浆主题，深黑红基底配合岩浆橙/红色调
    static ThemeColors Arctic(); // Arctic - 北极冰雪主题，纯白冰蓝基底，清新冷冽
    static ThemeColors NeonTokyo(); // Neon Tokyo - 霓虹东京主题，深紫黑基底配合霓虹粉/青/绿
    static ThemeColors TraeDark(); // Trae Dark - Trae 暗色主题，深灰黑基底配合蓝/紫/绿
    static ThemeColors TraeDeepBlue(); // Trae Deep Blue - Trae 深蓝主题，深蓝灰基底配合蓝/青/紫
    static ThemeColors Midnight(); // Midnight - 午夜蓝主题，深邃神秘的蓝紫色调
    static ThemeColors
    FrancisBacon(); // Francis Bacon - 弗朗西斯·培根主题，深红黑基底配合血色/暗金色调
    static ThemeColors Moray(); // Moray - 莫莱主题，深海鳗鱼色，深蓝绿基底配合荧光绿/橙色
    static ThemeColors VanGogh(); // Van Gogh - 梵高主题，星空蓝黄基底配合漩涡状色彩
    static ThemeColors Minecraft();   // Minecraft - 我的世界主题，经典像素游戏配色
    static ThemeColors EVAUnit01();   // EVA Unit-01 - 初号机主题，紫/绿配色
    static ThemeColors EVAUnit02();   // EVA Unit-02 - 贰号机主题，红/橙配色
    static ThemeColors EVAUnit00();   // EVA Unit-00 - 零号机主题，黄/蓝配色
    static ThemeColors EVAMark06();   // EVA Mark.06 - 六号机主题，蓝/白配色
    static ThemeColors EVATerminal(); // EVA Terminal - NERV 终端机风格，绿黑配色
    static ThemeColors IronMan();     // Iron Man - 钢铁侠主题，红金配色 + 反应堆蓝光
    static ThemeColors SpiderMan();   // Spider-Man - 蜘蛛侠主题，红蓝配色 + 蛛网元素
    static ThemeColors CaptainAmerica(); // Captain America - 美国队长主题，红白蓝配色 + 星条旗元素
    static ThemeColors Hulk(); // Hulk - 绿巨人主题，绿色皮肤 + 愤怒力量 + 紫色裤子
    static ThemeColors Superman(); // Superman - 超人主题，红蓝配色 + 黄金 S 标志
    static ThemeColors Godfather(); // The Godfather - 教父主题，黑金配色 + 意大利黑手党风格
    static ThemeColors RoboCop(); // RoboCop - 机械战警主题，银色金属 + HUD 界面蓝 + 警示红
    static ThemeColors Robot();    // Robot - 机器人主题，金属银 + 科技蓝 + 电子绿
    static ThemeColors Icy();      // Icy - 冰蓝主题，灰色与浅蓝色配色，清新冷冽
    static ThemeColors PureBlue(); // Pure Blue - 纯净蓝白主题，纯白 + 天蓝极简配色
    static ThemeColors Silver(); // Silver - 银色主题，高端金属质感，银白 + 深灰 + 亮银点缀
    static ThemeColors Egypt(); // Egypt - 古埃及主题，沙漠金 + 尼罗河蓝 + 金字塔金 + 纸莎草棕
    static ThemeColors Oasis(); // Oasis - 绿洲主题，翡翠绿 + 金色 + 纯白 + 深蓝，中东绿洲风格
    static ThemeColors
    Aladdin(); // Aladdin - 阿拉丁主题，神灯金 + 魔法紫 + 宝石蓝 + 深空黑，一千零一夜奇幻风格
    static ThemeColors Denmark(); // Denmark - 丹麦主题，丹麦国旗红 + 纯白 + 北欧简约风 + 金色点缀
    static ThemeColors France(); // France - 法兰西主题，法国国旗蓝 + 白 + 红 + 薰衣草紫 + 香槟金
    static ThemeColors Antarctica(); // Antarctica - 南极主题，冰雪白 + 冰川蓝 + 极光绿 + 企鹅黑
    static ThemeColors Heaven(); // Heaven - 天堂主题，天蓝 + 白云 + 阳光金 + 天使白
    static ThemeColors Hell();   // Hell - 地狱主题，熔岩红 + 硫磺黄 + 炭黑 + 恶魔橙
    static ThemeColors DraculaSoft(); // Dracula Soft - 德古拉柔和版，降低对比度更护眼
    static ThemeColors DraculaLight(); // Dracula Light - 德古拉浅色版，明亮办公环境
    static ThemeColors DraculaNeon();  // Dracula Neon - 德古拉霓虹版，增强霓虹灯效果
    static ThemeColors DraculaPastel(); // Dracula Pastel - 德古拉粉彩版，柔和马卡龙色系
    static ThemeColors DraculaDeep(); // Dracula Deep - 德古拉深邃版，更深沉的暗夜风格
    static ThemeColors GreeceClassic(); // Greece Classic - 希腊经典版，经典蓝白配色
    static ThemeColors GreeceSunset(); // Greece Sunset - 希腊日落版，爱琴海日落暖色调
    static ThemeColors GreeceOlive();  // Greece Olive - 希腊橄榄版，橄榄绿 + 地中海蓝
    static ThemeColors GreeceMyth();   // Greece Myth - 希腊神话版，紫金神话配色
    static ThemeColors GreeceAegean(); // Greece Aegean - 希腊爱琴海版，深邃海洋蓝

    void setTheme(const std::string& name);

    // 从配置加载自定义主题
    bool loadCustomTheme(const std::string& name, const ThemeColors& colors);

    // 移除自定义主题
    bool removeCustomTheme(const std::string& name);

    // 清除所有自定义主题
    void clearCustomThemes();

    // 从颜色配置结构加载主题并注册为自定义主题
    bool loadThemeFromConfig(
        const std::string& name, const std::vector<int>& background,
        const std::vector<int>& foreground, const std::vector<int>& current_line,
        const std::vector<int>& selection, const std::vector<int>& line_number,
        const std::vector<int>& line_number_current, const std::vector<int>& statusbar_bg,
        const std::vector<int>& statusbar_fg, const std::vector<int>& menubar_bg,
        const std::vector<int>& menubar_fg, const std::vector<int>& helpbar_bg,
        const std::vector<int>& helpbar_fg, const std::vector<int>& helpbar_key,
        const std::vector<int>& keyword, const std::vector<int>& string,
        const std::vector<int>& comment, const std::vector<int>& number,
        const std::vector<int>& function, const std::vector<int>& type,
        const std::vector<int>& operator_color, const std::vector<int>& error,
        const std::vector<int>& warning, const std::vector<int>& info,
        const std::vector<int>& success, const std::vector<int>& dialog_bg,
        const std::vector<int>& dialog_fg, const std::vector<int>& dialog_title_bg,
        const std::vector<int>& dialog_title_fg, const std::vector<int>& dialog_border);

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
