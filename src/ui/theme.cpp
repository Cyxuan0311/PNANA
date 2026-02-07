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
    // Modernized Monokai-inspired palette (more contrast, tuned hues)
    colors.background = Color::RGB(40, 42, 38);             // slightly brighter base
    colors.foreground = Color::RGB(235, 235, 230);          // softer foreground
    colors.current_line = Color::RGB(60, 60, 55);           // current line
    colors.selection = Color::RGB(70, 72, 66);              // selection
    colors.line_number = Color::RGB(120, 120, 114);         // line numbers
    colors.line_number_current = Color::RGB(235, 235, 230); // current line number

    colors.statusbar_bg = Color::RGB(36, 38, 35);
    colors.statusbar_fg = Color::RGB(225, 225, 220);

    colors.menubar_bg = Color::RGB(30, 31, 28);
    colors.menubar_fg = Color::RGB(225, 225, 220);

    colors.helpbar_bg = Color::RGB(36, 38, 35);
    colors.helpbar_fg = Color::RGB(160, 155, 130);
    colors.helpbar_key = Color::RGB(140, 200, 72);

    // Accent palette - modern tones
    colors.keyword = Color::RGB(255, 99, 150);        // pink
    colors.string = Color::RGB(240, 200, 100);        // warm yellow
    colors.comment = Color::RGB(140, 135, 120);       // muted gray
    colors.number = Color::RGB(180, 140, 255);        // soft purple
    colors.function = Color::RGB(100, 200, 150);      // teal
    colors.type = Color::RGB(100, 180, 220);          // cyan
    colors.operator_color = Color::RGB(255, 99, 150); // same as keyword

    colors.error = Color::RGB(255, 85, 110);    // red
    colors.warning = Color::RGB(255, 165, 80);  // orange
    colors.info = Color::RGB(110, 180, 220);    // info/cyan
    colors.success = Color::RGB(100, 200, 150); // green/teal

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(50, 52, 48);          // #323430 - 比背景稍亮的深灰
    colors.dialog_fg = Color::RGB(235, 235, 230);       // #ebebe6 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(40, 42, 38);    // #282a26 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 235, 230); // #ebebe6 - 与前景色一致
    colors.dialog_border = Color::RGB(100, 100, 95);    // #64645f - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Dracula() {
    ThemeColors colors;
    // Modernized Dracula palette (retained character but with cleaner tones)
    colors.background = Color::RGB(43, 45, 66);    // slightly deeper base
    colors.foreground = Color::RGB(232, 235, 244); // softer foreground
    colors.current_line = Color::RGB(60, 62, 84);
    colors.selection = Color::RGB(64, 66, 90);
    colors.line_number = Color::RGB(110, 125, 170);
    colors.line_number_current = Color::RGB(232, 235, 244);

    colors.statusbar_bg = Color::RGB(58, 60, 86);
    colors.statusbar_fg = Color::RGB(232, 235, 244);

    colors.menubar_bg = Color::RGB(36, 38, 60);
    colors.menubar_fg = Color::RGB(232, 235, 244);

    colors.helpbar_bg = Color::RGB(58, 60, 86);
    colors.helpbar_fg = Color::RGB(200, 170, 240);
    colors.helpbar_key = Color::RGB(90, 220, 150);

    colors.keyword = Color::RGB(255, 120, 190);
    colors.string = Color::RGB(240, 220, 120);
    colors.comment = Color::RGB(110, 125, 170);
    colors.number = Color::RGB(200, 150, 250);
    colors.function = Color::RGB(90, 220, 150);
    colors.type = Color::RGB(120, 200, 230);
    colors.operator_color = Color::RGB(255, 120, 190);

    colors.error = Color::RGB(255, 95, 95);
    colors.warning = Color::RGB(255, 170, 95);
    colors.info = Color::RGB(120, 200, 230);
    colors.success = Color::RGB(90, 220, 150);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(68, 71, 90);          // #44475a - 比背景稍亮的紫灰色
    colors.dialog_fg = Color::RGB(232, 235, 244);       // #e8ebf4 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(43, 45, 66);    // #2b2d42 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(232, 235, 244); // #e8ebf4 - 与前景色一致
    colors.dialog_border = Color::RGB(110, 125, 170);   // #6e7daa - 与行号颜色协调

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
    // Modern Catppuccin variant with balanced contrasts
    colors.background = Color::RGB(34, 34, 50);
    colors.foreground = Color::RGB(210, 215, 235);
    colors.current_line = Color::RGB(48, 48, 66);
    colors.selection = Color::RGB(50, 50, 70);
    colors.line_number = Color::RGB(110, 112, 130);
    colors.line_number_current = Color::RGB(210, 215, 235);

    colors.statusbar_bg = Color::RGB(30, 30, 46);
    colors.statusbar_fg = Color::RGB(210, 215, 235);

    colors.menubar_bg = Color::RGB(34, 34, 50);
    colors.menubar_fg = Color::RGB(210, 215, 235);

    colors.helpbar_bg = Color::RGB(30, 30, 46);
    colors.helpbar_fg = Color::RGB(110, 112, 130);
    colors.helpbar_key = Color::RGB(150, 220, 170);

    colors.keyword = Color::RGB(200, 160, 240);
    colors.string = Color::RGB(160, 220, 160);
    colors.comment = Color::RGB(110, 112, 130);
    colors.number = Color::RGB(245, 185, 140);
    colors.function = Color::RGB(130, 210, 230);
    colors.type = Color::RGB(140, 210, 200);
    colors.operator_color = Color::RGB(200, 160, 240);

    colors.error = Color::RGB(245, 130, 165);
    colors.warning = Color::RGB(245, 185, 140);
    colors.info = Color::RGB(130, 210, 230);
    colors.success = Color::RGB(150, 220, 170);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(48, 48, 66);          // #303042 - 比背景稍亮的柔和紫灰
    colors.dialog_fg = Color::RGB(210, 215, 235);       // #d2d7eb - 与前景色协调
    colors.dialog_title_bg = Color::RGB(34, 34, 50);    // #222232 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(210, 215, 235); // #d2d7eb - 与前景色一致
    colors.dialog_border = Color::RGB(110, 112, 130);   // #6e7082 - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Material() {
    ThemeColors colors;
    // Modern Material: deep slate base with soft high-contrast accents
    colors.background = Color::RGB(40, 50, 58);
    colors.foreground = Color::RGB(235, 245, 250);
    colors.current_line = Color::RGB(44, 54, 62);
    colors.selection = Color::RGB(58, 74, 82);
    colors.line_number = Color::RGB(100, 120, 130);
    colors.line_number_current = Color::RGB(235, 245, 250);

    colors.statusbar_bg = Color::RGB(38, 48, 56);
    colors.statusbar_fg = Color::RGB(235, 245, 250);

    colors.menubar_bg = Color::RGB(40, 50, 58);
    colors.menubar_fg = Color::RGB(235, 245, 250);

    colors.helpbar_bg = Color::RGB(38, 48, 56);
    colors.helpbar_fg = Color::RGB(100, 120, 130);
    colors.helpbar_key = Color::RGB(180, 230, 140);

    colors.keyword = Color::RGB(205, 150, 230);
    colors.string = Color::RGB(190, 230, 140);
    colors.comment = Color::RGB(100, 120, 130);
    colors.number = Color::RGB(245, 160, 120);
    colors.function = Color::RGB(120, 180, 240);
    colors.type = Color::RGB(245, 205, 120);
    colors.operator_color = Color::RGB(205, 150, 230);

    colors.error = Color::RGB(245, 90, 120);
    colors.warning = Color::RGB(245, 205, 120);
    colors.info = Color::RGB(120, 180, 240);
    colors.success = Color::RGB(180, 230, 140);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(58, 74, 82);          // #3a4a52 - 比背景稍亮的深蓝灰
    colors.dialog_fg = Color::RGB(235, 245, 250);       // #ebf5fa - 与前景色协调
    colors.dialog_title_bg = Color::RGB(40, 50, 58);    // #28323a - 比背景稍深
    colors.dialog_title_fg = Color::RGB(235, 245, 250); // #ebf5fa - 与前景色一致
    colors.dialog_border = Color::RGB(100, 120, 130);   // #647882 - 与行号颜色协调

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
    // Modernized NightOwl: deep oceanic blue with vibrant, contrasting accents
    colors.background = Color::RGB(15, 35, 60);
    colors.foreground = Color::RGB(225, 235, 245);
    colors.current_line = Color::RGB(30, 55, 85);
    colors.selection = Color::RGB(45, 70, 105);
    colors.line_number = Color::RGB(120, 140, 160);
    colors.line_number_current = Color::RGB(225, 235, 245);

    colors.statusbar_bg = Color::RGB(18, 40, 70);
    colors.statusbar_fg = Color::RGB(225, 235, 245);

    colors.menubar_bg = Color::RGB(15, 35, 60);
    colors.menubar_fg = Color::RGB(225, 235, 245);

    colors.helpbar_bg = Color::RGB(18, 40, 70);
    colors.helpbar_fg = Color::RGB(120, 140, 160);
    colors.helpbar_key = Color::RGB(150, 210, 255);

    colors.keyword = Color::RGB(220, 180, 245);
    colors.string = Color::RGB(190, 240, 160);
    colors.comment = Color::RGB(120, 140, 160);
    colors.number = Color::RGB(250, 180, 130);
    colors.function = Color::RGB(150, 210, 255);
    colors.type = Color::RGB(160, 200, 250);
    colors.operator_color = Color::RGB(220, 180, 245);

    colors.error = Color::RGB(245, 130, 155);
    colors.warning = Color::RGB(245, 210, 130);
    colors.info = Color::RGB(150, 210, 255);
    colors.success = Color::RGB(170, 230, 150);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(35, 60, 90);          // #233c5a - 比背景稍亮的深蓝
    colors.dialog_fg = Color::RGB(225, 235, 245);       // #e1ebf5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(15, 35, 60);    // #0f233c - 比背景稍深
    colors.dialog_title_fg = Color::RGB(225, 235, 245); // #e1ebf5 - 与前景色一致
    colors.dialog_border = Color::RGB(120, 140, 160);   // #788ca0 - 与行号颜色协调

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
    // Modernized OceanicNext - preserve oceanic tone with clearer accents
    colors.background = Color::RGB(20, 38, 54);
    colors.foreground = Color::RGB(210, 224, 230);
    colors.current_line = Color::RGB(26, 50, 70);
    colors.selection = Color::RGB(36, 60, 80);
    colors.line_number = Color::RGB(110, 140, 155);
    colors.line_number_current = Color::RGB(210, 224, 230);

    colors.statusbar_bg = Color::RGB(24, 46, 66);
    colors.statusbar_fg = Color::RGB(210, 224, 230);

    colors.menubar_bg = Color::RGB(20, 38, 54);
    colors.menubar_fg = Color::RGB(210, 224, 230);

    colors.helpbar_bg = Color::RGB(24, 46, 66);
    colors.helpbar_fg = Color::RGB(110, 140, 155);
    colors.helpbar_key = Color::RGB(170, 210, 140);

    colors.keyword = Color::RGB(200, 140, 220);
    colors.string = Color::RGB(150, 210, 150);
    colors.comment = Color::RGB(110, 140, 155);
    colors.number = Color::RGB(245, 190, 90);
    colors.function = Color::RGB(110, 190, 230);
    colors.type = Color::RGB(220, 190, 120);
    colors.operator_color = Color::RGB(100, 185, 190);

    colors.error = Color::RGB(225, 90, 70);
    colors.warning = Color::RGB(240, 190, 90);
    colors.info = Color::RGB(110, 190, 230);
    colors.success = Color::RGB(160, 200, 120);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(30, 53, 70);          // #1e3546 - 比背景稍亮的深海洋蓝
    colors.dialog_fg = Color::RGB(210, 224, 230);       // #d2e0e6 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(20, 43, 60);    // #142b3c - 比背景稍深
    colors.dialog_title_fg = Color::RGB(210, 224, 230); // #d2e0e6 - 与前景色一致
    colors.dialog_border = Color::RGB(110, 140, 155);   // #6e8c9b - 与行号颜色协调

    return colors;
}

