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
    colors.background = Color::RGB(39, 40, 34);             // #272822
    colors.foreground = Color::RGB(248, 248, 242);          // #f8f8f2
    colors.current_line = Color::RGB(73, 72, 62);           // #49483e
    colors.selection = Color::RGB(73, 72, 62);              // #49483e
    colors.line_number = Color::RGB(144, 144, 138);         // #90908a
    colors.line_number_current = Color::RGB(248, 248, 242); // #f8f8f2

    colors.statusbar_bg = Color::RGB(45, 45, 45);    // #2d2d2d
    colors.statusbar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.menubar_bg = Color::RGB(30, 31, 27);    // #1e1f1b
    colors.menubar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.helpbar_bg = Color::RGB(45, 45, 45);    // #2d2d2d
    colors.helpbar_fg = Color::RGB(117, 113, 94);  // #75715e
    colors.helpbar_key = Color::RGB(166, 226, 46); // #a6e22e

    colors.keyword = Color::RGB(249, 38, 114);        // #f92672
    colors.string = Color::RGB(230, 219, 116);        // #e6db74
    colors.comment = Color::RGB(117, 113, 94);        // #75715e
    colors.number = Color::RGB(174, 129, 255);        // #ae81ff
    colors.function = Color::RGB(166, 226, 46);       // #a6e22e
    colors.type = Color::RGB(102, 217, 239);          // #66d9ef
    colors.operator_color = Color::RGB(249, 38, 114); // #f92672

    colors.error = Color::RGB(249, 38, 114);   // #f92672
    colors.warning = Color::RGB(253, 151, 31); // #fd971f
    colors.info = Color::RGB(102, 217, 239);   // #66d9ef
    colors.success = Color::RGB(166, 226, 46); // #a6e22e

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(45, 45, 45);          // #2d2d2d
    colors.dialog_fg = Color::RGB(248, 248, 242);       // #f8f8f2
    colors.dialog_title_bg = Color::RGB(73, 72, 62);    // #49483e
    colors.dialog_title_fg = Color::RGB(248, 248, 242); // #f8f8f2
    colors.dialog_border = Color::RGB(117, 113, 94);    // #75715e

    return colors;
}

ThemeColors Theme::Dracula() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 42, 54);             // #282a36
    colors.foreground = Color::RGB(248, 248, 242);          // #f8f8f2
    colors.current_line = Color::RGB(68, 71, 90);           // #44475a
    colors.selection = Color::RGB(68, 71, 90);              // #44475a
    colors.line_number = Color::RGB(98, 114, 164);          // #6272a4
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff

    colors.statusbar_bg = Color::RGB(68, 71, 90);    // #44475a
    colors.statusbar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.menubar_bg = Color::RGB(30, 31, 40);    // #1e1f28
    colors.menubar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.helpbar_bg = Color::RGB(68, 71, 90);    // #44475a
    colors.helpbar_fg = Color::RGB(189, 147, 249); // #bd93f9
    colors.helpbar_key = Color::RGB(80, 250, 123); // #50fa7b

    colors.keyword = Color::RGB(255, 121, 198);        // #ff79c6
    colors.string = Color::RGB(241, 250, 140);         // #f1fa8c
    colors.comment = Color::RGB(98, 114, 164);         // #6272a4
    colors.number = Color::RGB(189, 147, 249);         // #bd93f9
    colors.function = Color::RGB(80, 250, 123);        // #50fa7b
    colors.type = Color::RGB(139, 233, 253);           // #8be9fd
    colors.operator_color = Color::RGB(255, 121, 198); // #ff79c6

    colors.error = Color::RGB(255, 85, 85);     // #ff5555
    colors.warning = Color::RGB(255, 184, 108); // #ffb86c
    colors.info = Color::RGB(139, 233, 253);    // #8be9fd
    colors.success = Color::RGB(80, 250, 123);  // #50fa7b

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(68, 71, 90);          // #44475a
    colors.dialog_fg = Color::RGB(248, 248, 242);       // #f8f8f2
    colors.dialog_title_bg = Color::RGB(40, 42, 54);    // #282a36
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(98, 114, 164);    // #6272a4

    return colors;
}

ThemeColors Theme::SolarizedDark() {
    ThemeColors colors;
    colors.background = Color::RGB(0, 43, 54);              // #002b36
    colors.foreground = Color::RGB(131, 148, 150);          // #839496
    colors.current_line = Color::RGB(7, 54, 66);            // #073642
    colors.selection = Color::RGB(7, 54, 66);               // #073642
    colors.line_number = Color::RGB(88, 110, 117);          // #586e75
    colors.line_number_current = Color::RGB(147, 161, 161); // #93a1a1

    colors.statusbar_bg = Color::RGB(7, 54, 66);     // #073642
    colors.statusbar_fg = Color::RGB(131, 148, 150); // #839496

    colors.menubar_bg = Color::RGB(0, 43, 54);     // #002b36
    colors.menubar_fg = Color::RGB(131, 148, 150); // #839496

    colors.helpbar_bg = Color::RGB(7, 54, 66);    // #073642
    colors.helpbar_fg = Color::RGB(88, 110, 117); // #586e75
    colors.helpbar_key = Color::RGB(133, 153, 0); // #859900

    colors.keyword = Color::RGB(203, 75, 22);         // #cb4b16
    colors.string = Color::RGB(42, 161, 152);         // #2aa198
    colors.comment = Color::RGB(88, 110, 117);        // #586e75
    colors.number = Color::RGB(108, 113, 196);        // #6c71c4
    colors.function = Color::RGB(133, 153, 0);        // #859900
    colors.type = Color::RGB(181, 137, 0);            // #b58900
    colors.operator_color = Color::RGB(38, 139, 210); // #268bd2

    colors.error = Color::RGB(220, 50, 47);   // #dc322f
    colors.warning = Color::RGB(203, 75, 22); // #cb4b16
    colors.info = Color::RGB(38, 139, 210);   // #268bd2
    colors.success = Color::RGB(133, 153, 0); // #859900

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(7, 54, 66);           // #073642
    colors.dialog_fg = Color::RGB(131, 148, 150);       // #839496
    colors.dialog_title_bg = Color::RGB(0, 43, 54);     // #002b36
    colors.dialog_title_fg = Color::RGB(147, 161, 161); // #93a1a1
    colors.dialog_border = Color::RGB(88, 110, 117);    // #586e75

    return colors;
}

ThemeColors Theme::SolarizedLight() {
    ThemeColors colors;
    colors.background = Color::RGB(253, 246, 227);         // #fdf6e3
    colors.foreground = Color::RGB(101, 123, 131);         // #657b83
    colors.current_line = Color::RGB(238, 232, 213);       // #eee8d5
    colors.selection = Color::RGB(238, 232, 213);          // #eee8d5
    colors.line_number = Color::RGB(147, 161, 161);        // #93a1a1
    colors.line_number_current = Color::RGB(88, 110, 117); // #586e75

    colors.statusbar_bg = Color::RGB(238, 232, 213); // #eee8d5
    colors.statusbar_fg = Color::RGB(101, 123, 131); // #657b83

    colors.menubar_bg = Color::RGB(253, 246, 227); // #fdf6e3
    colors.menubar_fg = Color::RGB(101, 123, 131); // #657b83

    colors.helpbar_bg = Color::RGB(238, 232, 213); // #eee8d5
    colors.helpbar_fg = Color::RGB(147, 161, 161); // #93a1a1
    colors.helpbar_key = Color::RGB(133, 153, 0);  // #859900

    colors.keyword = Color::RGB(203, 75, 22);         // #cb4b16
    colors.string = Color::RGB(42, 161, 152);         // #2aa198
    colors.comment = Color::RGB(147, 161, 161);       // #93a1a1
    colors.number = Color::RGB(108, 113, 196);        // #6c71c4
    colors.function = Color::RGB(133, 153, 0);        // #859900
    colors.type = Color::RGB(181, 137, 0);            // #b58900
    colors.operator_color = Color::RGB(38, 139, 210); // #268bd2

    colors.error = Color::RGB(220, 50, 47);   // #dc322f
    colors.warning = Color::RGB(203, 75, 22); // #cb4b16
    colors.info = Color::RGB(38, 139, 210);   // #268bd2
    colors.success = Color::RGB(133, 153, 0); // #859900

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(238, 232, 213);       // #eee8d5
    colors.dialog_fg = Color::RGB(101, 123, 131);       // #657b83
    colors.dialog_title_bg = Color::RGB(253, 246, 227); // #fdf6e3
    colors.dialog_title_fg = Color::RGB(88, 110, 117);  // #586e75
    colors.dialog_border = Color::RGB(147, 161, 161);   // #93a1a1

    return colors;
}

