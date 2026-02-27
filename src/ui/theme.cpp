#include "ui/theme.h"
#include <vector>

using namespace ftxui;

namespace pnana {
namespace ui {

Theme::Theme() : current_theme_("monokai") {
    colors_ = Monokai();
}

ThemeColors Theme::Monokai() {
    ThemeColors colors;
    // Monokai Classic: 深灰绿基底，粉/绿/青/橙标志色
    colors.background = Color::RGB(39, 40, 34);    // #272822
    colors.foreground = Color::RGB(248, 248, 242); // #f8f8f2
    colors.current_line = Color::RGB(55, 56, 48);  // #373831
    colors.selection = Color::RGB(73, 72, 62);     // #49483e
    colors.line_number = Color::RGB(117, 113, 94); // #75715e
    colors.line_number_current = Color::RGB(248, 248, 242);

    colors.statusbar_bg = Color::RGB(36, 37, 31);
    colors.statusbar_fg = Color::RGB(248, 248, 242);

    colors.menubar_bg = Color::RGB(30, 31, 28);
    colors.menubar_fg = Color::RGB(248, 248, 242);

    colors.helpbar_bg = Color::RGB(36, 37, 31);
    colors.helpbar_fg = Color::RGB(117, 113, 94);
    colors.helpbar_key = Color::RGB(166, 226, 46); // #a6e22e 绿

    // 经典 Monokai：粉红关键字/绿字符串/青类型/橙数字
    colors.keyword = Color::RGB(249, 38, 114);        // #f92672 粉红
    colors.string = Color::RGB(230, 219, 116);        // #e6db74 黄
    colors.comment = Color::RGB(117, 113, 94);        // #75715e 灰
    colors.number = Color::RGB(253, 151, 31);         // #fd971f 橙
    colors.function = Color::RGB(166, 226, 46);       // #a6e22e 绿
    colors.type = Color::RGB(102, 217, 239);          // #66d9ef 青
    colors.operator_color = Color::RGB(249, 38, 114); // 粉红

    colors.error = Color::RGB(249, 38, 114);   // #f92672
    colors.warning = Color::RGB(253, 151, 31); // #fd971f
    colors.info = Color::RGB(102, 217, 239);   // #66d9ef
    colors.success = Color::RGB(166, 226, 46); // #a6e22e

    colors.dialog_bg = Color::RGB(49, 50, 44);
    colors.dialog_fg = Color::RGB(248, 248, 242);
    colors.dialog_title_bg = Color::RGB(39, 40, 34);
    colors.dialog_title_fg = Color::RGB(248, 248, 242);
    colors.dialog_border = Color::RGB(117, 113, 94); // #75715e

    return colors;
}

ThemeColors Theme::Dracula() {
    ThemeColors colors;
    // Dracula 官方案例：紫灰基底，粉/紫/青/绿高饱和点缀
    colors.background = Color::RGB(40, 42, 54);    // #282a36
    colors.foreground = Color::RGB(248, 248, 242); // #f8f8f2
    colors.current_line = Color::RGB(52, 54, 70);  // #343652
    colors.selection = Color::RGB(68, 71, 90);     // #44475a
    colors.line_number = Color::RGB(98, 114, 164); // #6272a4
    colors.line_number_current = Color::RGB(248, 248, 242);

    colors.statusbar_bg = Color::RGB(68, 71, 90); // #44475a
    colors.statusbar_fg = Color::RGB(248, 248, 242);

    colors.menubar_bg = Color::RGB(33, 34, 44); // #21222c
    colors.menubar_fg = Color::RGB(248, 248, 242);

    colors.helpbar_bg = Color::RGB(68, 71, 90);
    colors.helpbar_fg = Color::RGB(189, 147, 249); // #bd93f9 紫
    colors.helpbar_key = Color::RGB(80, 250, 123); // #50fa7b 绿

    // Dracula 标志色：粉/紫/青/绿/橙
    colors.keyword = Color::RGB(255, 121, 198);        // #ff79c6 粉
    colors.string = Color::RGB(241, 250, 140);         // #f1fa8c 黄
    colors.comment = Color::RGB(98, 114, 164);         // #6272a4
    colors.number = Color::RGB(189, 147, 249);         // #bd93f9 紫
    colors.function = Color::RGB(80, 250, 123);        // #50fa7b 绿
    colors.type = Color::RGB(139, 233, 253);           // #8be9fd 青
    colors.operator_color = Color::RGB(255, 121, 198); // 粉

    colors.error = Color::RGB(255, 85, 85);     // #ff5555
    colors.warning = Color::RGB(255, 184, 108); // #ffb86c 橙
    colors.info = Color::RGB(139, 233, 253);    // #8be9fd
    colors.success = Color::RGB(80, 250, 123);  // #50fa7b

    colors.dialog_bg = Color::RGB(52, 54, 70);
    colors.dialog_fg = Color::RGB(248, 248, 242);
    colors.dialog_title_bg = Color::RGB(68, 71, 90);
    colors.dialog_title_fg = Color::RGB(248, 248, 242);
    colors.dialog_border = Color::RGB(98, 114, 164); // #6272a4

    return colors;
}

ThemeColors Theme::SolarizedDark() {
    ThemeColors colors;
    // Modernized Solarized Dark: classic blue-green theme with improved contrast
    colors.background = Color::RGB(10, 48, 60);
    colors.foreground = Color::RGB(140, 160, 165);
    colors.current_line = Color::RGB(20, 58, 74);
    colors.selection = Color::RGB(30, 68, 84);
    colors.line_number = Color::RGB(100, 125, 135);
    colors.line_number_current = Color::RGB(160, 175, 180);

    colors.statusbar_bg = Color::RGB(16, 54, 68);
    colors.statusbar_fg = Color::RGB(140, 160, 165);

    colors.menubar_bg = Color::RGB(10, 48, 60);
    colors.menubar_fg = Color::RGB(140, 160, 165);

    colors.helpbar_bg = Color::RGB(16, 54, 68);
    colors.helpbar_fg = Color::RGB(100, 125, 135);
    colors.helpbar_key = Color::RGB(140, 170, 30);

    colors.keyword = Color::RGB(210, 85, 35);
    colors.string = Color::RGB(50, 175, 165);
    colors.comment = Color::RGB(100, 125, 135);
    colors.number = Color::RGB(115, 120, 200);
    colors.function = Color::RGB(140, 170, 30);
    colors.type = Color::RGB(190, 150, 20);
    colors.operator_color = Color::RGB(45, 150, 220);

    colors.error = Color::RGB(225, 60, 55);
    colors.warning = Color::RGB(210, 85, 35);
    colors.info = Color::RGB(45, 150, 220);
    colors.success = Color::RGB(140, 170, 30);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(26, 64, 80);          // #1a4050 - 比背景稍亮的深蓝绿
    colors.dialog_fg = Color::RGB(140, 160, 165);       // #8ca0a5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(16, 54, 70);    // #103646 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(140, 160, 165); // #8ca0a5 - 与前景色一致
    colors.dialog_border = Color::RGB(110, 140, 155);   // #6e8c9b - 与行号颜色协调

    return colors;
}

ThemeColors Theme::SolarizedLight() {
    ThemeColors colors;
    // Modernized Solarized Light with clearer contrasts and softer accents
    colors.background = Color::RGB(250, 245, 235); // warm off-white
    colors.foreground = Color::RGB(95, 110, 115);  // softened foreground
    colors.current_line = Color::RGB(242, 238, 226);
    colors.selection = Color::RGB(235, 230, 215);
    colors.line_number = Color::RGB(140, 155, 155);
    colors.line_number_current = Color::RGB(90, 110, 115);

    colors.statusbar_bg = Color::RGB(240, 236, 222);
    colors.statusbar_fg = Color::RGB(95, 110, 115);

    colors.menubar_bg = Color::RGB(248, 244, 232);
    colors.menubar_fg = Color::RGB(95, 110, 115);

    colors.helpbar_bg = Color::RGB(240, 236, 222);
    colors.helpbar_fg = Color::RGB(145, 160, 160);
    colors.helpbar_key = Color::RGB(140, 165, 30);

    colors.keyword = Color::RGB(200, 80, 40);
    colors.string = Color::RGB(30, 150, 135);
    colors.comment = Color::RGB(145, 160, 160);
    colors.number = Color::RGB(115, 120, 190);
    colors.function = Color::RGB(120, 150, 30);
    colors.type = Color::RGB(180, 135, 30);
    colors.operator_color = Color::RGB(40, 120, 185);

    colors.error = Color::RGB(200, 50, 45);
    colors.warning = Color::RGB(200, 95, 30);
    colors.info = Color::RGB(40, 120, 185);
    colors.success = Color::RGB(120, 150, 30);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(242, 238, 226);       // #f2eee2 - 比背景稍深的暖白
    colors.dialog_fg = Color::RGB(95, 110, 115);        // #5f6e73 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(250, 245, 235); // #faf5eb - 比背景稍亮
    colors.dialog_title_fg = Color::RGB(95, 110, 115);  // #5f6e73 - 与前景色一致
    colors.dialog_border = Color::RGB(140, 155, 155);   // #8c9b9b - 与行号颜色协调

    return colors;
}

ThemeColors Theme::OneDark() {
    ThemeColors colors;
    // Modernized OneDark palette with slightly brighter accents
    colors.background = Color::RGB(38, 42, 52);
    colors.foreground = Color::RGB(200, 207, 220);
    colors.current_line = Color::RGB(50, 54, 66);
    colors.selection = Color::RGB(60, 64, 76);
    colors.line_number = Color::RGB(110, 120, 135);
    colors.line_number_current = Color::RGB(200, 207, 220);

    colors.statusbar_bg = Color::RGB(46, 50, 62);
    colors.statusbar_fg = Color::RGB(200, 207, 220);

    colors.menubar_bg = Color::RGB(44, 48, 60);
    colors.menubar_fg = Color::RGB(200, 207, 220);

    colors.helpbar_bg = Color::RGB(46, 50, 62);
    colors.helpbar_fg = Color::RGB(110, 120, 135);
    colors.helpbar_key = Color::RGB(170, 210, 140);

    colors.keyword = Color::RGB(210, 140, 230);
    colors.string = Color::RGB(160, 215, 130);
    colors.comment = Color::RGB(110, 120, 135);
    colors.number = Color::RGB(220, 170, 120);
    colors.function = Color::RGB(120, 200, 250);
    colors.type = Color::RGB(230, 200, 140);
    colors.operator_color = Color::RGB(110, 200, 210);

    colors.error = Color::RGB(230, 110, 130);
    colors.warning = Color::RGB(230, 200, 120);
    colors.info = Color::RGB(120, 200, 250);
    colors.success = Color::RGB(170, 210, 140);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(50, 54, 66);          // #323642 - 比背景稍亮的深蓝灰
    colors.dialog_fg = Color::RGB(200, 207, 220);       // #c8cfdc - 与前景色协调
    colors.dialog_title_bg = Color::RGB(38, 42, 52);    // #262a34 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(200, 207, 220); // #c8cfdc - 与前景色一致
    colors.dialog_border = Color::RGB(110, 120, 135);   // #6e7887 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Nord() {
    ThemeColors colors;
    // Modernized Nord: keep cool, icy tones with improved contrast
    colors.background = Color::RGB(40, 46, 58);
    colors.foreground = Color::RGB(220, 225, 230);
    colors.current_line = Color::RGB(58, 66, 78);
    colors.selection = Color::RGB(60, 68, 82);
    colors.line_number = Color::RGB(100, 114, 132);
    colors.line_number_current = Color::RGB(220, 225, 230);

    colors.statusbar_bg = Color::RGB(56, 64, 78);
    colors.statusbar_fg = Color::RGB(220, 225, 230);

    colors.menubar_bg = Color::RGB(40, 46, 58);
    colors.menubar_fg = Color::RGB(220, 225, 230);

    colors.helpbar_bg = Color::RGB(56, 64, 78);
    colors.helpbar_fg = Color::RGB(100, 114, 132);
    colors.helpbar_key = Color::RGB(170, 200, 140);

    colors.keyword = Color::RGB(150, 190, 220);
    colors.string = Color::RGB(170, 200, 150);
    colors.comment = Color::RGB(100, 114, 132);
    colors.number = Color::RGB(180, 150, 190);
    colors.function = Color::RGB(140, 200, 210);
    colors.type = Color::RGB(240, 210, 150);
    colors.operator_color = Color::RGB(150, 190, 220);

    colors.error = Color::RGB(210, 110, 120);
    colors.warning = Color::RGB(240, 200, 140);
    colors.info = Color::RGB(140, 200, 210);
    colors.success = Color::RGB(170, 200, 140);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(66, 74, 90);          // #424a5a - 比背景稍亮的冷蓝灰
    colors.dialog_fg = Color::RGB(220, 225, 230);       // #dce1e6 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(40, 46, 58);    // #282e3a - 比背景稍深
    colors.dialog_title_fg = Color::RGB(220, 225, 230); // #dce1e6 - 与前景色一致
    colors.dialog_border = Color::RGB(100, 114, 132);   // #647284 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Gruvbox() {
    ThemeColors colors;
    // Modernized Gruvbox-like palette with clearer contrast
    colors.background = Color::RGB(38, 38, 38);
    colors.foreground = Color::RGB(235, 220, 180);
    colors.current_line = Color::RGB(56, 52, 50);
    colors.selection = Color::RGB(56, 52, 50);
    colors.line_number = Color::RGB(120, 105, 95);
    colors.line_number_current = Color::RGB(235, 220, 180);

    colors.statusbar_bg = Color::RGB(48, 46, 45);
    colors.statusbar_fg = Color::RGB(235, 220, 180);

    colors.menubar_bg = Color::RGB(38, 38, 38);
    colors.menubar_fg = Color::RGB(235, 220, 180);

    colors.helpbar_bg = Color::RGB(48, 46, 45);
    colors.helpbar_fg = Color::RGB(120, 105, 95);
    colors.helpbar_key = Color::RGB(170, 160, 60);

    colors.keyword = Color::RGB(240, 110, 90);
    colors.string = Color::RGB(200, 190, 60);
    colors.comment = Color::RGB(140, 125, 110);
    colors.number = Color::RGB(210, 150, 160);
    colors.function = Color::RGB(240, 200, 80);
    colors.type = Color::RGB(140, 170, 155);
    colors.operator_color = Color::RGB(240, 110, 90);

    colors.error = Color::RGB(240, 110, 90);
    colors.warning = Color::RGB(240, 200, 80);
    colors.info = Color::RGB(140, 170, 155);
    colors.success = Color::RGB(200, 200, 80);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(56, 52, 50);          // #383432 - 比背景稍亮的暖灰
    colors.dialog_fg = Color::RGB(235, 220, 180);       // #ebdcb4 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(38, 38, 38);    // #262626 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 220, 180); // #ebdcb4 - 与前景色一致
    colors.dialog_border = Color::RGB(120, 105, 95);    // #78695f - 与行号颜色协调

    return colors;
}

ThemeColors Theme::TokyoNight() {
    ThemeColors colors;
    // Modern Tokyo Night: keep cool blue tones, increase legibility
    colors.background = Color::RGB(28, 30, 46);
    colors.foreground = Color::RGB(200, 210, 240);
    colors.current_line = Color::RGB(40, 44, 64);
    colors.selection = Color::RGB(42, 46, 66);
    colors.line_number = Color::RGB(100, 110, 140);
    colors.line_number_current = Color::RGB(200, 210, 240);

    colors.statusbar_bg = Color::RGB(42, 46, 66);
    colors.statusbar_fg = Color::RGB(200, 210, 240);

    colors.menubar_bg = Color::RGB(28, 30, 46);
    colors.menubar_fg = Color::RGB(200, 210, 240);

    colors.helpbar_bg = Color::RGB(42, 46, 66);
    colors.helpbar_fg = Color::RGB(100, 110, 140);
    colors.helpbar_key = Color::RGB(135, 215, 255);

    colors.keyword = Color::RGB(200, 160, 245);
    colors.string = Color::RGB(150, 210, 120);
    colors.comment = Color::RGB(100, 110, 140);
    colors.number = Color::RGB(255, 190, 110);
    colors.function = Color::RGB(135, 215, 255);
    colors.type = Color::RGB(130, 170, 245);
    colors.operator_color = Color::RGB(200, 160, 245);

    colors.error = Color::RGB(245, 120, 150);
    colors.warning = Color::RGB(255, 190, 110);
    colors.info = Color::RGB(135, 215, 255);
    colors.success = Color::RGB(150, 205, 115);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(42, 46, 66);          // #2a2e42 - 比背景稍亮的深蓝紫
    colors.dialog_fg = Color::RGB(200, 210, 240);       // #c8d2f0 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(28, 30, 46);    // #1c1e2e - 比背景稍深
    colors.dialog_title_fg = Color::RGB(200, 210, 240); // #c8d2f0 - 与前景色一致
    colors.dialog_border = Color::RGB(100, 110, 140);   // #646e8c - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Catppuccin() {
    ThemeColors colors;
    // Catppuccin Mocha: 暖色紫灰基底，柔和 pastel 点缀 (Rosewater/Mauve/Peach/Teal)
    colors.background = Color::RGB(30, 30, 46);             // #1e1e2e Base
    colors.foreground = Color::RGB(205, 214, 244);          // #cdd6f4 Text
    colors.current_line = Color::RGB(49, 50, 68);           // #313244 Surface0
    colors.selection = Color::RGB(69, 71, 90);              // #45475a Surface1
    colors.line_number = Color::RGB(108, 112, 134);         // #6c7086 Overlay0
    colors.line_number_current = Color::RGB(205, 214, 244); // #cdd6f4

    colors.statusbar_bg = Color::RGB(24, 24, 37); // #181825 Mantle
    colors.statusbar_fg = Color::RGB(205, 214, 244);

    colors.menubar_bg = Color::RGB(17, 17, 27); // #11111b Crust
    colors.menubar_fg = Color::RGB(205, 214, 244);

    colors.helpbar_bg = Color::RGB(24, 24, 37);
    colors.helpbar_fg = Color::RGB(166, 173, 222);  // #a6adc8 Overlay
    colors.helpbar_key = Color::RGB(148, 226, 213); // #94e2d5 Teal

    // Catppuccin 标志色：Mauve/Peach/Green/Teal/Sky
    colors.keyword = Color::RGB(203, 166, 247);        // #cba6f7 Mauve
    colors.string = Color::RGB(166, 227, 161);         // #a6e3a1 Green
    colors.comment = Color::RGB(108, 112, 134);        // #6c7086 Overlay0
    colors.number = Color::RGB(250, 179, 135);         // #fab387 Peach
    colors.function = Color::RGB(137, 220, 235);       // #89dceb Sky
    colors.type = Color::RGB(116, 199, 236);           // #74c7ec Sapphire
    colors.operator_color = Color::RGB(203, 166, 247); // Mauve

    colors.error = Color::RGB(243, 139, 168);   // #f38ba8 Red
    colors.warning = Color::RGB(250, 179, 135); // #fab387 Peach
    colors.info = Color::RGB(137, 220, 235);    // #89dceb Sky
    colors.success = Color::RGB(166, 227, 161); // #a6e3a1 Green

    colors.dialog_bg = Color::RGB(49, 50, 68); // #313244
    colors.dialog_fg = Color::RGB(205, 214, 244);
    colors.dialog_title_bg = Color::RGB(55, 59, 78); // #373b47 Surface2
    colors.dialog_title_fg = Color::RGB(205, 214, 244);
    colors.dialog_border = Color::RGB(88, 91, 112); // #585b70 Surface2

    return colors;
}

ThemeColors Theme::Material() {
    ThemeColors colors;
    // Material Oceanic: 经典深蓝灰基底 + 青/紫/橙点缀
    colors.background = Color::RGB(38, 50, 56);             // #263238
    colors.foreground = Color::RGB(176, 190, 197);          // #B0BEC5
    colors.current_line = Color::RGB(49, 62, 69);           // #314549 Active
    colors.selection = Color::RGB(84, 110, 122);            // #546E7A
    colors.line_number = Color::RGB(96, 125, 139);          // #607D8B Text
    colors.line_number_current = Color::RGB(238, 255, 255); // #eeffff

    colors.statusbar_bg = Color::RGB(50, 60, 67); // #32424A
    colors.statusbar_fg = Color::RGB(176, 190, 197);

    colors.menubar_bg = Color::RGB(30, 39, 44); // #1E272C Contrast
    colors.menubar_fg = Color::RGB(176, 190, 197);

    colors.helpbar_bg = Color::RGB(50, 60, 67);
    colors.helpbar_fg = Color::RGB(96, 125, 139);
    colors.helpbar_key = Color::RGB(0, 150, 136); // #009688 Accent 青绿

    // Material Oceanic 语法色：紫关键词/绿字符串/蓝函数/橙数字/青运算符
    colors.keyword = Color::RGB(199, 146, 234);        // #c792ea Purple
    colors.string = Color::RGB(195, 232, 141);         // #c3e88d Green
    colors.comment = Color::RGB(84, 110, 122);         // #546e7a
    colors.number = Color::RGB(247, 140, 108);         // #f78c6c Orange
    colors.function = Color::RGB(130, 170, 255);       // #82aaff Blue
    colors.type = Color::RGB(255, 203, 107);           // #ffcb6b Yellow
    colors.operator_color = Color::RGB(137, 221, 255); // #89ddff Cyan

    colors.error = Color::RGB(255, 83, 112);    // #ff5370
    colors.warning = Color::RGB(247, 140, 108); // #f78c6c Orange
    colors.info = Color::RGB(130, 170, 255);    // #82aaff
    colors.success = Color::RGB(195, 232, 141); // #c3e88d

    colors.dialog_bg = Color::RGB(50, 60, 67); // #32424A
    colors.dialog_fg = Color::RGB(176, 190, 197);
    colors.dialog_title_bg = Color::RGB(42, 55, 62); // #2A373E Border
    colors.dialog_title_fg = Color::RGB(238, 255, 255);
    colors.dialog_border = Color::RGB(66, 91, 103); // #425B67 Highlight

    return colors;
}

ThemeColors Theme::Ayu() {
    ThemeColors colors;
    // Modernized Ayu: deep blue base with bright accents, improved contrast
    colors.background = Color::RGB(20, 28, 52);
    colors.foreground = Color::RGB(230, 235, 245);
    colors.current_line = Color::RGB(32, 42, 68);
    colors.selection = Color::RGB(42, 54, 82);
    colors.line_number = Color::RGB(120, 135, 155);
    colors.line_number_current = Color::RGB(230, 235, 245);

    colors.statusbar_bg = Color::RGB(16, 24, 48);
    colors.statusbar_fg = Color::RGB(230, 235, 245);

    colors.menubar_bg = Color::RGB(20, 28, 52);
    colors.menubar_fg = Color::RGB(230, 235, 245);

    colors.helpbar_bg = Color::RGB(16, 24, 48);
    colors.helpbar_fg = Color::RGB(120, 135, 155);
    colors.helpbar_key = Color::RGB(50, 210, 120);

    colors.keyword = Color::RGB(160, 110, 240);
    colors.string = Color::RGB(50, 210, 120);
    colors.comment = Color::RGB(120, 135, 155);
    colors.number = Color::RGB(250, 160, 80);
    colors.function = Color::RGB(80, 150, 250);
    colors.type = Color::RGB(250, 200, 60);
    colors.operator_color = Color::RGB(160, 110, 240);

    colors.error = Color::RGB(240, 85, 85);
    colors.warning = Color::RGB(250, 160, 80);
    colors.info = Color::RGB(80, 150, 250);
    colors.success = Color::RGB(50, 210, 120);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(36, 46, 72);          // #242e48 - 比背景稍亮的深蓝
    colors.dialog_fg = Color::RGB(230, 235, 245);       // #e6ebf5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(24, 32, 56);    // #182038 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(230, 235, 245); // #e6ebf5 - 与前景色一致
    colors.dialog_border = Color::RGB(120, 135, 155);   // #78879b - 与行号颜色协调

    return colors;
}

ThemeColors Theme::GitHub() {
    ThemeColors colors;
    // Optimized GitHub Light theme with improved contrast
    colors.background = Color::RGB(255, 255, 255);       // #ffffff
    colors.foreground = Color::RGB(36, 41, 46);          // #24292e
    colors.current_line = Color::RGB(246, 248, 250);     // #f6f8fa
    colors.selection = Color::RGB(200, 225, 255);        // #c8e1ff
    colors.line_number = Color::RGB(106, 115, 125);      // #6a737d
    colors.line_number_current = Color::RGB(36, 41, 46); // #24292e

    colors.statusbar_bg = Color::RGB(246, 248, 250); // #f6f8fa
    colors.statusbar_fg = Color::RGB(36, 41, 46);    // #24292e

    colors.menubar_bg = Color::RGB(255, 255, 255); // #ffffff
    colors.menubar_fg = Color::RGB(36, 41, 46);    // #24292e

    colors.helpbar_bg = Color::RGB(246, 248, 250); // #f6f8fa
    colors.helpbar_fg = Color::RGB(106, 115, 125); // #6a737d
    colors.helpbar_key = Color::RGB(34, 134, 58);  // #22863a

    colors.keyword = Color::RGB(111, 66, 193);      // #6f42c1
    colors.string = Color::RGB(0, 92, 197);         // #005cc5
    colors.comment = Color::RGB(106, 115, 125);     // #6a737d
    colors.number = Color::RGB(0, 92, 197);         // #005cc5
    colors.function = Color::RGB(111, 66, 193);     // #6f42c1
    colors.type = Color::RGB(34, 134, 58);          // #22863a
    colors.operator_color = Color::RGB(36, 41, 46); // #24292e

    colors.error = Color::RGB(203, 36, 49);   // #cb2431
    colors.warning = Color::RGB(219, 171, 9); // #dbab09
    colors.info = Color::RGB(0, 92, 197);     // #005cc5
    colors.success = Color::RGB(34, 134, 58); // #22863a

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(240, 242, 245);       // #f0f2f5 - 比背景稍深的浅灰
    colors.dialog_fg = Color::RGB(45, 50, 55);          // #2d3237 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(250, 250, 250); // #fafafa - 比背景稍亮
    colors.dialog_title_fg = Color::RGB(45, 50, 55);    // #2d3237 - 与前景色一致
    colors.dialog_border = Color::RGB(110, 120, 130);   // #6e7882 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::GitHubDark() {
    ThemeColors colors;
    // Optimized GitHub Dark theme with improved contrast and readability
    colors.background = Color::RGB(13, 17, 23);             // #0d1117 - GitHub dark background
    colors.foreground = Color::RGB(230, 237, 243);          // #e6edf3 - GitHub dark foreground
    colors.current_line = Color::RGB(22, 27, 34);           // #161b22 - Current line highlight
    colors.selection = Color::RGB(33, 38, 45);              // #21262d - Selection background
    colors.line_number = Color::RGB(139, 148, 158);         // #8b949e - Line numbers
    colors.line_number_current = Color::RGB(230, 237, 243); // #e6edf3 - Current line number

    colors.statusbar_bg = Color::RGB(22, 27, 34);    // #161b22
    colors.statusbar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.menubar_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.menubar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.helpbar_bg = Color::RGB(22, 27, 34);    // #161b22
    colors.helpbar_fg = Color::RGB(139, 148, 158); // #8b949e
    colors.helpbar_key = Color::RGB(56, 178, 172); // #38b2ac - Teal accent

    colors.keyword = Color::RGB(255, 123, 172);        // #ff7bac - Pink for keywords
    colors.string = Color::RGB(163, 186, 202);         // #a3bac8 - Light blue for strings
    colors.comment = Color::RGB(139, 148, 158);        // #8b949e - Gray for comments
    colors.number = Color::RGB(121, 192, 255);         // #79c0ff - Blue for numbers
    colors.function = Color::RGB(210, 168, 255);       // #d2a8ff - Purple for functions
    colors.type = Color::RGB(56, 178, 172);            // #38b2ac - Teal for types
    colors.operator_color = Color::RGB(255, 123, 172); // #ff7bac - Pink for operators

    colors.error = Color::RGB(248, 81, 73);    // #f85149 - GitHub red
    colors.warning = Color::RGB(255, 188, 33); // #ffbc21 - GitHub yellow
    colors.info = Color::RGB(121, 192, 255);   // #79c0ff - GitHub blue
    colors.success = Color::RGB(56, 178, 172); // #38b2ac - GitHub teal

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(22, 27, 34);          // #161b22 - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(230, 237, 243);       // #e6edf3 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(13, 17, 23);    // #0d1117 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(230, 237, 243); // #e6edf3 - 与前景色一致
    colors.dialog_border = Color::RGB(139, 148, 158);   // #8b949e - 与行号颜色协调

    return colors;
}

ThemeColors Theme::MarkdownDark() {
    ThemeColors colors;
    // Markdown Dark theme optimized with gray tones and colors matching code editor style
    // Deep dark gray background for comfortable viewing
    colors.background = Color::RGB(25, 25, 28);     // #19191c - Deep dark gray background
    colors.foreground = Color::RGB(220, 220, 220);  // #dcdcdc - Soft white foreground
    colors.current_line = Color::RGB(35, 35, 38);   // #232326 - Subtle current line highlight
    colors.selection = Color::RGB(45, 50, 55);      // #2d3237 - Selection highlight
    colors.line_number = Color::RGB(100, 105, 110); // #64696e - Muted gray line numbers
    colors.line_number_current = Color::RGB(220, 220, 220); // #dcdcdc - Current line number

    colors.statusbar_bg = Color::RGB(30, 30, 33);    // #1e1e21
    colors.statusbar_fg = Color::RGB(220, 220, 220); // #dcdcdc

    colors.menubar_bg = Color::RGB(25, 25, 28);    // #19191c
    colors.menubar_fg = Color::RGB(220, 220, 220); // #dcdcdc

    colors.helpbar_bg = Color::RGB(30, 30, 33);     // #1e1e21
    colors.helpbar_fg = Color::RGB(140, 150, 160);  // #8c96a0 - Light grayish
    colors.helpbar_key = Color::RGB(220, 180, 120); // #dcb478 - Yellow/orange accent

    // Color scheme matching code editor style from image
    colors.keyword = Color::RGB(200, 120, 220);        // #c878dc - Purple for keywords/headers
    colors.string = Color::RGB(150, 200, 150);         // #96c896 - Green for strings/links
    colors.comment = Color::RGB(140, 150, 160);        // #8c96a0 - Light greenish-gray for comments
    colors.number = Color::RGB(120, 180, 220);         // #78b4dc - Light blue for numbers
    colors.function = Color::RGB(220, 180, 120);       // #dcb478 - Yellow/orange for functions
    colors.type = Color::RGB(120, 180, 220);           // #78b4dc - Light blue for types
    colors.operator_color = Color::RGB(200, 120, 220); // #c878dc - Purple for operators

    colors.error = Color::RGB(240, 100, 100);   // #f06464 - Soft red
    colors.warning = Color::RGB(240, 200, 100); // #f0c864 - Soft yellow
    colors.info = Color::RGB(120, 180, 220);    // #78b4dc - Light blue
    colors.success = Color::RGB(150, 200, 150); // #96c896 - Green

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(35, 35, 38);          // #232326 - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(220, 220, 220);       // #dcdcdc - 与前景色协调
    colors.dialog_title_bg = Color::RGB(25, 25, 28);    // #19191c - 比背景稍深
    colors.dialog_title_fg = Color::RGB(220, 220, 220); // #dcdcdc - 与前景色一致
    colors.dialog_border = Color::RGB(100, 105, 110);   // #64696e - 与行号颜色协调

    return colors;
}

ThemeColors Theme::VSCodeDark() {
    ThemeColors colors;
    // Modernized VSCode Dark palette (cleaner and slightly brighter accents)
    colors.background = Color::RGB(28, 28, 30);
    colors.foreground = Color::RGB(220, 220, 220);
    colors.current_line = Color::RGB(44, 44, 46);
    colors.selection = Color::RGB(60, 100, 150);
    colors.line_number = Color::RGB(140, 140, 140);
    colors.line_number_current = Color::RGB(220, 220, 220);

    colors.statusbar_bg = Color::RGB(0, 120, 200);
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(36, 36, 38);
    colors.menubar_fg = Color::RGB(210, 210, 210);

    colors.helpbar_bg = Color::RGB(36, 36, 38);
    colors.helpbar_fg = Color::RGB(140, 140, 140);
    colors.helpbar_key = Color::RGB(90, 160, 220);

    colors.keyword = Color::RGB(100, 150, 230);
    colors.string = Color::RGB(210, 150, 120);
    colors.comment = Color::RGB(130, 160, 110);
    colors.number = Color::RGB(190, 200, 160);
    colors.function = Color::RGB(230, 230, 190);
    colors.type = Color::RGB(90, 200, 170);
    colors.operator_color = Color::RGB(210, 210, 210);

    colors.error = Color::RGB(240, 80, 110);
    colors.warning = Color::RGB(255, 185, 100);
    colors.info = Color::RGB(90, 160, 220);
    colors.success = Color::RGB(110, 160, 110);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(44, 44, 46);          // #2c2c2e - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(220, 220, 220);       // #dcdcdc - 与前景色协调
    colors.dialog_title_bg = Color::RGB(28, 28, 30);    // #1c1c1e - 比背景稍深
    colors.dialog_title_fg = Color::RGB(220, 220, 220); // #dcdcdc - 与前景色一致
    colors.dialog_border = Color::RGB(140, 140, 140);   // #8c8c8c - 与行号颜色协调

    return colors;
}

ThemeColors Theme::NightOwl() {
    ThemeColors colors;
    // Night Owl (Sarah Drasner): 深蓝黑基底 + 紫/金/青冷调，夜间海洋感
    colors.background = Color::RGB(1, 22, 39);              // #011627 - 极深蓝
    colors.foreground = Color::RGB(214, 222, 235);          // #d6deeb
    colors.current_line = Color::RGB(11, 37, 58);           // #0b2942 - 微亮蓝
    colors.selection = Color::RGB(29, 59, 83);              // #1d3b53
    colors.line_number = Color::RGB(75, 100, 121);          // #4b6479
    colors.line_number_current = Color::RGB(197, 228, 253); // #C5E4FD 亮青

    colors.statusbar_bg = Color::RGB(1, 22, 39);
    colors.statusbar_fg = Color::RGB(95, 126, 151); // #5f7e97

    colors.menubar_bg = Color::RGB(1, 11, 29); // #010e1a
    colors.menubar_fg = Color::RGB(214, 222, 235);

    colors.helpbar_bg = Color::RGB(1, 22, 39);
    colors.helpbar_fg = Color::RGB(95, 126, 151);
    colors.helpbar_key = Color::RGB(130, 170, 255); // #82AAFF 亮蓝

    // Night Owl 标志色：紫关键字/暖橙数字/青绿变量/沙色字符串
    colors.keyword = Color::RGB(199, 146, 234);        // #c792ea 紫色
    colors.string = Color::RGB(236, 196, 141);         // #ecc48d 暖沙色
    colors.comment = Color::RGB(99, 119, 119);         // #637777 青灰
    colors.number = Color::RGB(247, 140, 108);         // #F78C6C 珊瑚橙
    colors.function = Color::RGB(199, 146, 234);       // #c792ea 紫 (函数名)
    colors.type = Color::RGB(130, 170, 255);           // #82AAFF 亮蓝
    colors.operator_color = Color::RGB(199, 146, 234); // 紫

    colors.error = Color::RGB(239, 83, 80);     // #EF5350
    colors.warning = Color::RGB(179, 149, 84);  // #b39554 暖棕
    colors.info = Color::RGB(130, 170, 255);    // #82AAFF
    colors.success = Color::RGB(197, 228, 120); // #c5e478 青柠

    colors.dialog_bg = Color::RGB(2, 19, 32); // #021320
    colors.dialog_fg = Color::RGB(214, 222, 235);
    colors.dialog_title_bg = Color::RGB(18, 45, 66); // #122d42
    colors.dialog_title_fg = Color::RGB(214, 222, 235);
    colors.dialog_border = Color::RGB(95, 126, 151); // #5f7e97

    return colors;
}

ThemeColors Theme::Palenight() {
    ThemeColors colors;
    // Modernized Palenight: keep soft purple base but improve contrast
    colors.background = Color::RGB(42, 44, 62);
    colors.foreground = Color::RGB(215, 215, 225);
    colors.current_line = Color::RGB(36, 38, 54);
    colors.selection = Color::RGB(52, 54, 72);
    colors.line_number = Color::RGB(100, 104, 127);
    colors.line_number_current = Color::RGB(215, 215, 225);

    colors.statusbar_bg = Color::RGB(38, 40, 56);
    colors.statusbar_fg = Color::RGB(215, 215, 225);

    colors.menubar_bg = Color::RGB(42, 44, 62);
    colors.menubar_fg = Color::RGB(215, 215, 225);

    colors.helpbar_bg = Color::RGB(38, 40, 56);
    colors.helpbar_fg = Color::RGB(100, 104, 127);
    colors.helpbar_key = Color::RGB(200, 235, 160);

    colors.keyword = Color::RGB(210, 160, 235);
    colors.string = Color::RGB(200, 235, 160);
    colors.comment = Color::RGB(100, 104, 127);
    colors.number = Color::RGB(250, 165, 130);
    colors.function = Color::RGB(110, 170, 255);
    colors.type = Color::RGB(250, 200, 120);
    colors.operator_color = Color::RGB(210, 160, 235);

    colors.error = Color::RGB(250, 100, 140);
    colors.warning = Color::RGB(250, 200, 120);
    colors.info = Color::RGB(110, 170, 255);
    colors.success = Color::RGB(200, 235, 160);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(42, 44, 62);          // #2a2c3e - 比背景稍亮的紫灰
    colors.dialog_fg = Color::RGB(215, 215, 225);       // #d7d7e1 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(36, 38, 54);    // #242636 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(215, 215, 225); // #d7d7e1 - 与前景色一致
    colors.dialog_border = Color::RGB(100, 104, 127);   // #64687f - 与行号颜色协调

    return colors;
}

ThemeColors Theme::OceanicNext() {
    ThemeColors colors;
    // Oceanic Next 官方案例：深海洋蓝基底 + 青/紫/绿/橙
    colors.background = Color::RGB(27, 43, 52);     // #1B2B34
    colors.foreground = Color::RGB(205, 211, 222);  // #CDD3DE
    colors.current_line = Color::RGB(52, 61, 70);   // #343d46
    colors.selection = Color::RGB(79, 91, 102);     // #4f5b66
    colors.line_number = Color::RGB(101, 115, 126); // #65737e
    colors.line_number_current = Color::RGB(205, 211, 222);

    colors.statusbar_bg = Color::RGB(52, 61, 70); // #343d46
    colors.statusbar_fg = Color::RGB(205, 211, 222);

    colors.menubar_bg = Color::RGB(27, 43, 52);
    colors.menubar_fg = Color::RGB(205, 211, 222);

    colors.helpbar_bg = Color::RGB(52, 61, 70);
    colors.helpbar_fg = Color::RGB(101, 115, 126);
    colors.helpbar_key = Color::RGB(95, 179, 179); // #5FB3B3 青

    // Oceanic Next 语法：紫关键词/绿字符串/蓝函数/橙数字/青运算符
    colors.keyword = Color::RGB(197, 148, 197);       // #C594C5
    colors.string = Color::RGB(153, 199, 148);        // #99C794
    colors.comment = Color::RGB(101, 115, 126);       // #65737e
    colors.number = Color::RGB(249, 145, 87);         // #F99157
    colors.function = Color::RGB(102, 153, 204);      // #6699CC
    colors.type = Color::RGB(250, 200, 99);           // #FAC863
    colors.operator_color = Color::RGB(95, 179, 179); // #5FB3B3

    colors.error = Color::RGB(235, 96, 107);    // #EB606B
    colors.warning = Color::RGB(249, 145, 87);  // #F99157
    colors.info = Color::RGB(102, 153, 204);    // #6699CC
    colors.success = Color::RGB(153, 199, 148); // #99C794

    colors.dialog_bg = Color::RGB(52, 61, 70);
    colors.dialog_fg = Color::RGB(205, 211, 222);
    colors.dialog_title_bg = Color::RGB(79, 91, 102);
    colors.dialog_title_fg = Color::RGB(205, 211, 222);
    colors.dialog_border = Color::RGB(101, 115, 126);

    return colors;
}

ThemeColors Theme::Kanagawa() {
    ThemeColors colors;
    // Kanagawa Wave 官方案例：墨色基底 + 藤白/春绿/柿青
    colors.background = Color::RGB(24, 24, 32);     // #181820 sumiInk1
    colors.foreground = Color::RGB(220, 215, 186);  // #DCD7BA fujiWhite
    colors.current_line = Color::RGB(34, 50, 73);   // #223249 waveBlue1
    colors.selection = Color::RGB(45, 79, 103);     // #2D4F67 waveBlue2
    colors.line_number = Color::RGB(114, 113, 105); // #727169 fujiGray
    colors.line_number_current = Color::RGB(220, 215, 186);

    colors.statusbar_bg = Color::RGB(31, 31, 40); // #1F1F28 sumiInk3
    colors.statusbar_fg = Color::RGB(220, 215, 186);

    colors.menubar_bg = Color::RGB(24, 24, 32);
    colors.menubar_fg = Color::RGB(220, 215, 186);

    colors.helpbar_bg = Color::RGB(31, 31, 40);
    colors.helpbar_fg = Color::RGB(114, 113, 105);
    colors.helpbar_key = Color::RGB(127, 180, 202); // #7FB4CA springBlue

    // Kanagawa 语法：鬼紫关键词/春绿字符串/澄蓝函数/秋黄数字/波青运算符
    colors.keyword = Color::RGB(149, 127, 184);        // #957FB8 oniViolet
    colors.string = Color::RGB(152, 187, 108);         // #98BB6C springGreen
    colors.comment = Color::RGB(114, 113, 105);        // #727169 fujiGray
    colors.number = Color::RGB(220, 165, 97);          // #DCA561 autumnYellow
    colors.function = Color::RGB(126, 156, 216);       // #7E9CD8 crystalBlue
    colors.type = Color::RGB(127, 180, 202);           // #7FB4CA springBlue
    colors.operator_color = Color::RGB(122, 168, 159); // #7AA89F waveAqua2

    colors.error = Color::RGB(232, 36, 36);     // #E82424 samuraiRed
    colors.warning = Color::RGB(255, 158, 59);  // #FF9E3B roninYellow
    colors.info = Color::RGB(126, 156, 216);    // #7E9CD8 crystalBlue
    colors.success = Color::RGB(118, 148, 106); // #76946A autumnGreen

    colors.dialog_bg = Color::RGB(42, 42, 55); // #2A2A37 sumiInk4
    colors.dialog_fg = Color::RGB(220, 215, 186);
    colors.dialog_title_bg = Color::RGB(34, 50, 73);
    colors.dialog_title_fg = Color::RGB(220, 215, 186);
    colors.dialog_border = Color::RGB(114, 113, 105);

    return colors;
}

ThemeColors Theme::TomorrowNight() {
    ThemeColors colors;
    // Modernized Tomorrow Night: neutral dark with soft warm accents
    colors.background = Color::RGB(34, 36, 38);
    colors.foreground = Color::RGB(210, 212, 210);
    colors.current_line = Color::RGB(44, 46, 48);
    colors.selection = Color::RGB(58, 62, 66);
    colors.line_number = Color::RGB(140, 142, 140);
    colors.line_number_current = Color::RGB(210, 212, 210);

    colors.statusbar_bg = Color::RGB(36, 38, 40);
    colors.statusbar_fg = Color::RGB(210, 212, 210);

    colors.menubar_bg = Color::RGB(34, 36, 38);
    colors.menubar_fg = Color::RGB(210, 212, 210);

    colors.helpbar_bg = Color::RGB(36, 38, 40);
    colors.helpbar_fg = Color::RGB(140, 142, 140);
    colors.helpbar_key = Color::RGB(190, 200, 110);

    colors.keyword = Color::RGB(220, 110, 110);
    colors.string = Color::RGB(190, 200, 110);
    colors.comment = Color::RGB(140, 142, 140);
    colors.number = Color::RGB(190, 150, 200);
    colors.function = Color::RGB(240, 210, 120);
    colors.type = Color::RGB(110, 160, 200);
    colors.operator_color = Color::RGB(220, 110, 110);

    colors.error = Color::RGB(220, 110, 110);
    colors.warning = Color::RGB(240, 210, 120);
    colors.info = Color::RGB(110, 160, 200);
    colors.success = Color::RGB(190, 200, 110);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(46, 48, 52);          // #2e3034 - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(210, 212, 210);       // #d2d4d2 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(34, 36, 38);    // #222426 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(210, 212, 210); // #d2d4d2 - 与前景色一致
    colors.dialog_border = Color::RGB(140, 142, 140);   // #8c8e8c - 与行号颜色协调

    return colors;
}

ThemeColors Theme::TomorrowNightBlue() {
    ThemeColors colors;
    // Modernized Tomorrow Night Blue: deep blue base with bright, vibrant accents
    colors.background = Color::RGB(15, 40, 85);
    colors.foreground = Color::RGB(235, 240, 245);
    colors.current_line = Color::RGB(25, 50, 110);
    colors.selection = Color::RGB(35, 60, 135);
    colors.line_number = Color::RGB(140, 160, 185);
    colors.line_number_current = Color::RGB(235, 240, 245);

    colors.statusbar_bg = Color::RGB(12, 35, 80);
    colors.statusbar_fg = Color::RGB(235, 240, 245);

    colors.menubar_bg = Color::RGB(15, 40, 85);
    colors.menubar_fg = Color::RGB(235, 240, 245);

    colors.helpbar_bg = Color::RGB(12, 35, 80);
    colors.helpbar_fg = Color::RGB(140, 160, 185);
    colors.helpbar_key = Color::RGB(250, 210, 130);

    colors.keyword = Color::RGB(250, 120, 235);
    colors.string = Color::RGB(250, 210, 130);
    colors.comment = Color::RGB(140, 160, 185);
    colors.number = Color::RGB(250, 120, 235);
    colors.function = Color::RGB(250, 210, 130);
    colors.type = Color::RGB(140, 220, 210);
    colors.operator_color = Color::RGB(250, 120, 235);

    colors.error = Color::RGB(250, 80, 140);
    colors.warning = Color::RGB(250, 210, 130);
    colors.info = Color::RGB(140, 220, 210);
    colors.success = Color::RGB(200, 250, 50);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(30, 55, 115);         // #1e3773 - 比背景稍亮的深蓝
    colors.dialog_fg = Color::RGB(235, 240, 245);       // #ebf0f5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(20, 45, 100);   // #142d64 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 240, 245); // #ebf0f5 - 与前景色一致
    colors.dialog_border = Color::RGB(140, 160, 185);   // #8ca0b9

    return colors;
}

ThemeColors Theme::Cobalt() {
    ThemeColors colors;
    // Modernized Cobalt: deep navy base with bright, contrasting accents
    colors.background = Color::RGB(10, 35, 70);
    colors.foreground = Color::RGB(240, 245, 250);
    colors.current_line = Color::RGB(20, 45, 95);
    colors.selection = Color::RGB(30, 55, 115);
    colors.line_number = Color::RGB(135, 165, 195);
    colors.line_number_current = Color::RGB(240, 245, 250);

    colors.statusbar_bg = Color::RGB(8, 30, 65);
    colors.statusbar_fg = Color::RGB(240, 245, 250);

    colors.menubar_bg = Color::RGB(10, 35, 70);
    colors.menubar_fg = Color::RGB(240, 245, 250);

    colors.helpbar_bg = Color::RGB(8, 30, 65);
    colors.helpbar_fg = Color::RGB(135, 165, 195);
    colors.helpbar_key = Color::RGB(250, 210, 130);

    colors.keyword = Color::RGB(250, 170, 30);
    colors.string = Color::RGB(80, 170, 130);
    colors.comment = Color::RGB(135, 165, 195);
    colors.number = Color::RGB(250, 210, 130);
    colors.function = Color::RGB(250, 210, 130);
    colors.type = Color::RGB(250, 170, 30);
    colors.operator_color = Color::RGB(250, 170, 30);

    colors.error = Color::RGB(250, 80, 140);
    colors.warning = Color::RGB(250, 210, 130);
    colors.info = Color::RGB(140, 220, 210);
    colors.success = Color::RGB(80, 170, 130);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(25, 50, 100);         // #193264 - 比背景稍亮的深蓝
    colors.dialog_fg = Color::RGB(240, 245, 250);       // #f0f5fa - 与前景色协调
    colors.dialog_title_bg = Color::RGB(15, 40, 85);    // #0f2855 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(240, 245, 250); // #f0f5fa - 与前景色一致
    colors.dialog_border = Color::RGB(135, 165, 195);   // #87a5c3

    return colors;
}

ThemeColors Theme::Zenburn() {
    ThemeColors colors;
    // Modernized Zenburn: warm gray base with subtle yellow/cream accents
    colors.background = Color::RGB(45, 45, 45);
    colors.foreground = Color::RGB(235, 235, 220);
    colors.current_line = Color::RGB(60, 60, 60);
    colors.selection = Color::RGB(70, 70, 70);
    colors.line_number = Color::RGB(120, 120, 120);
    colors.line_number_current = Color::RGB(235, 235, 220);

    colors.statusbar_bg = Color::RGB(42, 42, 42);
    colors.statusbar_fg = Color::RGB(235, 235, 220);

    colors.menubar_bg = Color::RGB(45, 45, 45);
    colors.menubar_fg = Color::RGB(235, 235, 220);

    colors.helpbar_bg = Color::RGB(42, 42, 42);
    colors.helpbar_fg = Color::RGB(120, 120, 120);
    colors.helpbar_key = Color::RGB(220, 170, 170);

    colors.keyword = Color::RGB(240, 225, 180);
    colors.string = Color::RGB(220, 170, 170);
    colors.comment = Color::RGB(130, 165, 130);
    colors.number = Color::RGB(235, 235, 220);
    colors.function = Color::RGB(100, 185, 220);
    colors.type = Color::RGB(225, 185, 155);
    colors.operator_color = Color::RGB(240, 225, 180);

    colors.error = Color::RGB(210, 110, 110);
    colors.warning = Color::RGB(225, 185, 155);
    colors.info = Color::RGB(100, 185, 220);
    colors.success = Color::RGB(130, 165, 130);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(65, 65, 65);          // #414141 - 比背景稍亮的暖灰
    colors.dialog_fg = Color::RGB(235, 235, 220);       // #ebebdd - 与前景色协调
    colors.dialog_title_bg = Color::RGB(50, 50, 50);    // #323232 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 235, 220); // #ebebdd - 与前景色一致
    colors.dialog_border = Color::RGB(125, 125, 125);   // #7d7d7d - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Base16Dark() {
    ThemeColors colors;
    // Modernized Base16 Dark: neutral gray base with vibrant, contrasting accents
    colors.background = Color::RGB(25, 25, 25);
    colors.foreground = Color::RGB(220, 220, 220);
    colors.current_line = Color::RGB(45, 45, 45);
    colors.selection = Color::RGB(55, 55, 55);
    colors.line_number = Color::RGB(90, 90, 90);
    colors.line_number_current = Color::RGB(220, 220, 220);

    colors.statusbar_bg = Color::RGB(30, 30, 30);
    colors.statusbar_fg = Color::RGB(220, 220, 220);

    colors.menubar_bg = Color::RGB(25, 25, 25);
    colors.menubar_fg = Color::RGB(220, 220, 220);

    colors.helpbar_bg = Color::RGB(30, 30, 30);
    colors.helpbar_fg = Color::RGB(90, 90, 90);
    colors.helpbar_key = Color::RGB(190, 150, 15);

    colors.keyword = Color::RGB(220, 65, 140);
    colors.string = Color::RGB(55, 175, 165);
    colors.comment = Color::RGB(100, 100, 100);
    colors.number = Color::RGB(120, 125, 205);
    colors.function = Color::RGB(50, 155, 220);
    colors.type = Color::RGB(190, 150, 15);
    colors.operator_color = Color::RGB(220, 65, 140);

    colors.error = Color::RGB(225, 60, 55);
    colors.warning = Color::RGB(210, 85, 35);
    colors.info = Color::RGB(50, 155, 220);
    colors.success = Color::RGB(140, 170, 20);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(50, 50, 50);          // #323232 - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(220, 220, 220);       // #dcdcdc - 与前景色协调
    colors.dialog_title_bg = Color::RGB(30, 30, 30);    // #1e1e1e - 比背景稍深
    colors.dialog_title_fg = Color::RGB(220, 220, 220); // #dcdcdc - 与前景色一致
    colors.dialog_border = Color::RGB(100, 100, 100);   // #646464 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::PaperColor() {
    ThemeColors colors;
    // Modernized PaperColor: clean neutral dark theme with readable accents
    colors.background = Color::RGB(30, 30, 32);
    colors.foreground = Color::RGB(220, 220, 220);
    colors.current_line = Color::RGB(42, 42, 44);
    colors.selection = Color::RGB(58, 58, 60);
    colors.line_number = Color::RGB(138, 138, 138);
    colors.line_number_current = Color::RGB(220, 220, 220);

    colors.statusbar_bg = Color::RGB(40, 40, 42);
    colors.statusbar_fg = Color::RGB(220, 220, 220);

    colors.menubar_bg = Color::RGB(30, 30, 32);
    colors.menubar_fg = Color::RGB(220, 220, 220);

    colors.helpbar_bg = Color::RGB(40, 40, 42);
    colors.helpbar_fg = Color::RGB(138, 138, 138);
    colors.helpbar_key = Color::RGB(140, 200, 80);

    colors.keyword = Color::RGB(200, 90, 220);
    colors.string = Color::RGB(90, 180, 90);
    colors.comment = Color::RGB(128, 128, 128);
    colors.number = Color::RGB(80, 160, 190);
    colors.function = Color::RGB(80, 80, 220);
    colors.type = Color::RGB(120, 160, 80);
    colors.operator_color = Color::RGB(200, 90, 220);

    colors.error = Color::RGB(220, 60, 60);
    colors.warning = Color::RGB(220, 140, 60);
    colors.info = Color::RGB(80, 80, 220);
    colors.success = Color::RGB(80, 180, 90);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(45, 45, 45);          // #2d2d2d - 比背景稍亮的深炭灰
    colors.dialog_fg = Color::RGB(235, 235, 235);       // #ebebeb - 与前景色协调
    colors.dialog_title_bg = Color::RGB(30, 30, 30);    // #1e1e1e - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 235, 235); // #ebebeb - 与前景色一致
    colors.dialog_border = Color::RGB(140, 140, 140);   // #8c8c8c - 与行号颜色协调

    return colors;
}

ThemeColors Theme::RosePine() {
    ThemeColors colors;
    // Modernized RosePine: softer pastels with improved contrast
    colors.background = Color::RGB(30, 28, 44);
    colors.foreground = Color::RGB(230, 226, 245);
    colors.current_line = Color::RGB(34, 32, 50);
    colors.selection = Color::RGB(68, 64, 92);
    colors.line_number = Color::RGB(112, 108, 138);
    colors.line_number_current = Color::RGB(230, 226, 245);

    colors.statusbar_bg = Color::RGB(34, 32, 50);
    colors.statusbar_fg = Color::RGB(230, 226, 245);

    colors.menubar_bg = Color::RGB(30, 28, 44);
    colors.menubar_fg = Color::RGB(230, 226, 245);

    colors.helpbar_bg = Color::RGB(34, 32, 50);
    colors.helpbar_fg = Color::RGB(112, 108, 138);
    colors.helpbar_key = Color::RGB(160, 210, 220);

    colors.keyword = Color::RGB(240, 120, 150);
    colors.string = Color::RGB(160, 210, 220);
    colors.comment = Color::RGB(112, 108, 138);
    colors.number = Color::RGB(250, 200, 140);
    colors.function = Color::RGB(160, 210, 220);
    colors.type = Color::RGB(200, 170, 235);
    colors.operator_color = Color::RGB(240, 120, 150);

    colors.error = Color::RGB(240, 120, 150);
    colors.warning = Color::RGB(250, 200, 140);
    colors.info = Color::RGB(160, 210, 220);
    colors.success = Color::RGB(160, 210, 220);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(36, 34, 52);          // #242234 - 比背景稍亮的柔和紫灰
    colors.dialog_fg = Color::RGB(230, 226, 245);       // #e6e2f5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(30, 28, 44);    // #1e1c2c - 比背景稍深
    colors.dialog_title_fg = Color::RGB(230, 226, 245); // #e6e2f5 - 与前景色一致
    colors.dialog_border = Color::RGB(115, 111, 139);   // #736f8b - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Everforest() {
    ThemeColors colors;
    // Modern Everforest: forest greens with balanced contrast
    colors.background = Color::RGB(36, 44, 48);
    colors.foreground = Color::RGB(225, 220, 200);
    colors.current_line = Color::RGB(48, 56, 60);
    colors.selection = Color::RGB(60, 70, 72);
    colors.line_number = Color::RGB(120, 132, 140);
    colors.line_number_current = Color::RGB(225, 220, 200);

    colors.statusbar_bg = Color::RGB(40, 48, 52);
    colors.statusbar_fg = Color::RGB(225, 220, 200);

    colors.menubar_bg = Color::RGB(36, 44, 48);
    colors.menubar_fg = Color::RGB(225, 220, 200);

    colors.helpbar_bg = Color::RGB(40, 48, 52);
    colors.helpbar_fg = Color::RGB(120, 132, 140);
    colors.helpbar_key = Color::RGB(170, 200, 150);

    colors.keyword = Color::RGB(140, 190, 185);
    colors.string = Color::RGB(170, 200, 140);
    colors.comment = Color::RGB(120, 132, 140);
    colors.number = Color::RGB(230, 180, 120);
    colors.function = Color::RGB(170, 200, 140);
    colors.type = Color::RGB(140, 190, 185);
    colors.operator_color = Color::RGB(140, 190, 185);

    colors.error = Color::RGB(220, 110, 115);
    colors.warning = Color::RGB(230, 180, 120);
    colors.info = Color::RGB(140, 190, 185);
    colors.success = Color::RGB(170, 200, 140);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(56, 63, 70);          // #383f46 - 比背景稍亮的森林绿灰
    colors.dialog_fg = Color::RGB(225, 220, 200);       // #e1dcc8 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(44, 50, 56);    // #2c3238 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(225, 220, 200); // #e1dcc8 - 与前景色一致
    colors.dialog_border = Color::RGB(130, 142, 154);   // #828e9a - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Jellybeans() {
    ThemeColors colors;
    // Jellybeans 官方案例：深炭黑基底 + 糖果色点缀
    colors.background = Color::RGB(21, 21, 21);             // #151515
    colors.foreground = Color::RGB(232, 232, 211);          // #e8e8d3
    colors.current_line = Color::RGB(28, 28, 28);           // #1c1c1c
    colors.selection = Color::RGB(64, 64, 64);              // #404040
    colors.line_number = Color::RGB(96, 89, 88);            // #605958
    colors.line_number_current = Color::RGB(204, 197, 196); // #ccc5c4

    colors.statusbar_bg = Color::RGB(64, 60, 65); // #403c41
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(21, 21, 21);
    colors.menubar_fg = Color::RGB(232, 232, 211);

    colors.helpbar_bg = Color::RGB(64, 60, 65);
    colors.helpbar_fg = Color::RGB(136, 136, 136);
    colors.helpbar_key = Color::RGB(250, 208, 122); // #fad07a Function

    // Jellybeans 语法：蓝关键词/绿字符串/黄函数/橙类型/珊瑚数字
    colors.keyword = Color::RGB(129, 151, 191);        // #8197bf Statement
    colors.string = Color::RGB(153, 173, 106);         // #99ad6a
    colors.comment = Color::RGB(136, 136, 136);        // #888888
    colors.number = Color::RGB(207, 106, 76);          // #cf6a4c Constant
    colors.function = Color::RGB(250, 208, 122);       // #fad07a
    colors.type = Color::RGB(255, 185, 100);           // #ffb964
    colors.operator_color = Color::RGB(143, 191, 220); // #8fbfdc Structure

    colors.error = Color::RGB(207, 106, 76);    // #cf6a4c
    colors.warning = Color::RGB(255, 185, 100); // #ffb964
    colors.info = Color::RGB(143, 191, 220);    // #8fbfdc
    colors.success = Color::RGB(153, 173, 106); // #99ad6a

    colors.dialog_bg = Color::RGB(48, 48, 56); // #303038 Pmenu
    colors.dialog_fg = Color::RGB(255, 255, 255);
    colors.dialog_title_bg = Color::RGB(28, 28, 28);
    colors.dialog_title_fg = Color::RGB(232, 232, 211);
    colors.dialog_border = Color::RGB(96, 89, 88);

    return colors;
}

ThemeColors Theme::Desert() {
    ThemeColors colors;
    // Desert 官方案例：Santa Fe 沙漠黄昏，暖灰基底 + 卡其/珊瑚/青绿
    colors.background = Color::RGB(51, 51, 51);      // #333333
    colors.foreground = Color::RGB(255, 255, 255);   // #ffffff
    colors.current_line = Color::RGB(102, 102, 102); // #666666
    colors.selection = Color::RGB(107, 142, 36);     // #6b8e24 olive (Visual)
    colors.line_number = Color::RGB(238, 238, 0);    // #eeee00
    colors.line_number_current = Color::RGB(238, 238, 0);

    colors.statusbar_bg = Color::RGB(194, 191, 165); // #c2bfa5 tan accent
    colors.statusbar_fg = Color::RGB(51, 51, 51);

    colors.menubar_bg = Color::RGB(51, 51, 51);
    colors.menubar_fg = Color::RGB(255, 255, 255);

    colors.helpbar_bg = Color::RGB(194, 191, 165);
    colors.helpbar_fg = Color::RGB(127, 127, 140);  // #7f7f8c
    colors.helpbar_key = Color::RGB(240, 230, 140); // #f0e68c khaki

    // Desert 语法：卡其关键词/青绿函数/暗卡其类型/珊瑚预处理器
    colors.keyword = Color::RGB(240, 230, 140);        // #f0e68c Statement khaki
    colors.string = Color::RGB(137, 251, 152);         // #89fb98 Identifier
    colors.comment = Color::RGB(109, 206, 235);        // #6dceeb Comment cyan
    colors.number = Color::RGB(255, 160, 160);         // #ffa0a0 Constant
    colors.function = Color::RGB(137, 251, 152);       // #89fb98 Identifier
    colors.type = Color::RGB(189, 183, 107);           // #bdb76b dark khaki
    colors.operator_color = Color::RGB(240, 230, 140); // #f0e68c

    colors.error = Color::RGB(205, 92, 92);     // #cd5c5c indian red
    colors.warning = Color::RGB(255, 222, 155); // #ffde9b peach
    colors.info = Color::RGB(109, 206, 235);    // #6dceeb cyan
    colors.success = Color::RGB(154, 205, 50);  // #9acd32 green

    colors.dialog_bg = Color::RGB(102, 102, 102); // #666666 Pmenu
    colors.dialog_fg = Color::RGB(255, 255, 255);
    colors.dialog_title_bg = Color::RGB(77, 77, 77); // #4d4d4d
    colors.dialog_title_fg = Color::RGB(238, 238, 0);
    colors.dialog_border = Color::RGB(194, 191, 165);

    return colors;
}

ThemeColors Theme::Slate() {
    ThemeColors colors;
    // Modernized Slate: sophisticated dark blue-gray with clean, professional accents
    colors.background = Color::RGB(30, 35, 42);
    colors.foreground = Color::RGB(215, 225, 235);
    colors.current_line = Color::RGB(45, 52, 62);
    colors.selection = Color::RGB(60, 68, 80);
    colors.line_number = Color::RGB(115, 125, 135);
    colors.line_number_current = Color::RGB(215, 225, 235);

    colors.statusbar_bg = Color::RGB(35, 40, 48);
    colors.statusbar_fg = Color::RGB(215, 225, 235);

    colors.menubar_bg = Color::RGB(30, 35, 42);
    colors.menubar_fg = Color::RGB(215, 225, 235);

    colors.helpbar_bg = Color::RGB(35, 40, 48);
    colors.helpbar_fg = Color::RGB(115, 125, 135);
    colors.helpbar_key = Color::RGB(130, 200, 255);

    colors.keyword = Color::RGB(250, 130, 125);
    colors.string = Color::RGB(130, 200, 255);
    colors.comment = Color::RGB(115, 125, 135);
    colors.number = Color::RGB(130, 200, 255);
    colors.function = Color::RGB(210, 175, 255);
    colors.type = Color::RGB(130, 200, 255);
    colors.operator_color = Color::RGB(250, 130, 125);

    colors.error = Color::RGB(245, 90, 85);
    colors.warning = Color::RGB(250, 195, 25);
    colors.info = Color::RGB(130, 200, 255);
    colors.success = Color::RGB(70, 220, 170);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(50, 57, 67);          // #323943 - 比背景稍亮的深蓝灰
    colors.dialog_fg = Color::RGB(215, 225, 235);       // #d7e1eb - 与前景色协调
    colors.dialog_title_bg = Color::RGB(34, 39, 46);    // #22272e - 比背景稍深
    colors.dialog_title_fg = Color::RGB(215, 225, 235); // #d7e1eb - 与前景色一致
    colors.dialog_border = Color::RGB(120, 130, 140);   // #78828c - 与行号颜色协调

    return colors;
}

ThemeColors Theme::AtomOneLight() {
    ThemeColors colors;
    // Atom One Light: clean light theme with vibrant, readable accents
    colors.background = Color::RGB(250, 250, 250);       // #fafafa - Clean white background
    colors.foreground = Color::RGB(56, 58, 66);          // #383a42 - Dark gray foreground
    colors.current_line = Color::RGB(245, 245, 245);     // #f5f5f5 - Subtle current line
    colors.selection = Color::RGB(230, 240, 255);        // #e6f0ff - Light blue selection
    colors.line_number = Color::RGB(160, 160, 170);      // #a0a0aa - Muted line numbers
    colors.line_number_current = Color::RGB(56, 58, 66); // #383a42 - Current line number

    colors.statusbar_bg = Color::RGB(245, 245, 245); // #f5f5f5
    colors.statusbar_fg = Color::RGB(56, 58, 66);    // #383a42

    colors.menubar_bg = Color::RGB(250, 250, 250); // #fafafa
    colors.menubar_fg = Color::RGB(56, 58, 66);    // #383a42

    colors.helpbar_bg = Color::RGB(245, 245, 245); // #f5f5f5
    colors.helpbar_fg = Color::RGB(160, 160, 170); // #a0a0aa
    colors.helpbar_key = Color::RGB(50, 150, 255); // #3296ff - Blue accent

    colors.keyword = Color::RGB(166, 38, 164);      // #a626a4 - Purple for keywords
    colors.string = Color::RGB(80, 161, 79);        // #50a14f - Green for strings
    colors.comment = Color::RGB(160, 160, 170);     // #a0a0aa - Gray for comments
    colors.number = Color::RGB(152, 104, 1);        // #986801 - Brown for numbers
    colors.function = Color::RGB(97, 175, 239);     // #61afef - Blue for functions
    colors.type = Color::RGB(184, 187, 38);         // #b8bb26 - Yellow-green for types
    colors.operator_color = Color::RGB(56, 58, 66); // #383a42 - Dark for operators

    colors.error = Color::RGB(225, 53, 47);    // #e1352f - Red for errors
    colors.warning = Color::RGB(209, 154, 12); // #d19a0c - Orange for warnings
    colors.info = Color::RGB(97, 175, 239);    // #61afef - Blue for info
    colors.success = Color::RGB(80, 161, 79);  // #50a14f - Green for success

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(240, 240, 240);       // #f0f0f0 - 比背景稍深的浅灰
    colors.dialog_fg = Color::RGB(56, 58, 66);          // #383a42 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(250, 250, 250); // #fafafa - 比背景稍亮
    colors.dialog_title_fg = Color::RGB(56, 58, 66);    // #383a42 - 与前景色一致
    colors.dialog_border = Color::RGB(200, 200, 210);   // #c8c8d2 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::TokyoNightDay() {
    ThemeColors colors;
    // Tokyo Night Day: bright, clean light theme with cool blue accents
    colors.background = Color::RGB(250, 248, 245);       // #faf8f5 - Warm off-white
    colors.foreground = Color::RGB(45, 52, 70);          // #2d3446 - Dark blue-gray
    colors.current_line = Color::RGB(245, 243, 240);     // #f5f3f0 - Subtle highlight
    colors.selection = Color::RGB(220, 230, 245);        // #dce6f5 - Light blue selection
    colors.line_number = Color::RGB(160, 170, 190);      // #a0aabe - Muted line numbers
    colors.line_number_current = Color::RGB(45, 52, 70); // #2d3446 - Current line number

    colors.statusbar_bg = Color::RGB(240, 238, 235); // #f0eeeb
    colors.statusbar_fg = Color::RGB(45, 52, 70);    // #2d3446

    colors.menubar_bg = Color::RGB(250, 248, 245); // #faf8f5
    colors.menubar_fg = Color::RGB(45, 52, 70);    // #2d3446

    colors.helpbar_bg = Color::RGB(240, 238, 235); // #f0eeeb
    colors.helpbar_fg = Color::RGB(160, 170, 190); // #a0aabe
    colors.helpbar_key = Color::RGB(20, 132, 200); // #1484c8 - Blue accent

    colors.keyword = Color::RGB(180, 100, 220);        // #b464dc - Purple for keywords
    colors.string = Color::RGB(100, 180, 100);         // #64b464 - Green for strings
    colors.comment = Color::RGB(140, 150, 170);        // #8c96aa - Gray-blue for comments
    colors.number = Color::RGB(240, 150, 80);          // #f09650 - Orange for numbers
    colors.function = Color::RGB(20, 132, 200);        // #1484c8 - Blue for functions
    colors.type = Color::RGB(100, 150, 200);           // #6496c8 - Light blue for types
    colors.operator_color = Color::RGB(180, 100, 220); // #b464dc - Purple for operators

    colors.error = Color::RGB(220, 80, 100);    // #dc5064 - Red for errors
    colors.warning = Color::RGB(240, 150, 80);  // #f09650 - Orange for warnings
    colors.info = Color::RGB(20, 132, 200);     // #1484c8 - Blue for info
    colors.success = Color::RGB(100, 180, 100); // #64b464 - Green for success

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(240, 238, 235);       // #f0eeeb - 比背景稍深的暖白
    colors.dialog_fg = Color::RGB(45, 52, 70);          // #2d3446 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(250, 248, 245); // #faf8f5 - 比背景稍亮
    colors.dialog_title_fg = Color::RGB(45, 52, 70);    // #2d3446 - 与前景色一致
    colors.dialog_border = Color::RGB(200, 210, 220);   // #c8d2dc - 与行号颜色协调

    return colors;
}

ThemeColors Theme::BlueLight() {
    ThemeColors colors;
    // Blue Light: 黑色背景 + 白色文字 + 浅蓝色主色
    colors.background = Color::RGB(0, 0, 0);                // #000000 - 纯黑背景
    colors.foreground = Color::RGB(255, 255, 255);          // #ffffff - 白色文字
    colors.current_line = Color::RGB(15, 22, 35);           // #0f1623 - 深蓝灰高亮
    colors.selection = Color::RGB(25, 45, 80);              // #192d50 - 浅蓝调选中
    colors.line_number = Color::RGB(100, 145, 190);         // #6491be - 浅蓝灰行号
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff - 当前行号

    colors.statusbar_bg = Color::RGB(8, 12, 22);     // #080c16
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(0, 0, 0);       // #000000
    colors.menubar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.helpbar_bg = Color::RGB(8, 12, 22);      // #080c16
    colors.helpbar_fg = Color::RGB(180, 200, 230);  // #b4c8e6
    colors.helpbar_key = Color::RGB(135, 206, 250); // #87ceeb - 浅蓝按键

    // 浅蓝色系语法高亮
    colors.keyword = Color::RGB(135, 206, 250);        // #87ceeb - sky blue
    colors.string = Color::RGB(173, 216, 230);         // #add8e6 - light blue
    colors.comment = Color::RGB(100, 149, 237);        // #6495ed - cornflower
    colors.number = Color::RGB(176, 224, 230);         // #b0e0e6 - powder blue
    colors.function = Color::RGB(135, 206, 255);       // #87ceff - 亮天蓝
    colors.type = Color::RGB(100, 149, 237);           // #6495ed - cornflower
    colors.operator_color = Color::RGB(135, 206, 250); // #87ceeb

    colors.error = Color::RGB(255, 105, 105);   // #ff6969
    colors.warning = Color::RGB(255, 200, 120); // #ffc878
    colors.info = Color::RGB(135, 206, 250);    // #87ceeb
    colors.success = Color::RGB(100, 220, 180); // #64dcb4 - 青绿

    // 弹窗
    colors.dialog_bg = Color::RGB(10, 18, 30);          // #0a121e
    colors.dialog_fg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_title_bg = Color::RGB(20, 35, 55);    // #142337
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(60, 100, 150);    // #3c6496 - 浅蓝边框

    return colors;
}

ThemeColors Theme::Cyberpunk() {
    ThemeColors colors;
    // Cyberpunk: 赛博朋克霓虹风 - 深黑背景 + 霓虹粉/青/黄
    colors.background = Color::RGB(8, 8, 18);             // #080812 - 深黑偏紫
    colors.foreground = Color::RGB(230, 230, 245);        // #e6e6f5 - 淡紫白
    colors.current_line = Color::RGB(18, 18, 35);         // #121223 - 霓虹高亮区
    colors.selection = Color::RGB(40, 20, 60);            // #28143c - 粉紫选中
    colors.line_number = Color::RGB(100, 80, 140);        // #64508c - 紫灰行号
    colors.line_number_current = Color::RGB(255, 0, 255); // #ff00ff - 霓虹品红

    colors.statusbar_bg = Color::RGB(15, 10, 25);  // #0f0a19
    colors.statusbar_fg = Color::RGB(0, 255, 255); // #00ffff - 霓虹青

    colors.menubar_bg = Color::RGB(8, 8, 18);      // #080812
    colors.menubar_fg = Color::RGB(230, 230, 245); // #e6e6f5

    colors.helpbar_bg = Color::RGB(15, 10, 25);    // #0f0a19
    colors.helpbar_fg = Color::RGB(150, 130, 200); // #9682c8
    colors.helpbar_key = Color::RGB(255, 0, 255);  // #ff00ff - 霓虹粉

    // 赛博朋克霓虹语法高亮
    colors.keyword = Color::RGB(255, 0, 255);        // #ff00ff - 霓虹品红
    colors.string = Color::RGB(0, 255, 200);         // #00ffc8 - 霓虹青绿
    colors.comment = Color::RGB(120, 100, 160);      // #7864a0 - 紫灰
    colors.number = Color::RGB(255, 200, 0);         // #ffc800 - 霓虹黄
    colors.function = Color::RGB(0, 255, 255);       // #00ffff - 霓虹青
    colors.type = Color::RGB(255, 100, 200);         // #ff64c8 - 粉红
    colors.operator_color = Color::RGB(255, 0, 255); // #ff00ff

    colors.error = Color::RGB(255, 50, 80);   // #ff3250
    colors.warning = Color::RGB(255, 200, 0); // #ffc800
    colors.info = Color::RGB(0, 255, 255);    // #00ffff
    colors.success = Color::RGB(0, 255, 150); // #00ff96 - 霓虹绿

    // 弹窗
    colors.dialog_bg = Color::RGB(15, 12, 28);        // #0f0c1c
    colors.dialog_fg = Color::RGB(230, 230, 245);     // #e6e6f5
    colors.dialog_title_bg = Color::RGB(40, 20, 70);  // #281446 - 霓虹紫
    colors.dialog_title_fg = Color::RGB(255, 0, 255); // #ff00ff
    colors.dialog_border = Color::RGB(100, 50, 180);  // #6432b4 - 紫蓝边框

    return colors;
}

ThemeColors Theme::Hacker() {
    ThemeColors colors;
    // Hacker: 黑底绿字，Matrix/终端风格，青/琥珀点缀
    colors.background = Color::RGB(0, 0, 0);             // #000000 纯黑
    colors.foreground = Color::RGB(0, 255, 65);          // #00ff41 矩阵绿
    colors.current_line = Color::RGB(10, 30, 10);        // #0a1e0a 暗绿高亮
    colors.selection = Color::RGB(20, 50, 20);           // #143214
    colors.line_number = Color::RGB(0, 120, 40);         // #007828 暗绿
    colors.line_number_current = Color::RGB(0, 255, 65); // 亮绿

    colors.statusbar_bg = Color::RGB(5, 15, 5);
    colors.statusbar_fg = Color::RGB(0, 255, 65);

    colors.menubar_bg = Color::RGB(0, 0, 0);
    colors.menubar_fg = Color::RGB(0, 255, 65);

    colors.helpbar_bg = Color::RGB(5, 15, 5);
    colors.helpbar_fg = Color::RGB(0, 180, 55);
    colors.helpbar_key = Color::RGB(0, 255, 200); // 青绿

    colors.keyword = Color::RGB(0, 255, 200);  // #00ffc8 青绿
    colors.string = Color::RGB(0, 255, 65);    // 亮绿
    colors.comment = Color::RGB(0, 120, 40);   // 暗绿
    colors.number = Color::RGB(255, 200, 50);  // #ffc832 琥珀
    colors.function = Color::RGB(0, 255, 255); // #00ffff 青
    colors.type = Color::RGB(0, 220, 180);     // 青绿
    colors.operator_color = Color::RGB(0, 255, 200);

    colors.error = Color::RGB(255, 50, 50);    // 红
    colors.warning = Color::RGB(255, 200, 50); // 琥珀
    colors.info = Color::RGB(0, 255, 255);     // 青
    colors.success = Color::RGB(0, 255, 65);   // 绿

    colors.dialog_bg = Color::RGB(5, 20, 8);
    colors.dialog_fg = Color::RGB(0, 255, 65);
    colors.dialog_title_bg = Color::RGB(10, 40, 15);
    colors.dialog_title_fg = Color::RGB(0, 255, 65);
    colors.dialog_border = Color::RGB(0, 120, 40);

    return colors;
}

ThemeColors Theme::HatsuneMiku() {
    ThemeColors colors;
    // 初音未来：黑青基底 + 葱色/粉/金黄点缀
    colors.background = Color::RGB(15, 18, 28);            // #0f121c 深蓝黑
    colors.foreground = Color::RGB(230, 245, 250);         // #e6f5fa 柔和白青
    colors.current_line = Color::RGB(22, 28, 42);          // #1c1c2a
    colors.selection = Color::RGB(40, 60, 90);             // #283c5a
    colors.line_number = Color::RGB(80, 140, 160);         // 葱色系灰
    colors.line_number_current = Color::RGB(57, 197, 207); // #39C5CF 初音葱色

    colors.statusbar_bg = Color::RGB(20, 25, 38);
    colors.statusbar_fg = Color::RGB(230, 245, 250);

    colors.menubar_bg = Color::RGB(15, 18, 28);
    colors.menubar_fg = Color::RGB(230, 245, 250);

    colors.helpbar_bg = Color::RGB(20, 25, 38);
    colors.helpbar_fg = Color::RGB(80, 140, 160);
    colors.helpbar_key = Color::RGB(57, 197, 207); // 葱色

    // 初音配色：粉关键词/金黄字符串/葱色函数/粉紫类型
    colors.keyword = Color::RGB(255, 105, 180); // #ff69b4 粉 (目/装饰)
    colors.string = Color::RGB(255, 215, 0);    // #ffd700 金黄 (丝带)
    colors.comment = Color::RGB(100, 150, 170);
    colors.number = Color::RGB(255, 193, 7);    // #ffc107 金
    colors.function = Color::RGB(57, 197, 207); // #39C5CF 初音葱色
    colors.type = Color::RGB(200, 130, 230);    // 淡紫
    colors.operator_color = Color::RGB(57, 197, 207);

    colors.error = Color::RGB(255, 82, 82);
    colors.warning = Color::RGB(255, 193, 7);
    colors.info = Color::RGB(57, 197, 207);
    colors.success = Color::RGB(100, 220, 180);

    colors.dialog_bg = Color::RGB(22, 28, 42);
    colors.dialog_fg = Color::RGB(230, 245, 250);
    colors.dialog_title_bg = Color::RGB(30, 45, 70);
    colors.dialog_title_fg = Color::RGB(57, 197, 207);
    colors.dialog_border = Color::RGB(80, 140, 160);

    return colors;
}

void Theme::setTheme(const std::string& name) {
    current_theme_ = name;

    // 首先检查是否是自定义主题
    if (custom_themes_.find(name) != custom_themes_.end()) {
        colors_ = custom_themes_[name];
        return;
    }

    // 否则使用预设主题
    if (name == "monokai") {
        colors_ = Monokai();
    } else if (name == "dracula") {
        colors_ = Dracula();
    } else if (name == "solarized-dark") {
        colors_ = SolarizedDark();
    } else if (name == "solarized-light") {
        colors_ = SolarizedLight();
    } else if (name == "onedark") {
        colors_ = OneDark();
    } else if (name == "nord") {
        colors_ = Nord();
    } else if (name == "gruvbox") {
        colors_ = Gruvbox();
    } else if (name == "tokyo-night") {
        colors_ = TokyoNight();
    } else if (name == "catppuccin") {
        colors_ = Catppuccin();
    } else if (name == "material") {
        colors_ = Material();
    } else if (name == "ayu") {
        colors_ = Ayu();
    } else if (name == "github") {
        colors_ = GitHub();
    } else if (name == "github-dark") {
        colors_ = GitHubDark();
    } else if (name == "markdown-dark") {
        colors_ = MarkdownDark();
    } else if (name == "vscode-dark") {
        colors_ = VSCodeDark();
    } else if (name == "night-owl") {
        colors_ = NightOwl();
    } else if (name == "palenight") {
        colors_ = Palenight();
    } else if (name == "oceanic-next") {
        colors_ = OceanicNext();
    } else if (name == "kanagawa") {
        colors_ = Kanagawa();
    } else if (name == "tomorrow-night") {
        colors_ = TomorrowNight();
    } else if (name == "tomorrow-night-blue") {
        colors_ = TomorrowNightBlue();
    } else if (name == "cobalt") {
        colors_ = Cobalt();
    } else if (name == "zenburn") {
        colors_ = Zenburn();
    } else if (name == "base16-dark") {
        colors_ = Base16Dark();
    } else if (name == "papercolor") {
        colors_ = PaperColor();
    } else if (name == "rose-pine") {
        colors_ = RosePine();
    } else if (name == "everforest") {
        colors_ = Everforest();
    } else if (name == "jellybeans") {
        colors_ = Jellybeans();
    } else if (name == "desert") {
        colors_ = Desert();
    } else if (name == "slate") {
        colors_ = Slate();
    } else if (name == "atom-one-light") {
        colors_ = AtomOneLight();
    } else if (name == "tokyo-night-day") {
        colors_ = TokyoNightDay();
    } else if (name == "blue-light") {
        colors_ = BlueLight();
    } else if (name == "cyberpunk") {
        colors_ = Cyberpunk();
    } else if (name == "hacker") {
        colors_ = Hacker();
    } else if (name == "hatsune-miku") {
        colors_ = HatsuneMiku();
    } else {
        colors_ = Monokai(); // 默认主题
    }
}

bool Theme::loadCustomTheme(const std::string& name, const ThemeColors& colors) {
    custom_themes_[name] = colors;
    // 注意：自定义主题的持久化现在通过插件系统处理
    // 当插件加载时会重新添加这些主题
    return true;
}

bool Theme::removeCustomTheme(const std::string& name) {
    auto it = custom_themes_.find(name);
    if (it != custom_themes_.end()) {
        custom_themes_.erase(it);
        return true;
    }
    return false;
}

void Theme::clearCustomThemes() {
    custom_themes_.clear();
}

bool Theme::loadThemeFromConfig(
    const std::vector<int>& background, const std::vector<int>& foreground,
    const std::vector<int>& current_line, const std::vector<int>& selection,
    const std::vector<int>& line_number, const std::vector<int>& line_number_current,
    const std::vector<int>& statusbar_bg, const std::vector<int>& statusbar_fg,
    const std::vector<int>& menubar_bg, const std::vector<int>& menubar_fg,
    const std::vector<int>& helpbar_bg, const std::vector<int>& helpbar_fg,
    const std::vector<int>& helpbar_key, const std::vector<int>& keyword,
    const std::vector<int>& string, const std::vector<int>& comment, const std::vector<int>& number,
    const std::vector<int>& function, const std::vector<int>& type,
    const std::vector<int>& operator_color, const std::vector<int>& error,
    const std::vector<int>& warning, const std::vector<int>& info, const std::vector<int>& success,
    const std::vector<int>& dialog_bg, const std::vector<int>& dialog_fg,
    const std::vector<int>& dialog_title_bg, const std::vector<int>& dialog_title_fg,
    const std::vector<int>& dialog_border) {
    ThemeColors colors;

    if (background.size() >= 3)
        colors.background = rgbToColor(background);
    if (foreground.size() >= 3)
        colors.foreground = rgbToColor(foreground);
    if (current_line.size() >= 3)
        colors.current_line = rgbToColor(current_line);
    if (selection.size() >= 3)
        colors.selection = rgbToColor(selection);
    if (line_number.size() >= 3)
        colors.line_number = rgbToColor(line_number);
    if (line_number_current.size() >= 3)
        colors.line_number_current = rgbToColor(line_number_current);
    if (statusbar_bg.size() >= 3)
        colors.statusbar_bg = rgbToColor(statusbar_bg);
    if (statusbar_fg.size() >= 3)
        colors.statusbar_fg = rgbToColor(statusbar_fg);
    if (menubar_bg.size() >= 3)
        colors.menubar_bg = rgbToColor(menubar_bg);
    if (menubar_fg.size() >= 3)
        colors.menubar_fg = rgbToColor(menubar_fg);
    if (helpbar_bg.size() >= 3)
        colors.helpbar_bg = rgbToColor(helpbar_bg);
    if (helpbar_fg.size() >= 3)
        colors.helpbar_fg = rgbToColor(helpbar_fg);
    if (helpbar_key.size() >= 3)
        colors.helpbar_key = rgbToColor(helpbar_key);
    if (keyword.size() >= 3)
        colors.keyword = rgbToColor(keyword);
    if (string.size() >= 3)
        colors.string = rgbToColor(string);
    if (comment.size() >= 3)
        colors.comment = rgbToColor(comment);
    if (number.size() >= 3)
        colors.number = rgbToColor(number);
    if (function.size() >= 3)
        colors.function = rgbToColor(function);
    if (type.size() >= 3)
        colors.type = rgbToColor(type);
    if (operator_color.size() >= 3)
        colors.operator_color = rgbToColor(operator_color);
    if (error.size() >= 3)
        colors.error = rgbToColor(error);
    if (warning.size() >= 3)
        colors.warning = rgbToColor(warning);
    if (info.size() >= 3)
        colors.info = rgbToColor(info);
    if (success.size() >= 3)
        colors.success = rgbToColor(success);
    if (dialog_bg.size() >= 3)
        colors.dialog_bg = rgbToColor(dialog_bg);
    if (dialog_fg.size() >= 3)
        colors.dialog_fg = rgbToColor(dialog_fg);
    if (dialog_title_bg.size() >= 3)
        colors.dialog_title_bg = rgbToColor(dialog_title_bg);
    if (dialog_title_fg.size() >= 3)
        colors.dialog_title_fg = rgbToColor(dialog_title_fg);
    if (dialog_border.size() >= 3)
        colors.dialog_border = rgbToColor(dialog_border);

    colors_ = colors;
    return true;
}

ftxui::Color Theme::rgbToColor(const std::vector<int>& rgb) {
    if (rgb.size() >= 3) {
        return Color::RGB(rgb[0], rgb[1], rgb[2]);
    }
    return Color::Default;
}

std::vector<std::string> Theme::getAvailableThemes() {
    return {"monokai",
            "dracula",
            "solarized-dark",
            "solarized-light",
            "onedark",
            "nord",
            "gruvbox",
            "tokyo-night",
            "catppuccin",
            "material",
            "ayu",
            "github",
            "github-dark",
            "markdown-dark",
            "vscode-dark",
            "night-owl",
            "palenight",
            "oceanic-next",
            "kanagawa",
            "tomorrow-night",
            "tomorrow-night-blue",
            "cobalt",
            "zenburn",
            "base16-dark",
            "papercolor",
            "rose-pine",
            "everforest",
            "jellybeans",
            "desert",
            "slate",
            "atom-one-light",
            "tokyo-night-day",
            "blue-light",
            "cyberpunk",
            "hacker",
            "hatsune-miku"};
}

std::vector<std::string> Theme::getCustomThemeNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : custom_themes_) {
        names.push_back(name);
    }
    return names;
}

} // namespace ui
} // namespace pnana