ThemeColors Theme::Kanagawa() {
    ThemeColors colors;
    // Modern Kanagawa: retain rich warm accents but increase readability
    colors.background = Color::RGB(32, 34, 44);
    colors.foreground = Color::RGB(225, 215, 185);
    colors.current_line = Color::RGB(44, 44, 56);
    colors.selection = Color::RGB(52, 78, 100);
    colors.line_number = Color::RGB(120, 118, 110);
    colors.line_number_current = Color::RGB(225, 215, 185);

    colors.statusbar_bg = Color::RGB(34, 34, 44);
    colors.statusbar_fg = Color::RGB(225, 215, 185);

    colors.menubar_bg = Color::RGB(32, 34, 44);
    colors.menubar_fg = Color::RGB(225, 215, 185);

    colors.helpbar_bg = Color::RGB(34, 34, 44);
    colors.helpbar_fg = Color::RGB(120, 118, 110);
    colors.helpbar_key = Color::RGB(150, 180, 220);

    colors.keyword = Color::RGB(160, 140, 195);
    colors.string = Color::RGB(160, 200, 110);
    colors.comment = Color::RGB(120, 118, 110);
    colors.number = Color::RGB(210, 170, 220);
    colors.function = Color::RGB(150, 180, 220);
    colors.type = Color::RGB(140, 180, 200);
    colors.operator_color = Color::RGB(160, 140, 195);

    colors.error = Color::RGB(200, 80, 90);
    colors.warning = Color::RGB(200, 160, 110);
    colors.info = Color::RGB(150, 180, 220);
    colors.success = Color::RGB(170, 200, 110);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(48, 48, 62);          // #30303e - 比背景稍亮的深紫灰
    colors.dialog_fg = Color::RGB(225, 215, 185);       // #e1d7b9 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(36, 36, 48);    // #242430 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(225, 215, 185); // #e1d7b9 - 与前景色一致
    colors.dialog_border = Color::RGB(120, 118, 110);   // #78766e - 与行号颜色协调

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
    // Modernized Jellybeans: deep charcoal base with bright, candy-like accents
    colors.background = Color::RGB(25, 25, 25);
    colors.foreground = Color::RGB(235, 235, 235);
    colors.current_line = Color::RGB(45, 45, 45);
    colors.selection = Color::RGB(75, 75, 75);
    colors.line_number = Color::RGB(120, 120, 120);
    colors.line_number_current = Color::RGB(235, 235, 235);

    colors.statusbar_bg = Color::RGB(30, 30, 30);
    colors.statusbar_fg = Color::RGB(235, 235, 235);

    colors.menubar_bg = Color::RGB(25, 25, 25);
    colors.menubar_fg = Color::RGB(235, 235, 235);

    colors.helpbar_bg = Color::RGB(30, 30, 30);
    colors.helpbar_fg = Color::RGB(120, 120, 120);
    colors.helpbar_key = Color::RGB(165, 210, 135);

    colors.keyword = Color::RGB(215, 115, 115);
    colors.string = Color::RGB(165, 210, 135);
    colors.comment = Color::RGB(120, 120, 120);
    colors.number = Color::RGB(250, 200, 60);
    colors.function = Color::RGB(250, 200, 60);
    colors.type = Color::RGB(110, 185, 245);
    colors.operator_color = Color::RGB(215, 115, 115);

    colors.error = Color::RGB(230, 120, 130);
    colors.warning = Color::RGB(250, 200, 60);
    colors.info = Color::RGB(110, 185, 245);
    colors.success = Color::RGB(165, 210, 135);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(45, 45, 45);          // #2d2d2d
    colors.dialog_fg = Color::RGB(235, 235, 235);       // #ebebeb
    colors.dialog_title_bg = Color::RGB(25, 25, 25);    // #191919
    colors.dialog_title_fg = Color::RGB(235, 235, 235); // #ebebeb
    colors.dialog_border = Color::RGB(120, 120, 120);   // #787878

    return colors;
}