ThemeColors Theme::OneDark() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 44, 52);             // #282c34
    colors.foreground = Color::RGB(171, 178, 191);          // #abb2bf
    colors.current_line = Color::RGB(44, 48, 57);           // #2c3039
    colors.selection = Color::RGB(56, 61, 72);              // #383d48
    colors.line_number = Color::RGB(92, 99, 112);           // #5c6370
    colors.line_number_current = Color::RGB(171, 178, 191); // #abb2bf

    colors.statusbar_bg = Color::RGB(33, 37, 43);    // #21252b
    colors.statusbar_fg = Color::RGB(171, 178, 191); // #abb2bf

    colors.menubar_bg = Color::RGB(33, 37, 43);    // #21252b
    colors.menubar_fg = Color::RGB(171, 178, 191); // #abb2bf

    colors.helpbar_bg = Color::RGB(33, 37, 43);     // #21252b
    colors.helpbar_fg = Color::RGB(92, 99, 112);    // #5c6370
    colors.helpbar_key = Color::RGB(152, 195, 121); // #98c379

    colors.keyword = Color::RGB(198, 120, 221);       // #c678dd
    colors.string = Color::RGB(152, 195, 121);        // #98c379
    colors.comment = Color::RGB(92, 99, 112);         // #5c6370
    colors.number = Color::RGB(209, 154, 102);        // #d19a66
    colors.function = Color::RGB(97, 175, 239);       // #61afef
    colors.type = Color::RGB(229, 192, 123);          // #e5c07b
    colors.operator_color = Color::RGB(86, 182, 194); // #56b6c2

    colors.error = Color::RGB(224, 108, 117);   // #e06c75
    colors.warning = Color::RGB(229, 192, 123); // #e5c07b
    colors.info = Color::RGB(97, 175, 239);     // #61afef
    colors.success = Color::RGB(152, 195, 121); // #98c379

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(44, 48, 57);          // #2c3039
    colors.dialog_fg = Color::RGB(171, 178, 191);       // #abb2bf
    colors.dialog_title_bg = Color::RGB(33, 37, 43);    // #21252b
    colors.dialog_title_fg = Color::RGB(171, 178, 191); // #abb2bf
    colors.dialog_border = Color::RGB(92, 99, 112);     // #5c6370

    return colors;
}

ThemeColors Theme::Nord() {
    ThemeColors colors;
    colors.background = Color::RGB(46, 52, 64);             // #2e3440
    colors.foreground = Color::RGB(216, 222, 233);          // #d8dee9
    colors.current_line = Color::RGB(59, 66, 82);           // #3b4252
    colors.selection = Color::RGB(59, 66, 82);              // #3b4252
    colors.line_number = Color::RGB(76, 86, 106);           // #4c566a
    colors.line_number_current = Color::RGB(216, 222, 233); // #d8dee9

    colors.statusbar_bg = Color::RGB(59, 66, 82);    // #3b4252
    colors.statusbar_fg = Color::RGB(216, 222, 233); // #d8dee9

    colors.menubar_bg = Color::RGB(46, 52, 64);    // #2e3440
    colors.menubar_fg = Color::RGB(216, 222, 233); // #d8dee9

    colors.helpbar_bg = Color::RGB(59, 66, 82);     // #3b4252
    colors.helpbar_fg = Color::RGB(76, 86, 106);    // #4c566a
    colors.helpbar_key = Color::RGB(163, 190, 140); // #a3be8c

    colors.keyword = Color::RGB(129, 161, 193);        // #81a1c1
    colors.string = Color::RGB(163, 190, 140);         // #a3be8c
    colors.comment = Color::RGB(76, 86, 106);          // #4c566a
    colors.number = Color::RGB(180, 142, 173);         // #b48ead
    colors.function = Color::RGB(136, 192, 208);       // #88c0d0
    colors.type = Color::RGB(235, 203, 139);           // #ebcb8b
    colors.operator_color = Color::RGB(129, 161, 193); // #81a1c1

    colors.error = Color::RGB(191, 97, 106);    // #bf616a
    colors.warning = Color::RGB(235, 203, 139); // #ebcb8b
    colors.info = Color::RGB(136, 192, 208);    // #88c0d0
    colors.success = Color::RGB(163, 190, 140); // #a3be8c

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(59, 66, 82);          // #3b4252
    colors.dialog_fg = Color::RGB(216, 222, 233);       // #d8dee9
    colors.dialog_title_bg = Color::RGB(46, 52, 64);    // #2e3440
    colors.dialog_title_fg = Color::RGB(216, 222, 233); // #d8dee9
    colors.dialog_border = Color::RGB(76, 86, 106);     // #4c566a

    return colors;
}

ThemeColors Theme::Gruvbox() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 40, 40);             // #282828
    colors.foreground = Color::RGB(235, 219, 178);          // #ebdbb2
    colors.current_line = Color::RGB(60, 56, 54);           // #3c3836
    colors.selection = Color::RGB(60, 56, 54);              // #3c3836
    colors.line_number = Color::RGB(124, 111, 100);         // #7c6f64
    colors.line_number_current = Color::RGB(235, 219, 178); // #ebdbb2

    colors.statusbar_bg = Color::RGB(50, 48, 47);    // #32302f
    colors.statusbar_fg = Color::RGB(235, 219, 178); // #ebdbb2

    colors.menubar_bg = Color::RGB(40, 40, 40);    // #282828
    colors.menubar_fg = Color::RGB(235, 219, 178); // #ebdbb2

    colors.helpbar_bg = Color::RGB(50, 48, 47);    // #32302f
    colors.helpbar_fg = Color::RGB(124, 111, 100); // #7c6f64
    colors.helpbar_key = Color::RGB(152, 151, 26); // #98971a

    colors.keyword = Color::RGB(251, 73, 52);        // #fb4934
    colors.string = Color::RGB(184, 187, 38);        // #b8bb26
    colors.comment = Color::RGB(146, 131, 116);      // #928374
    colors.number = Color::RGB(211, 134, 155);       // #d3869b
    colors.function = Color::RGB(250, 189, 47);      // #fabd2f
    colors.type = Color::RGB(131, 165, 152);         // #83a598
    colors.operator_color = Color::RGB(251, 73, 52); // #fb4934

    colors.error = Color::RGB(251, 73, 52);    // #fb4934
    colors.warning = Color::RGB(250, 189, 47); // #fabd2f
    colors.info = Color::RGB(131, 165, 152);   // #83a598
    colors.success = Color::RGB(184, 187, 38); // #b8bb26

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(50, 48, 47);          // #32302f
    colors.dialog_fg = Color::RGB(235, 219, 178);       // #ebdbb2
    colors.dialog_title_bg = Color::RGB(40, 40, 40);    // #282828
    colors.dialog_title_fg = Color::RGB(235, 219, 178); // #ebdbb2
    colors.dialog_border = Color::RGB(124, 111, 100);   // #7c6f64

    return colors;
}

ThemeColors Theme::TokyoNight() {
    ThemeColors colors;
    colors.background = Color::RGB(26, 27, 38);             // #1a1b26
    colors.foreground = Color::RGB(192, 202, 245);          // #c0caf5
    colors.current_line = Color::RGB(36, 40, 59);           // #24283b
    colors.selection = Color::RGB(36, 40, 59);              // #24283b
    colors.line_number = Color::RGB(99, 104, 135);          // #63688f
    colors.line_number_current = Color::RGB(192, 202, 245); // #c0caf5

    colors.statusbar_bg = Color::RGB(36, 40, 59);    // #24283b
    colors.statusbar_fg = Color::RGB(192, 202, 245); // #c0caf5

    colors.menubar_bg = Color::RGB(26, 27, 38);    // #1a1b26
    colors.menubar_fg = Color::RGB(192, 202, 245); // #c0caf5

    colors.helpbar_bg = Color::RGB(36, 40, 59);     // #24283b
    colors.helpbar_fg = Color::RGB(99, 104, 135);   // #63688f
    colors.helpbar_key = Color::RGB(125, 207, 255); // #7dcfff

    colors.keyword = Color::RGB(187, 154, 247);        // #bb9af7
    colors.string = Color::RGB(158, 206, 106);         // #9ece6a
    colors.comment = Color::RGB(99, 104, 135);         // #63688f
    colors.number = Color::RGB(255, 184, 108);         // #ffb86c
    colors.function = Color::RGB(125, 207, 255);       // #7dcfff
    colors.type = Color::RGB(122, 162, 247);           // #7aa2f7
    colors.operator_color = Color::RGB(187, 154, 247); // #bb9af7

    colors.error = Color::RGB(247, 118, 142);   // #f7768e
    colors.warning = Color::RGB(255, 184, 108); // #ffb86c
    colors.info = Color::RGB(125, 207, 255);    // #7dcfff
    colors.success = Color::RGB(158, 206, 106); // #9ece6a

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(36, 40, 59);          // #24283b
    colors.dialog_fg = Color::RGB(192, 202, 245);       // #c0caf5
    colors.dialog_title_bg = Color::RGB(26, 27, 38);    // #1a1b26
    colors.dialog_title_fg = Color::RGB(192, 202, 245); // #c0caf5
    colors.dialog_border = Color::RGB(99, 104, 135);    // #63688f

    return colors;
}

ThemeColors Theme::Catppuccin() {
    ThemeColors colors;
    colors.background = Color::RGB(30, 30, 46);             // #1e1e2e
    colors.foreground = Color::RGB(205, 214, 244);          // #cdd6f4
    colors.current_line = Color::RGB(49, 50, 68);           // #313244
    colors.selection = Color::RGB(49, 50, 68);              // #313244
    colors.line_number = Color::RGB(108, 112, 134);         // #6c7086
    colors.line_number_current = Color::RGB(205, 214, 244); // #cdd6f4

    colors.statusbar_bg = Color::RGB(24, 24, 37);    // #181825
    colors.statusbar_fg = Color::RGB(205, 214, 244); // #cdd6f4

    colors.menubar_bg = Color::RGB(30, 30, 46);    // #1e1e2e
    colors.menubar_fg = Color::RGB(205, 214, 244); // #cdd6f4

    colors.helpbar_bg = Color::RGB(24, 24, 37);     // #181825
    colors.helpbar_fg = Color::RGB(108, 112, 134);  // #6c7086
    colors.helpbar_key = Color::RGB(166, 227, 161); // #a6e3a1

    colors.keyword = Color::RGB(203, 166, 247);        // #cba6f7
    colors.string = Color::RGB(166, 227, 161);         // #a6e3a1
    colors.comment = Color::RGB(108, 112, 134);        // #6c7086
    colors.number = Color::RGB(250, 179, 135);         // #fab387
    colors.function = Color::RGB(137, 220, 235);       // #89dceb
    colors.type = Color::RGB(148, 226, 213);           // #94e2d5
    colors.operator_color = Color::RGB(203, 166, 247); // #cba6f7

    colors.error = Color::RGB(243, 139, 168);   // #f38ba8
    colors.warning = Color::RGB(250, 179, 135); // #fab387
    colors.info = Color::RGB(137, 220, 235);    // #89dceb
    colors.success = Color::RGB(166, 227, 161); // #a6e3a1

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(49, 50, 68);          // #313244
    colors.dialog_fg = Color::RGB(205, 214, 244);       // #cdd6f4
    colors.dialog_title_bg = Color::RGB(30, 30, 46);    // #1e1e2e
    colors.dialog_title_fg = Color::RGB(205, 214, 244); // #cdd6f4
    colors.dialog_border = Color::RGB(108, 112, 134);   // #6c7086

    return colors;
}

ThemeColors Theme::Material() {
    ThemeColors colors;
    colors.background = Color::RGB(38, 50, 56);             // #263238
    colors.foreground = Color::RGB(238, 255, 255);          // #eeffff
    colors.current_line = Color::RGB(38, 50, 56);           // #263238
    colors.selection = Color::RGB(55, 71, 79);              // #37474f
    colors.line_number = Color::RGB(84, 110, 122);          // #546e7a
    colors.line_number_current = Color::RGB(238, 255, 255); // #eeffff

    colors.statusbar_bg = Color::RGB(38, 50, 56);    // #263238
    colors.statusbar_fg = Color::RGB(238, 255, 255); // #eeffff

    colors.menubar_bg = Color::RGB(38, 50, 56);    // #263238
    colors.menubar_fg = Color::RGB(238, 255, 255); // #eeffff

    colors.helpbar_bg = Color::RGB(38, 50, 56);     // #263238
    colors.helpbar_fg = Color::RGB(84, 110, 122);   // #546e7a
    colors.helpbar_key = Color::RGB(195, 232, 141); // #c3e88d

    colors.keyword = Color::RGB(199, 146, 234);        // #c792ea
    colors.string = Color::RGB(195, 232, 141);         // #c3e88d
    colors.comment = Color::RGB(84, 110, 122);         // #546e7a
    colors.number = Color::RGB(247, 140, 108);         // #f78c6c
    colors.function = Color::RGB(130, 170, 255);       // #82aaff
    colors.type = Color::RGB(255, 203, 107);           // #ffcb6b
    colors.operator_color = Color::RGB(199, 146, 234); // #c792ea

    colors.error = Color::RGB(255, 83, 112);    // #ff5370
    colors.warning = Color::RGB(255, 203, 107); // #ffcb6b
    colors.info = Color::RGB(130, 170, 255);    // #82aaff
    colors.success = Color::RGB(195, 232, 141); // #c3e88d

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(55, 71, 79);          // #37474f
    colors.dialog_fg = Color::RGB(238, 255, 255);       // #eeffff
    colors.dialog_title_bg = Color::RGB(38, 50, 56);    // #263238
    colors.dialog_title_fg = Color::RGB(238, 255, 255); // #eeffff
    colors.dialog_border = Color::RGB(84, 110, 122);    // #546e7a

    return colors;
}