ThemeColors Theme::Desert() {
    ThemeColors colors;
    // Modernized Desert: warm beige/charcoal base with vibrant, tropical accents
    colors.background = Color::RGB(45, 45, 45);
    colors.foreground = Color::RGB(245, 245, 245);
    colors.current_line = Color::RGB(65, 65, 65);
    colors.selection = Color::RGB(75, 75, 75);
    colors.line_number = Color::RGB(135, 135, 135);
    colors.line_number_current = Color::RGB(245, 245, 245);

    colors.statusbar_bg = Color::RGB(50, 50, 50);
    colors.statusbar_fg = Color::RGB(245, 245, 245);

    colors.menubar_bg = Color::RGB(45, 45, 45);
    colors.menubar_fg = Color::RGB(245, 245, 245);

    colors.helpbar_bg = Color::RGB(50, 50, 50);
    colors.helpbar_fg = Color::RGB(135, 135, 135);
    colors.helpbar_key = Color::RGB(245, 215, 175);

    colors.keyword = Color::RGB(100, 190, 235);
    colors.string = Color::RGB(235, 220, 125);
    colors.comment = Color::RGB(125, 120, 105);
    colors.number = Color::RGB(180, 135, 250);
    colors.function = Color::RGB(175, 235, 65);
    colors.type = Color::RGB(110, 225, 240);
    colors.operator_color = Color::RGB(245, 50, 125);

    colors.error = Color::RGB(245, 50, 125);
    colors.warning = Color::RGB(250, 165, 45);
    colors.info = Color::RGB(110, 225, 240);
    colors.success = Color::RGB(175, 235, 65);

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(70, 70, 70);          // #464646 - 比背景稍亮的暖灰
    colors.dialog_fg = Color::RGB(245, 245, 245);       // #f5f5f5 - 与前景色协调
    colors.dialog_title_bg = Color::RGB(50, 50, 50);    // #323232 - 比背景稍深
    colors.dialog_title_fg = Color::RGB(245, 245, 245); // #f5f5f5 - 与前景色一致
    colors.dialog_border = Color::RGB(140, 140, 140);   // #8c8c8c - 与行号颜色协调

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
            "slate"};
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