ThemeColors Theme::Ayu() {
    ThemeColors colors;
    colors.background = Color::RGB(15, 23, 42);             // #0f172a
    colors.foreground = Color::RGB(203, 213, 225);          // #cbd5e1
    colors.current_line = Color::RGB(30, 41, 59);           // #1e293b
    colors.selection = Color::RGB(30, 41, 59);              // #1e293b
    colors.line_number = Color::RGB(100, 116, 139);         // #64748b
    colors.line_number_current = Color::RGB(203, 213, 225); // #cbd5e1

    colors.statusbar_bg = Color::RGB(15, 23, 42);    // #0f172a
    colors.statusbar_fg = Color::RGB(203, 213, 225); // #cbd5e1

    colors.menubar_bg = Color::RGB(15, 23, 42);    // #0f172a
    colors.menubar_fg = Color::RGB(203, 213, 225); // #cbd5e1

    colors.helpbar_bg = Color::RGB(15, 23, 42);    // #0f172a
    colors.helpbar_fg = Color::RGB(100, 116, 139); // #64748b
    colors.helpbar_key = Color::RGB(34, 197, 94);  // #22c55e

    colors.keyword = Color::RGB(139, 92, 246);        // #8b5cf6
    colors.string = Color::RGB(34, 197, 94);          // #22c55e
    colors.comment = Color::RGB(100, 116, 139);       // #64748b
    colors.number = Color::RGB(251, 146, 60);         // #fb923c
    colors.function = Color::RGB(59, 130, 246);       // #3b82f6
    colors.type = Color::RGB(251, 191, 36);           // #fbbf24
    colors.operator_color = Color::RGB(139, 92, 246); // #8b5cf6

    colors.error = Color::RGB(239, 68, 68);    // #ef4444
    colors.warning = Color::RGB(251, 146, 60); // #fb923c
    colors.info = Color::RGB(59, 130, 246);    // #3b82f6
    colors.success = Color::RGB(34, 197, 94);  // #22c55e

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(30, 41, 59);          // #1e293b
    colors.dialog_fg = Color::RGB(203, 213, 225);       // #cbd5e1
    colors.dialog_title_bg = Color::RGB(15, 23, 42);    // #0f172a
    colors.dialog_title_fg = Color::RGB(203, 213, 225); // #cbd5e1
    colors.dialog_border = Color::RGB(100, 116, 139);   // #64748b

    return colors;
}

ThemeColors Theme::GitHub() {
    ThemeColors colors;
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
    colors.dialog_bg = Color::RGB(246, 248, 250);       // #f6f8fa
    colors.dialog_fg = Color::RGB(36, 41, 46);          // #24292e
    colors.dialog_title_bg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_title_fg = Color::RGB(36, 41, 46);    // #24292e
    colors.dialog_border = Color::RGB(106, 115, 125);   // #6a737d

    return colors;
}

ThemeColors Theme::VSCodeDark() {
    ThemeColors colors;
    colors.background = Color::RGB(30, 30, 30);             // #1e1e1e
    colors.foreground = Color::RGB(212, 212, 212);          // #d4d4d4
    colors.current_line = Color::RGB(42, 42, 42);           // #2a2a2a
    colors.selection = Color::RGB(38, 79, 120);             // #264e7a
    colors.line_number = Color::RGB(133, 133, 133);         // #858585
    colors.line_number_current = Color::RGB(212, 212, 212); // #d4d4d4

    colors.statusbar_bg = Color::RGB(0, 122, 204);   // #007acc
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(37, 37, 38);    // #252526
    colors.menubar_fg = Color::RGB(204, 204, 204); // #cccccc

    colors.helpbar_bg = Color::RGB(37, 37, 38);    // #252526
    colors.helpbar_fg = Color::RGB(133, 133, 133); // #858585
    colors.helpbar_key = Color::RGB(78, 148, 206); // #4e94ce

    colors.keyword = Color::RGB(86, 156, 214);         // #569cd6
    colors.string = Color::RGB(206, 145, 120);         // #ce9178
    colors.comment = Color::RGB(106, 153, 85);         // #6a9955
    colors.number = Color::RGB(181, 206, 168);         // #b5cea8
    colors.function = Color::RGB(220, 220, 170);       // #dcdcaa
    colors.type = Color::RGB(78, 201, 176);            // #4ec9b0
    colors.operator_color = Color::RGB(212, 212, 212); // #d4d4d4

    colors.error = Color::RGB(244, 63, 94);     // #f43f5e
    colors.warning = Color::RGB(255, 184, 108); // #ffb86c
    colors.info = Color::RGB(78, 148, 206);     // #4e94ce
    colors.success = Color::RGB(106, 153, 85);  // #6a9955

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(37, 37, 38);          // #252526
    colors.dialog_fg = Color::RGB(212, 212, 212);       // #d4d4d4
    colors.dialog_title_bg = Color::RGB(30, 30, 30);    // #1e1e1e
    colors.dialog_title_fg = Color::RGB(212, 212, 212); // #d4d4d4
    colors.dialog_border = Color::RGB(133, 133, 133);   // #858585

    return colors;
}

ThemeColors Theme::NightOwl() {
    ThemeColors colors;
    colors.background = Color::RGB(1, 22, 39);              // #011627
    colors.foreground = Color::RGB(214, 222, 235);          // #d6deeb
    colors.current_line = Color::RGB(22, 40, 54);           // #162836
    colors.selection = Color::RGB(33, 60, 81);              // #213c51
    colors.line_number = Color::RGB(99, 119, 142);          // #63778e
    colors.line_number_current = Color::RGB(214, 222, 235); // #d6deeb

    colors.statusbar_bg = Color::RGB(1, 22, 39);     // #011627
    colors.statusbar_fg = Color::RGB(214, 222, 235); // #d6deeb

    colors.menubar_bg = Color::RGB(1, 22, 39);     // #011627
    colors.menubar_fg = Color::RGB(214, 222, 235); // #d6deeb

    colors.helpbar_bg = Color::RGB(1, 22, 39);      // #011627
    colors.helpbar_fg = Color::RGB(99, 119, 142);   // #63778e
    colors.helpbar_key = Color::RGB(130, 170, 255); // #82aaff

    colors.keyword = Color::RGB(199, 146, 234);        // #c792ea
    colors.string = Color::RGB(195, 232, 141);         // #c3e88d
    colors.comment = Color::RGB(99, 119, 142);         // #63778e
    colors.number = Color::RGB(247, 140, 108);         // #f78c6c
    colors.function = Color::RGB(130, 170, 255);       // #82aaff
    colors.type = Color::RGB(255, 203, 107);           // #ffcb6b
    colors.operator_color = Color::RGB(199, 146, 234); // #c792ea

    colors.error = Color::RGB(255, 83, 112);    // #ff5370
    colors.warning = Color::RGB(255, 203, 107); // #ffcb6b
    colors.info = Color::RGB(130, 170, 255);    // #82aaff
    colors.success = Color::RGB(195, 232, 141); // #c3e88d

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(22, 40, 54);          // #162836
    colors.dialog_fg = Color::RGB(214, 222, 235);       // #d6deeb
    colors.dialog_title_bg = Color::RGB(1, 22, 39);     // #011627
    colors.dialog_title_fg = Color::RGB(214, 222, 235); // #d6deeb
    colors.dialog_border = Color::RGB(99, 119, 142);    // #63778e

    return colors;
}

ThemeColors Theme::Palenight() {
    ThemeColors colors;
    colors.background = Color::RGB(41, 45, 62);             // #292d3e
    colors.foreground = Color::RGB(200, 200, 200);          // #c8c8c8
    colors.current_line = Color::RGB(35, 38, 52);           // #232634
    colors.selection = Color::RGB(55, 59, 82);              // #373b52
    colors.line_number = Color::RGB(95, 99, 122);           // #5f637a
    colors.line_number_current = Color::RGB(200, 200, 200); // #c8c8c8

    colors.statusbar_bg = Color::RGB(35, 38, 52);    // #232634
    colors.statusbar_fg = Color::RGB(200, 200, 200); // #c8c8c8

    colors.menubar_bg = Color::RGB(41, 45, 62);    // #292d3e
    colors.menubar_fg = Color::RGB(200, 200, 200); // #c8c8c8

    colors.helpbar_bg = Color::RGB(35, 38, 52);     // #232634
    colors.helpbar_fg = Color::RGB(95, 99, 122);    // #5f637a
    colors.helpbar_key = Color::RGB(195, 232, 141); // #c3e88d

    colors.keyword = Color::RGB(199, 146, 234);        // #c792ea
    colors.string = Color::RGB(195, 232, 141);         // #c3e88d
    colors.comment = Color::RGB(95, 99, 122);          // #5f637a
    colors.number = Color::RGB(247, 140, 108);         // #f78c6c
    colors.function = Color::RGB(130, 170, 255);       // #82aaff
    colors.type = Color::RGB(255, 203, 107);           // #ffcb6b
    colors.operator_color = Color::RGB(199, 146, 234); // #c792ea

    colors.error = Color::RGB(255, 83, 112);    // #ff5370
    colors.warning = Color::RGB(255, 203, 107); // #ffcb6b
    colors.info = Color::RGB(130, 170, 255);    // #82aaff
    colors.success = Color::RGB(195, 232, 141); // #c3e88d

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(35, 38, 52);          // #232634
    colors.dialog_fg = Color::RGB(200, 200, 200);       // #c8c8c8
    colors.dialog_title_bg = Color::RGB(41, 45, 62);    // #292d3e
    colors.dialog_title_fg = Color::RGB(200, 200, 200); // #c8c8c8
    colors.dialog_border = Color::RGB(95, 99, 122);     // #5f637a

    return colors;
}

ThemeColors Theme::OceanicNext() {
    ThemeColors colors;
    colors.background = Color::RGB(23, 43, 58);             // #1b2b34
    colors.foreground = Color::RGB(200, 213, 219);          // #c8d5db
    colors.current_line = Color::RGB(25, 48, 65);           // #193041
    colors.selection = Color::RGB(39, 62, 81);              // #273e51
    colors.line_number = Color::RGB(101, 130, 145);         // #658291
    colors.line_number_current = Color::RGB(200, 213, 219); // #c8d5db

    colors.statusbar_bg = Color::RGB(23, 43, 58);    // #1b2b34
    colors.statusbar_fg = Color::RGB(200, 213, 219); // #c8d5db

    colors.menubar_bg = Color::RGB(23, 43, 58);    // #1b2b34
    colors.menubar_fg = Color::RGB(200, 213, 219); // #c8d5db

    colors.helpbar_bg = Color::RGB(23, 43, 58);     // #1b2b34
    colors.helpbar_fg = Color::RGB(101, 130, 145);  // #658291
    colors.helpbar_key = Color::RGB(152, 195, 121); // #98c379

    colors.keyword = Color::RGB(198, 120, 221);       // #c678dd
    colors.string = Color::RGB(152, 195, 121);        // #98c379
    colors.comment = Color::RGB(101, 130, 145);       // #658291
    colors.number = Color::RGB(250, 189, 47);         // #fabd2f
    colors.function = Color::RGB(97, 175, 239);       // #61afef
    colors.type = Color::RGB(229, 192, 123);          // #e5c07b
    colors.operator_color = Color::RGB(86, 182, 194); // #56b6c2

    colors.error = Color::RGB(231, 76, 60);     // #e74c3c
    colors.warning = Color::RGB(250, 189, 47);  // #fabd2f
    colors.info = Color::RGB(97, 175, 239);     // #61afef
    colors.success = Color::RGB(152, 195, 121); // #98c379

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(25, 48, 65);          // #193041
    colors.dialog_fg = Color::RGB(200, 213, 219);       // #c8d5db
    colors.dialog_title_bg = Color::RGB(23, 43, 58);    // #1b2b34
    colors.dialog_title_fg = Color::RGB(200, 213, 219); // #c8d5db
    colors.dialog_border = Color::RGB(101, 130, 145);   // #658291

    return colors;
}

ThemeColors Theme::Kanagawa() {
    ThemeColors colors;
    colors.background = Color::RGB(31, 31, 40);             // #1f1f28
    colors.foreground = Color::RGB(220, 215, 186);          // #dcd7ba
    colors.current_line = Color::RGB(42, 42, 55);           // #2a2a37
    colors.selection = Color::RGB(45, 79, 103);             // #2d4f67
    colors.line_number = Color::RGB(114, 113, 105);         // #727169
    colors.line_number_current = Color::RGB(220, 215, 186); // #dcd7ba

    colors.statusbar_bg = Color::RGB(31, 31, 40);    // #1f1f28
    colors.statusbar_fg = Color::RGB(220, 215, 186); // #dcd7ba

    colors.menubar_bg = Color::RGB(31, 31, 40);    // #1f1f28
    colors.menubar_fg = Color::RGB(220, 215, 186); // #dcd7ba

    colors.helpbar_bg = Color::RGB(31, 31, 40);     // #1f1f28
    colors.helpbar_fg = Color::RGB(114, 113, 105);  // #727169
    colors.helpbar_key = Color::RGB(126, 156, 216); // #7e9cd8

    colors.keyword = Color::RGB(149, 127, 184);        // #957fb8
    colors.string = Color::RGB(152, 187, 108);         // #98bb6c
    colors.comment = Color::RGB(114, 113, 105);        // #727169
    colors.number = Color::RGB(202, 169, 224);         // #caa9e0
    colors.function = Color::RGB(126, 156, 216);       // #7e9cd8
    colors.type = Color::RGB(127, 180, 202);           // #7fb4ca
    colors.operator_color = Color::RGB(149, 127, 184); // #957fb8

    colors.error = Color::RGB(195, 64, 67);     // #c34043
    colors.warning = Color::RGB(192, 163, 110); // #c0a36e
    colors.info = Color::RGB(126, 156, 216);    // #7e9cd8
    colors.success = Color::RGB(152, 187, 108); // #98bb6c

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(42, 42, 55);          // #2a2a37
    colors.dialog_fg = Color::RGB(220, 215, 186);       // #dcd7ba
    colors.dialog_title_bg = Color::RGB(31, 31, 40);    // #1f1f28
    colors.dialog_title_fg = Color::RGB(220, 215, 186); // #dcd7ba
    colors.dialog_border = Color::RGB(114, 113, 105);   // #727169

    return colors;
}

ThemeColors Theme::TomorrowNight() {
    ThemeColors colors;
    colors.background = Color::RGB(29, 31, 33);             // #1d1f21
    colors.foreground = Color::RGB(197, 200, 198);          // #c5c8c6
    colors.current_line = Color::RGB(40, 42, 46);           // #282a2e
    colors.selection = Color::RGB(55, 59, 65);              // #373b41
    colors.line_number = Color::RGB(150, 152, 150);         // #969896
    colors.line_number_current = Color::RGB(197, 200, 198); // #c5c8c6

    colors.statusbar_bg = Color::RGB(29, 31, 33);    // #1d1f21
    colors.statusbar_fg = Color::RGB(197, 200, 198); // #c5c8c6

    colors.menubar_bg = Color::RGB(29, 31, 33);    // #1d1f21
    colors.menubar_fg = Color::RGB(197, 200, 198); // #c5c8c6

    colors.helpbar_bg = Color::RGB(29, 31, 33);     // #1d1f21
    colors.helpbar_fg = Color::RGB(150, 152, 150);  // #969896
    colors.helpbar_key = Color::RGB(181, 189, 104); // #b5bd68

    colors.keyword = Color::RGB(204, 102, 102);        // #cc6666
    colors.string = Color::RGB(181, 189, 104);         // #b5bd68
    colors.comment = Color::RGB(150, 152, 150);        // #969896
    colors.number = Color::RGB(178, 148, 187);         // #b294bb
    colors.function = Color::RGB(240, 198, 116);       // #f0c674
    colors.type = Color::RGB(102, 153, 204);           // #6699cc
    colors.operator_color = Color::RGB(204, 102, 102); // #cc6666

    colors.error = Color::RGB(204, 102, 102);   // #cc6666
    colors.warning = Color::RGB(240, 198, 116); // #f0c674
    colors.info = Color::RGB(102, 153, 204);    // #6699cc
    colors.success = Color::RGB(181, 189, 104); // #b5bd68

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(40, 42, 46);          // #282a2e
    colors.dialog_fg = Color::RGB(197, 200, 198);       // #c5c8c6
    colors.dialog_title_bg = Color::RGB(29, 31, 33);    // #1d1f21
    colors.dialog_title_fg = Color::RGB(197, 200, 198); // #c5c8c6
    colors.dialog_border = Color::RGB(150, 152, 150);   // #969896

    return colors;
}

ThemeColors Theme::TomorrowNightBlue() {
    ThemeColors colors;
    colors.background = Color::RGB(0, 36, 81);              // #002451
    colors.foreground = Color::RGB(255, 255, 255);          // #ffffff
    colors.current_line = Color::RGB(0, 46, 102);           // #002e66
    colors.selection = Color::RGB(0, 56, 122);              // #00387a
    colors.line_number = Color::RGB(128, 151, 177);         // #8097b1
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff

    colors.statusbar_bg = Color::RGB(0, 36, 81);     // #002451
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(0, 36, 81);     // #002451
    colors.menubar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.helpbar_bg = Color::RGB(0, 36, 81);      // #002451
    colors.helpbar_fg = Color::RGB(128, 151, 177);  // #8097b1
    colors.helpbar_key = Color::RGB(255, 220, 117); // #ffdc75

    colors.keyword = Color::RGB(255, 129, 239);        // #ff81f0
    colors.string = Color::RGB(255, 220, 117);         // #ffdc75
    colors.comment = Color::RGB(128, 151, 177);        // #8097b1
    colors.number = Color::RGB(255, 129, 239);         // #ff81f0
    colors.function = Color::RGB(255, 220, 117);       // #ffdc75
    colors.type = Color::RGB(129, 216, 208);           // #81d8d0
    colors.operator_color = Color::RGB(255, 129, 239); // #ff81f0

    colors.error = Color::RGB(255, 64, 129);    // #ff4081
    colors.warning = Color::RGB(255, 220, 117); // #ffdc75
    colors.info = Color::RGB(129, 216, 208);    // #81d8d0
    colors.success = Color::RGB(204, 255, 0);   // #ccff00

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(0, 46, 102);          // #002e66
    colors.dialog_fg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_title_bg = Color::RGB(0, 36, 81);     // #002451
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(128, 151, 177);   // #8097b1

    return colors;
}

ThemeColors Theme::Cobalt() {
    ThemeColors colors;
    colors.background = Color::RGB(0, 31, 63);              // #001f3f
    colors.foreground = Color::RGB(255, 255, 255);          // #ffffff
    colors.current_line = Color::RGB(0, 41, 82);            // #002952
    colors.selection = Color::RGB(0, 51, 102);              // #003366
    colors.line_number = Color::RGB(128, 160, 192);         // #80a0c0
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff

    colors.statusbar_bg = Color::RGB(0, 31, 63);     // #001f3f
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(0, 31, 63);     // #001f3f
    colors.menubar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.helpbar_bg = Color::RGB(0, 31, 63);      // #001f3f
    colors.helpbar_fg = Color::RGB(128, 160, 192);  // #80a0c0
    colors.helpbar_key = Color::RGB(255, 220, 117); // #ffdc75

    colors.keyword = Color::RGB(255, 157, 0);        // #ff9d00
    colors.string = Color::RGB(61, 153, 112);        // #3d9970
    colors.comment = Color::RGB(128, 160, 192);      // #80a0c0
    colors.number = Color::RGB(255, 220, 117);       // #ffdc75
    colors.function = Color::RGB(255, 220, 117);     // #ffdc75
    colors.type = Color::RGB(255, 157, 0);           // #ff9d00
    colors.operator_color = Color::RGB(255, 157, 0); // #ff9d00

    colors.error = Color::RGB(255, 64, 129);    // #ff4081
    colors.warning = Color::RGB(255, 220, 117); // #ffdc75
    colors.info = Color::RGB(129, 216, 208);    // #81d8d0
    colors.success = Color::RGB(61, 153, 112);  // #3d9970

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(0, 41, 82);           // #002952
    colors.dialog_fg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_title_bg = Color::RGB(0, 31, 63);     // #001f3f
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(128, 160, 192);   // #80a0c0

    return colors;
}

ThemeColors Theme::Zenburn() {
    ThemeColors colors;
    colors.background = Color::RGB(59, 59, 59);             // #3f3f3f
    colors.foreground = Color::RGB(220, 220, 204);          // #dcdccc
    colors.current_line = Color::RGB(73, 73, 73);           // #494949
    colors.selection = Color::RGB(73, 73, 73);              // #494949
    colors.line_number = Color::RGB(112, 112, 112);         // #707070
    colors.line_number_current = Color::RGB(220, 220, 204); // #dcdccc

    colors.statusbar_bg = Color::RGB(59, 59, 59);    // #3f3f3f
    colors.statusbar_fg = Color::RGB(220, 220, 204); // #dcdccc

    colors.menubar_bg = Color::RGB(59, 59, 59);    // #3f3f3f
    colors.menubar_fg = Color::RGB(220, 220, 204); // #dcdccc

    colors.helpbar_bg = Color::RGB(59, 59, 59);     // #3f3f3f
    colors.helpbar_fg = Color::RGB(112, 112, 112);  // #707070
    colors.helpbar_key = Color::RGB(220, 163, 163); // #dca3a3

    colors.keyword = Color::RGB(240, 223, 175);        // #f0dfaf
    colors.string = Color::RGB(220, 163, 163);         // #dca3a3
    colors.comment = Color::RGB(127, 159, 127);        // #7f9f7f
    colors.number = Color::RGB(220, 220, 204);         // #dcdccc
    colors.function = Color::RGB(93, 176, 215);        // #5db0d7
    colors.type = Color::RGB(223, 175, 143);           // #dfaf8f
    colors.operator_color = Color::RGB(240, 223, 175); // #f0dfaf

    colors.error = Color::RGB(204, 102, 102);   // #cc6666
    colors.warning = Color::RGB(223, 175, 143); // #dfaf8f
    colors.info = Color::RGB(93, 176, 215);     // #5db0d7
    colors.success = Color::RGB(127, 159, 127); // #7f9f7f

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(73, 73, 73);          // #494949
    colors.dialog_fg = Color::RGB(220, 220, 204);       // #dcdccc
    colors.dialog_title_bg = Color::RGB(59, 59, 59);    // #3f3f3f
    colors.dialog_title_fg = Color::RGB(220, 220, 204); // #dcdccc
    colors.dialog_border = Color::RGB(112, 112, 112);   // #707070

    return colors;
}

ThemeColors Theme::Base16Dark() {
    ThemeColors colors;
    colors.background = Color::RGB(21, 21, 21);             // #151515
    colors.foreground = Color::RGB(208, 208, 208);          // #d0d0d0
    colors.current_line = Color::RGB(40, 40, 40);           // #282828
    colors.selection = Color::RGB(48, 48, 48);              // #303030
    colors.line_number = Color::RGB(80, 80, 80);            // #505050
    colors.line_number_current = Color::RGB(208, 208, 208); // #d0d0d0

    colors.statusbar_bg = Color::RGB(21, 21, 21);    // #151515
    colors.statusbar_fg = Color::RGB(208, 208, 208); // #d0d0d0

    colors.menubar_bg = Color::RGB(21, 21, 21);    // #151515
    colors.menubar_fg = Color::RGB(208, 208, 208); // #d0d0d0

    colors.helpbar_bg = Color::RGB(21, 21, 21);   // #151515
    colors.helpbar_fg = Color::RGB(80, 80, 80);   // #505050
    colors.helpbar_key = Color::RGB(181, 137, 0); // #b58900

    colors.keyword = Color::RGB(211, 54, 130);        // #d33682
    colors.string = Color::RGB(42, 161, 152);         // #2aa198
    colors.comment = Color::RGB(93, 93, 93);          // #5d5d5d
    colors.number = Color::RGB(108, 113, 196);        // #6c71c4
    colors.function = Color::RGB(38, 139, 210);       // #268bd2
    colors.type = Color::RGB(181, 137, 0);            // #b58900
    colors.operator_color = Color::RGB(211, 54, 130); // #d33682

    colors.error = Color::RGB(220, 50, 47);   // #dc322f
    colors.warning = Color::RGB(203, 75, 22); // #cb4b16
    colors.info = Color::RGB(38, 139, 210);   // #268bd2
    colors.success = Color::RGB(133, 153, 0); // #859900

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(40, 40, 40);          // #282828
    colors.dialog_fg = Color::RGB(208, 208, 208);       // #d0d0d0
    colors.dialog_title_bg = Color::RGB(21, 21, 21);    // #151515
    colors.dialog_title_fg = Color::RGB(208, 208, 208); // #d0d0d0
    colors.dialog_border = Color::RGB(80, 80, 80);      // #505050

    return colors;
}

ThemeColors Theme::PaperColor() {
    ThemeColors colors;
    colors.background = Color::RGB(28, 28, 28);             // #1c1c1c
    colors.foreground = Color::RGB(212, 212, 212);          // #d4d4d4
    colors.current_line = Color::RGB(40, 40, 40);           // #282828
    colors.selection = Color::RGB(58, 58, 58);              // #3a3a3a
    colors.line_number = Color::RGB(136, 136, 136);         // #888888
    colors.line_number_current = Color::RGB(212, 212, 212); // #d4d4d4

    colors.statusbar_bg = Color::RGB(40, 40, 40);    // #282828
    colors.statusbar_fg = Color::RGB(212, 212, 212); // #d4d4d4

    colors.menubar_bg = Color::RGB(28, 28, 28);    // #1c1c1c
    colors.menubar_fg = Color::RGB(212, 212, 212); // #d4d4d4

    colors.helpbar_bg = Color::RGB(40, 40, 40);    // #282828
    colors.helpbar_fg = Color::RGB(136, 136, 136); // #888888
    colors.helpbar_key = Color::RGB(95, 135, 0);   // #5f8700

    colors.keyword = Color::RGB(175, 0, 219);        // #af00db
    colors.string = Color::RGB(0, 135, 0);           // #008700
    colors.comment = Color::RGB(124, 124, 124);      // #7c7c7c
    colors.number = Color::RGB(0, 135, 175);         // #0087af
    colors.function = Color::RGB(0, 0, 215);         // #0000d7
    colors.type = Color::RGB(95, 135, 0);            // #5f8700
    colors.operator_color = Color::RGB(175, 0, 219); // #af00db

    colors.error = Color::RGB(215, 0, 0);    // #d70000
    colors.warning = Color::RGB(215, 95, 0); // #d75f00
    colors.info = Color::RGB(0, 0, 215);     // #0000d7
    colors.success = Color::RGB(0, 135, 0);  // #008700

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(40, 40, 40);          // #282828
    colors.dialog_fg = Color::RGB(212, 212, 212);       // #d4d4d4
    colors.dialog_title_bg = Color::RGB(28, 28, 28);    // #1c1c1c
    colors.dialog_title_fg = Color::RGB(212, 212, 212); // #d4d4d4
    colors.dialog_border = Color::RGB(136, 136, 136);   // #888888

    return colors;
}

ThemeColors Theme::RosePine() {
    ThemeColors colors;
    colors.background = Color::RGB(25, 23, 36);             // #191724
    colors.foreground = Color::RGB(224, 222, 244);          // #e0def4
    colors.current_line = Color::RGB(31, 29, 46);           // #1f1d2e
    colors.selection = Color::RGB(64, 61, 88);              // #403d58
    colors.line_number = Color::RGB(110, 106, 134);         // #6e6a86
    colors.line_number_current = Color::RGB(224, 222, 244); // #e0def4

    colors.statusbar_bg = Color::RGB(31, 29, 46);    // #1f1d2e
    colors.statusbar_fg = Color::RGB(224, 222, 244); // #e0def4

    colors.menubar_bg = Color::RGB(25, 23, 36);    // #191724
    colors.menubar_fg = Color::RGB(224, 222, 244); // #e0def4

    colors.helpbar_bg = Color::RGB(31, 29, 46);     // #1f1d2e
    colors.helpbar_fg = Color::RGB(110, 106, 134);  // #6e6a86
    colors.helpbar_key = Color::RGB(156, 207, 216); // #9ccfd8

    colors.keyword = Color::RGB(235, 111, 146);        // #eb6f92
    colors.string = Color::RGB(156, 207, 216);         // #9ccfd8
    colors.comment = Color::RGB(110, 106, 134);        // #6e6a86
    colors.number = Color::RGB(246, 193, 119);         // #f6c177
    colors.function = Color::RGB(156, 207, 216);       // #9ccfd8
    colors.type = Color::RGB(196, 167, 231);           // #c4a7e7
    colors.operator_color = Color::RGB(235, 111, 146); // #eb6f92

    colors.error = Color::RGB(235, 111, 146);   // #eb6f92
    colors.warning = Color::RGB(246, 193, 119); // #f6c177
    colors.info = Color::RGB(156, 207, 216);    // #9ccfd8
    colors.success = Color::RGB(156, 207, 216); // #9ccfd8

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(31, 29, 46);          // #1f1d2e
    colors.dialog_fg = Color::RGB(224, 222, 244);       // #e0def4
    colors.dialog_title_bg = Color::RGB(25, 23, 36);    // #191724
    colors.dialog_title_fg = Color::RGB(224, 222, 244); // #e0def4
    colors.dialog_border = Color::RGB(110, 106, 134);   // #6e6a86

    return colors;
}

ThemeColors Theme::Everforest() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 46, 52);             // #2d353b
    colors.foreground = Color::RGB(220, 215, 195);          // #d3c6aa
    colors.current_line = Color::RGB(52, 59, 66);           // #343f46
    colors.selection = Color::RGB(64, 73, 82);              // #404850
    colors.line_number = Color::RGB(124, 136, 148);         // #7c8898
    colors.line_number_current = Color::RGB(220, 215, 195); // #d3c6aa

    colors.statusbar_bg = Color::RGB(40, 46, 52);    // #2d353b
    colors.statusbar_fg = Color::RGB(220, 215, 195); // #d3c6aa

    colors.menubar_bg = Color::RGB(40, 46, 52);    // #2d353b
    colors.menubar_fg = Color::RGB(220, 215, 195); // #d3c6aa

    colors.helpbar_bg = Color::RGB(40, 46, 52);     // #2d353b
    colors.helpbar_fg = Color::RGB(124, 136, 148);  // #7c8898
    colors.helpbar_key = Color::RGB(163, 190, 140); // #a3be8c

    colors.keyword = Color::RGB(143, 188, 187);        // #8fbcbb
    colors.string = Color::RGB(163, 190, 140);         // #a3be8c
    colors.comment = Color::RGB(124, 136, 148);        // #7c8898
    colors.number = Color::RGB(233, 179, 101);         // #e9b365
    colors.function = Color::RGB(163, 190, 140);       // #a3be8c
    colors.type = Color::RGB(143, 188, 187);           // #8fbcbb
    colors.operator_color = Color::RGB(143, 188, 187); // #8fbcbb

    colors.error = Color::RGB(231, 130, 132);   // #e78284
    colors.warning = Color::RGB(233, 179, 101); // #e9b365
    colors.info = Color::RGB(143, 188, 187);    // #8fbcbb
    colors.success = Color::RGB(163, 190, 140); // #a3be8c

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(52, 59, 66);          // #343f46
    colors.dialog_fg = Color::RGB(220, 215, 195);       // #d3c6aa
    colors.dialog_title_bg = Color::RGB(40, 46, 52);    // #2d353b
    colors.dialog_title_fg = Color::RGB(220, 215, 195); // #d3c6aa
    colors.dialog_border = Color::RGB(124, 136, 148);   // #7c8898

    return colors;
}

ThemeColors Theme::Jellybeans() {
    ThemeColors colors;
    colors.background = Color::RGB(21, 21, 21);             // #151515
    colors.foreground = Color::RGB(224, 224, 224);          // #e0e0e0
    colors.current_line = Color::RGB(40, 40, 40);           // #282828
    colors.selection = Color::RGB(68, 68, 68);              // #444444
    colors.line_number = Color::RGB(112, 112, 112);         // #707070
    colors.line_number_current = Color::RGB(224, 224, 224); // #e0e0e0

    colors.statusbar_bg = Color::RGB(21, 21, 21);    // #151515
    colors.statusbar_fg = Color::RGB(224, 224, 224); // #e0e0e0

    colors.menubar_bg = Color::RGB(21, 21, 21);    // #151515
    colors.menubar_fg = Color::RGB(224, 224, 224); // #e0e0e0

    colors.helpbar_bg = Color::RGB(21, 21, 21);     // #151515
    colors.helpbar_fg = Color::RGB(112, 112, 112);  // #707070
    colors.helpbar_key = Color::RGB(152, 195, 121); // #98c379

    colors.keyword = Color::RGB(204, 102, 102);        // #cc6666
    colors.string = Color::RGB(152, 195, 121);         // #98c379
    colors.comment = Color::RGB(112, 112, 112);        // #707070
    colors.number = Color::RGB(250, 189, 47);          // #fabd2f
    colors.function = Color::RGB(250, 189, 47);        // #fabd2f
    colors.type = Color::RGB(97, 175, 239);            // #61afef
    colors.operator_color = Color::RGB(204, 102, 102); // #cc6666

    colors.error = Color::RGB(224, 108, 117);   // #e06c75
    colors.warning = Color::RGB(250, 189, 47);  // #fabd2f
    colors.info = Color::RGB(97, 175, 239);     // #61afef
    colors.success = Color::RGB(152, 195, 121); // #98c379

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(40, 40, 40);          // #282828
    colors.dialog_fg = Color::RGB(224, 224, 224);       // #e0e0e0
    colors.dialog_title_bg = Color::RGB(21, 21, 21);    // #151515
    colors.dialog_title_fg = Color::RGB(224, 224, 224); // #e0e0e0
    colors.dialog_border = Color::RGB(112, 112, 112);   // #707070

    return colors;
}

ThemeColors Theme::Desert() {
    ThemeColors colors;
    colors.background = Color::RGB(51, 51, 51);             // #333333
    colors.foreground = Color::RGB(255, 255, 255);          // #ffffff
    colors.current_line = Color::RGB(68, 68, 68);           // #444444
    colors.selection = Color::RGB(68, 68, 68);              // #444444
    colors.line_number = Color::RGB(136, 136, 136);         // #888888
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff

    colors.statusbar_bg = Color::RGB(51, 51, 51);    // #333333
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(51, 51, 51);    // #333333
    colors.menubar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.helpbar_bg = Color::RGB(51, 51, 51);     // #333333
    colors.helpbar_fg = Color::RGB(136, 136, 136);  // #888888
    colors.helpbar_key = Color::RGB(255, 220, 177); // #ffdcb1

    colors.keyword = Color::RGB(86, 180, 233);        // #56b4e9
    colors.string = Color::RGB(230, 219, 116);        // #e6db74
    colors.comment = Color::RGB(117, 113, 94);        // #75715e
    colors.number = Color::RGB(174, 129, 255);        // #ae81ff
    colors.function = Color::RGB(166, 226, 46);       // #a6e22e
    colors.type = Color::RGB(102, 217, 239);          // #66d9ef
    colors.operator_color = Color::RGB(249, 38, 114); // #f92672

    colors.error = Color::RGB(249, 38, 114);   // #f92672
    colors.warning = Color::RGB(253, 151, 31); // #fd971f
    colors.info = Color::RGB(102, 217, 239);   // #66d9ef
    colors.success = Color::RGB(166, 226, 46); // #a6e22e

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(68, 68, 68);          // #444444
    colors.dialog_fg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_title_bg = Color::RGB(51, 51, 51);    // #333333
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(136, 136, 136);   // #888888

    return colors;
}

ThemeColors Theme::Slate() {
    ThemeColors colors;
    colors.background = Color::RGB(34, 39, 46);             // #22272e
    colors.foreground = Color::RGB(205, 217, 229);          // #cdd9e5
    colors.current_line = Color::RGB(45, 51, 59);           // #2d333b
    colors.selection = Color::RGB(55, 62, 72);              // #373e47
    colors.line_number = Color::RGB(108, 117, 125);         // #6c757d
    colors.line_number_current = Color::RGB(205, 217, 229); // #cdd9e5

    colors.statusbar_bg = Color::RGB(34, 39, 46);    // #22272e
    colors.statusbar_fg = Color::RGB(205, 217, 229); // #cdd9e5

    colors.menubar_bg = Color::RGB(34, 39, 46);    // #22272e
    colors.menubar_fg = Color::RGB(205, 217, 229); // #cdd9e5

    colors.helpbar_bg = Color::RGB(34, 39, 46);     // #22272e
    colors.helpbar_fg = Color::RGB(108, 117, 125);  // #6c757d
    colors.helpbar_key = Color::RGB(121, 192, 255); // #79c0ff

    colors.keyword = Color::RGB(255, 123, 114);        // #ff7b72
    colors.string = Color::RGB(121, 192, 255);         // #79c0ff
    colors.comment = Color::RGB(108, 117, 125);        // #6c757d
    colors.number = Color::RGB(121, 192, 255);         // #79c0ff
    colors.function = Color::RGB(210, 168, 255);       // #d2a8ff
    colors.type = Color::RGB(121, 192, 255);           // #79c0ff
    colors.operator_color = Color::RGB(255, 123, 114); // #ff7b72

    colors.error = Color::RGB(248, 81, 73);    // #f85149
    colors.warning = Color::RGB(251, 188, 5);  // #fbbc05
    colors.info = Color::RGB(121, 192, 255);   // #79c0ff
    colors.success = Color::RGB(56, 211, 159); // #38d39f

    // 弹窗颜色 - 使用稍微不同的背景色来突出弹窗
    colors.dialog_bg = Color::RGB(45, 51, 59);          // #2d333b
    colors.dialog_fg = Color::RGB(205, 217, 229);       // #cdd9e5
    colors.dialog_title_bg = Color::RGB(34, 39, 46);    // #22272e
    colors.dialog_title_fg = Color::RGB(205, 217, 229); // #cdd9e5
    colors.dialog_border = Color::RGB(108, 117, 125);   // #6c757d

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
    return {"monokai",      "dracula",     "solarized-dark", "solarized-light",     "onedark",
            "nord",         "gruvbox",     "tokyo-night",    "catppuccin",          "material",
            "ayu",          "github",      "vscode-dark",    "night-owl",           "palenight",
            "oceanic-next", "kanagawa",    "tomorrow-night", "tomorrow-night-blue", "cobalt",
            "zenburn",      "base16-dark", "papercolor",     "rose-pine",           "everforest",
            "jellybeans",   "desert",      "slate"};
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
