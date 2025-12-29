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
    colors.background = Color::RGB(39, 40, 34);           // #272822
    colors.foreground = Color::RGB(248, 248, 242);        // #f8f8f2
    colors.current_line = Color::RGB(73, 72, 62);         // #49483e
    colors.selection = Color::RGB(73, 72, 62);            // #49483e
    colors.line_number = Color::RGB(144, 144, 138);       // #90908a
    colors.line_number_current = Color::RGB(248, 248, 242); // #f8f8f2
    
    colors.statusbar_bg = Color::RGB(45, 45, 45);         // #2d2d2d
    colors.statusbar_fg = Color::RGB(248, 248, 242);      // #f8f8f2
    
    colors.menubar_bg = Color::RGB(30, 31, 27);           // #1e1f1b
    colors.menubar_fg = Color::RGB(248, 248, 242);        // #f8f8f2
    
    colors.helpbar_bg = Color::RGB(45, 45, 45);           // #2d2d2d
    colors.helpbar_fg = Color::RGB(117, 113, 94);         // #75715e
    colors.helpbar_key = Color::RGB(166, 226, 46);        // #a6e22e
    
    colors.keyword = Color::RGB(249, 38, 114);            // #f92672
    colors.string = Color::RGB(230, 219, 116);            // #e6db74
    colors.comment = Color::RGB(117, 113, 94);            // #75715e
    colors.number = Color::RGB(174, 129, 255);            // #ae81ff
    colors.function = Color::RGB(166, 226, 46);           // #a6e22e
    colors.type = Color::RGB(102, 217, 239);              // #66d9ef
    colors.operator_color = Color::RGB(249, 38, 114);     // #f92672
    
    colors.error = Color::RGB(249, 38, 114);              // #f92672
    colors.warning = Color::RGB(253, 151, 31);            // #fd971f
    colors.info = Color::RGB(102, 217, 239);              // #66d9ef
    colors.success = Color::RGB(166, 226, 46);            // #a6e22e
    
    return colors;
}

ThemeColors Theme::Dracula() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 42, 54);           // #282a36
    colors.foreground = Color::RGB(248, 248, 242);        // #f8f8f2
    colors.current_line = Color::RGB(68, 71, 90);         // #44475a
    colors.selection = Color::RGB(68, 71, 90);            // #44475a
    colors.line_number = Color::RGB(98, 114, 164);        // #6272a4
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff
    
    colors.statusbar_bg = Color::RGB(68, 71, 90);         // #44475a
    colors.statusbar_fg = Color::RGB(248, 248, 242);      // #f8f8f2
    
    colors.menubar_bg = Color::RGB(30, 31, 40);           // #1e1f28
    colors.menubar_fg = Color::RGB(248, 248, 242);        // #f8f8f2
    
    colors.helpbar_bg = Color::RGB(68, 71, 90);           // #44475a
    colors.helpbar_fg = Color::RGB(189, 147, 249);        // #bd93f9
    colors.helpbar_key = Color::RGB(80, 250, 123);        // #50fa7b
    
    colors.keyword = Color::RGB(255, 121, 198);           // #ff79c6
    colors.string = Color::RGB(241, 250, 140);            // #f1fa8c
    colors.comment = Color::RGB(98, 114, 164);            // #6272a4
    colors.number = Color::RGB(189, 147, 249);            // #bd93f9
    colors.function = Color::RGB(80, 250, 123);           // #50fa7b
    colors.type = Color::RGB(139, 233, 253);              // #8be9fd
    colors.operator_color = Color::RGB(255, 121, 198);    // #ff79c6
    
    colors.error = Color::RGB(255, 85, 85);               // #ff5555
    colors.warning = Color::RGB(255, 184, 108);           // #ffb86c
    colors.info = Color::RGB(139, 233, 253);              // #8be9fd
    colors.success = Color::RGB(80, 250, 123);            // #50fa7b
    
    return colors;
}

ThemeColors Theme::SolarizedDark() {
    ThemeColors colors;
    colors.background = Color::RGB(0, 43, 54);            // #002b36
    colors.foreground = Color::RGB(131, 148, 150);        // #839496
    colors.current_line = Color::RGB(7, 54, 66);          // #073642
    colors.selection = Color::RGB(7, 54, 66);             // #073642
    colors.line_number = Color::RGB(88, 110, 117);        // #586e75
    colors.line_number_current = Color::RGB(147, 161, 161); // #93a1a1
    
    colors.statusbar_bg = Color::RGB(7, 54, 66);          // #073642
    colors.statusbar_fg = Color::RGB(131, 148, 150);      // #839496
    
    colors.menubar_bg = Color::RGB(0, 43, 54);            // #002b36
    colors.menubar_fg = Color::RGB(131, 148, 150);        // #839496
    
    colors.helpbar_bg = Color::RGB(7, 54, 66);            // #073642
    colors.helpbar_fg = Color::RGB(88, 110, 117);         // #586e75
    colors.helpbar_key = Color::RGB(133, 153, 0);         // #859900
    
    colors.keyword = Color::RGB(203, 75, 22);             // #cb4b16
    colors.string = Color::RGB(42, 161, 152);             // #2aa198
    colors.comment = Color::RGB(88, 110, 117);            // #586e75
    colors.number = Color::RGB(108, 113, 196);            // #6c71c4
    colors.function = Color::RGB(133, 153, 0);            // #859900
    colors.type = Color::RGB(181, 137, 0);                // #b58900
    colors.operator_color = Color::RGB(38, 139, 210);     // #268bd2
    
    colors.error = Color::RGB(220, 50, 47);               // #dc322f
    colors.warning = Color::RGB(203, 75, 22);             // #cb4b16
    colors.info = Color::RGB(38, 139, 210);               // #268bd2
    colors.success = Color::RGB(133, 153, 0);             // #859900
    
    return colors;
}

ThemeColors Theme::SolarizedLight() {
    ThemeColors colors;
    colors.background = Color::RGB(253, 246, 227);        // #fdf6e3
    colors.foreground = Color::RGB(101, 123, 131);        // #657b83
    colors.current_line = Color::RGB(238, 232, 213);      // #eee8d5
    colors.selection = Color::RGB(238, 232, 213);         // #eee8d5
    colors.line_number = Color::RGB(147, 161, 161);       // #93a1a1
    colors.line_number_current = Color::RGB(88, 110, 117); // #586e75
    
    colors.statusbar_bg = Color::RGB(238, 232, 213);      // #eee8d5
    colors.statusbar_fg = Color::RGB(101, 123, 131);      // #657b83
    
    colors.menubar_bg = Color::RGB(253, 246, 227);        // #fdf6e3
    colors.menubar_fg = Color::RGB(101, 123, 131);        // #657b83
    
    colors.helpbar_bg = Color::RGB(238, 232, 213);        // #eee8d5
    colors.helpbar_fg = Color::RGB(147, 161, 161);        // #93a1a1
    colors.helpbar_key = Color::RGB(133, 153, 0);         // #859900
    
    colors.keyword = Color::RGB(203, 75, 22);             // #cb4b16
    colors.string = Color::RGB(42, 161, 152);             // #2aa198
    colors.comment = Color::RGB(147, 161, 161);           // #93a1a1
    colors.number = Color::RGB(108, 113, 196);            // #6c71c4
    colors.function = Color::RGB(133, 153, 0);            // #859900
    colors.type = Color::RGB(181, 137, 0);                // #b58900
    colors.operator_color = Color::RGB(38, 139, 210);     // #268bd2
    
    colors.error = Color::RGB(220, 50, 47);               // #dc322f
    colors.warning = Color::RGB(203, 75, 22);             // #cb4b16
    colors.info = Color::RGB(38, 139, 210);               // #268bd2
    colors.success = Color::RGB(133, 153, 0);             // #859900
    
    return colors;
}

ThemeColors Theme::OneDark() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 44, 52);           // #282c34
    colors.foreground = Color::RGB(171, 178, 191);        // #abb2bf
    colors.current_line = Color::RGB(44, 48, 57);         // #2c3039
    colors.selection = Color::RGB(56, 61, 72);            // #383d48
    colors.line_number = Color::RGB(92, 99, 112);         // #5c6370
    colors.line_number_current = Color::RGB(171, 178, 191); // #abb2bf
    
    colors.statusbar_bg = Color::RGB(33, 37, 43);         // #21252b
    colors.statusbar_fg = Color::RGB(171, 178, 191);      // #abb2bf
    
    colors.menubar_bg = Color::RGB(33, 37, 43);           // #21252b
    colors.menubar_fg = Color::RGB(171, 178, 191);        // #abb2bf
    
    colors.helpbar_bg = Color::RGB(33, 37, 43);           // #21252b
    colors.helpbar_fg = Color::RGB(92, 99, 112);          // #5c6370
    colors.helpbar_key = Color::RGB(152, 195, 121);       // #98c379
    
    colors.keyword = Color::RGB(198, 120, 221);           // #c678dd
    colors.string = Color::RGB(152, 195, 121);            // #98c379
    colors.comment = Color::RGB(92, 99, 112);             // #5c6370
    colors.number = Color::RGB(209, 154, 102);            // #d19a66
    colors.function = Color::RGB(97, 175, 239);           // #61afef
    colors.type = Color::RGB(229, 192, 123);              // #e5c07b
    colors.operator_color = Color::RGB(86, 182, 194);     // #56b6c2
    
    colors.error = Color::RGB(224, 108, 117);             // #e06c75
    colors.warning = Color::RGB(229, 192, 123);           // #e5c07b
    colors.info = Color::RGB(97, 175, 239);               // #61afef
    colors.success = Color::RGB(152, 195, 121);           // #98c379
    
    return colors;
}

ThemeColors Theme::Nord() {
    ThemeColors colors;
    colors.background = Color::RGB(46, 52, 64);           // #2e3440
    colors.foreground = Color::RGB(216, 222, 233);        // #d8dee9
    colors.current_line = Color::RGB(59, 66, 82);         // #3b4252
    colors.selection = Color::RGB(59, 66, 82);            // #3b4252
    colors.line_number = Color::RGB(76, 86, 106);         // #4c566a
    colors.line_number_current = Color::RGB(216, 222, 233); // #d8dee9
    
    colors.statusbar_bg = Color::RGB(59, 66, 82);         // #3b4252
    colors.statusbar_fg = Color::RGB(216, 222, 233);      // #d8dee9
    
    colors.menubar_bg = Color::RGB(46, 52, 64);           // #2e3440
    colors.menubar_fg = Color::RGB(216, 222, 233);        // #d8dee9
    
    colors.helpbar_bg = Color::RGB(59, 66, 82);           // #3b4252
    colors.helpbar_fg = Color::RGB(76, 86, 106);          // #4c566a
    colors.helpbar_key = Color::RGB(163, 190, 140);       // #a3be8c
    
    colors.keyword = Color::RGB(129, 161, 193);           // #81a1c1
    colors.string = Color::RGB(163, 190, 140);            // #a3be8c
    colors.comment = Color::RGB(76, 86, 106);             // #4c566a
    colors.number = Color::RGB(180, 142, 173);            // #b48ead
    colors.function = Color::RGB(136, 192, 208);          // #88c0d0
    colors.type = Color::RGB(235, 203, 139);              // #ebcb8b
    colors.operator_color = Color::RGB(129, 161, 193);    // #81a1c1
    
    colors.error = Color::RGB(191, 97, 106);              // #bf616a
    colors.warning = Color::RGB(235, 203, 139);           // #ebcb8b
    colors.info = Color::RGB(136, 192, 208);              // #88c0d0
    colors.success = Color::RGB(163, 190, 140);           // #a3be8c
    
    return colors;
}

ThemeColors Theme::Gruvbox() {
    ThemeColors colors;
    colors.background = Color::RGB(40, 40, 40);           // #282828
    colors.foreground = Color::RGB(235, 219, 178);       // #ebdbb2
    colors.current_line = Color::RGB(60, 56, 54);         // #3c3836
    colors.selection = Color::RGB(60, 56, 54);             // #3c3836
    colors.line_number = Color::RGB(124, 111, 100);       // #7c6f64
    colors.line_number_current = Color::RGB(235, 219, 178); // #ebdbb2
    
    colors.statusbar_bg = Color::RGB(50, 48, 47);         // #32302f
    colors.statusbar_fg = Color::RGB(235, 219, 178);      // #ebdbb2
    
    colors.menubar_bg = Color::RGB(40, 40, 40);           // #282828
    colors.menubar_fg = Color::RGB(235, 219, 178);        // #ebdbb2
    
    colors.helpbar_bg = Color::RGB(50, 48, 47);           // #32302f
    colors.helpbar_fg = Color::RGB(124, 111, 100);        // #7c6f64
    colors.helpbar_key = Color::RGB(152, 151, 26);        // #98971a
    
    colors.keyword = Color::RGB(251, 73, 52);             // #fb4934
    colors.string = Color::RGB(184, 187, 38);             // #b8bb26
    colors.comment = Color::RGB(146, 131, 116);           // #928374
    colors.number = Color::RGB(211, 134, 155);            // #d3869b
    colors.function = Color::RGB(250, 189, 47);           // #fabd2f
    colors.type = Color::RGB(131, 165, 152);              // #83a598
    colors.operator_color = Color::RGB(251, 73, 52);      // #fb4934
    
    colors.error = Color::RGB(251, 73, 52);               // #fb4934
    colors.warning = Color::RGB(250, 189, 47);            // #fabd2f
    colors.info = Color::RGB(131, 165, 152);             // #83a598
    colors.success = Color::RGB(184, 187, 38);           // #b8bb26
    
    return colors;
}

ThemeColors Theme::TokyoNight() {
    ThemeColors colors;
    colors.background = Color::RGB(26, 27, 38);           // #1a1b26
    colors.foreground = Color::RGB(192, 202, 245);        // #c0caf5
    colors.current_line = Color::RGB(36, 40, 59);         // #24283b
    colors.selection = Color::RGB(36, 40, 59);            // #24283b
    colors.line_number = Color::RGB(99, 104, 135);       // #63688f
    colors.line_number_current = Color::RGB(192, 202, 245); // #c0caf5
    
    colors.statusbar_bg = Color::RGB(36, 40, 59);         // #24283b
    colors.statusbar_fg = Color::RGB(192, 202, 245);       // #c0caf5
    
    colors.menubar_bg = Color::RGB(26, 27, 38);           // #1a1b26
    colors.menubar_fg = Color::RGB(192, 202, 245);         // #c0caf5
    
    colors.helpbar_bg = Color::RGB(36, 40, 59);           // #24283b
    colors.helpbar_fg = Color::RGB(99, 104, 135);         // #63688f
    colors.helpbar_key = Color::RGB(125, 207, 255);       // #7dcfff
    
    colors.keyword = Color::RGB(187, 154, 247);           // #bb9af7
    colors.string = Color::RGB(158, 206, 106);            // #9ece6a
    colors.comment = Color::RGB(99, 104, 135);            // #63688f
    colors.number = Color::RGB(255, 184, 108);           // #ffb86c
    colors.function = Color::RGB(125, 207, 255);          // #7dcfff
    colors.type = Color::RGB(122, 162, 247);              // #7aa2f7
    colors.operator_color = Color::RGB(187, 154, 247);     // #bb9af7
    
    colors.error = Color::RGB(247, 118, 142);             // #f7768e
    colors.warning = Color::RGB(255, 184, 108);           // #ffb86c
    colors.info = Color::RGB(125, 207, 255);              // #7dcfff
    colors.success = Color::RGB(158, 206, 106);           // #9ece6a
    
    return colors;
}

ThemeColors Theme::Catppuccin() {
    ThemeColors colors;
    colors.background = Color::RGB(30, 30, 46);           // #1e1e2e
    colors.foreground = Color::RGB(205, 214, 244);        // #cdd6f4
    colors.current_line = Color::RGB(49, 50, 68);         // #313244
    colors.selection = Color::RGB(49, 50, 68);            // #313244
    colors.line_number = Color::RGB(108, 112, 134);      // #6c7086
    colors.line_number_current = Color::RGB(205, 214, 244); // #cdd6f4
    
    colors.statusbar_bg = Color::RGB(24, 24, 37);        // #181825
    colors.statusbar_fg = Color::RGB(205, 214, 244);       // #cdd6f4
    
    colors.menubar_bg = Color::RGB(30, 30, 46);           // #1e1e2e
    colors.menubar_fg = Color::RGB(205, 214, 244);        // #cdd6f4
    
    colors.helpbar_bg = Color::RGB(24, 24, 37);           // #181825
    colors.helpbar_fg = Color::RGB(108, 112, 134);        // #6c7086
    colors.helpbar_key = Color::RGB(166, 227, 161);       // #a6e3a1
    
    colors.keyword = Color::RGB(203, 166, 247);           // #cba6f7
    colors.string = Color::RGB(166, 227, 161);             // #a6e3a1
    colors.comment = Color::RGB(108, 112, 134);           // #6c7086
    colors.number = Color::RGB(250, 179, 135);            // #fab387
    colors.function = Color::RGB(137, 220, 235);          // #89dceb
    colors.type = Color::RGB(148, 226, 213);              // #94e2d5
    colors.operator_color = Color::RGB(203, 166, 247);    // #cba6f7
    
    colors.error = Color::RGB(243, 139, 168);             // #f38ba8
    colors.warning = Color::RGB(250, 179, 135);           // #fab387
    colors.info = Color::RGB(137, 220, 235);              // #89dceb
    colors.success = Color::RGB(166, 227, 161);           // #a6e3a1
    
    return colors;
}

ThemeColors Theme::Material() {
    ThemeColors colors;
    colors.background = Color::RGB(38, 50, 56);           // #263238
    colors.foreground = Color::RGB(238, 255, 255);         // #eeffff
    colors.current_line = Color::RGB(38, 50, 56);          // #263238
    colors.selection = Color::RGB(55, 71, 79);            // #37474f
    colors.line_number = Color::RGB(84, 110, 122);        // #546e7a
    colors.line_number_current = Color::RGB(238, 255, 255); // #eeffff
    
    colors.statusbar_bg = Color::RGB(38, 50, 56);         // #263238
    colors.statusbar_fg = Color::RGB(238, 255, 255);       // #eeffff
    
    colors.menubar_bg = Color::RGB(38, 50, 56);           // #263238
    colors.menubar_fg = Color::RGB(238, 255, 255);        // #eeffff
    
    colors.helpbar_bg = Color::RGB(38, 50, 56);           // #263238
    colors.helpbar_fg = Color::RGB(84, 110, 122);        // #546e7a
    colors.helpbar_key = Color::RGB(195, 232, 141);        // #c3e88d
    
    colors.keyword = Color::RGB(199, 146, 234);           // #c792ea
    colors.string = Color::RGB(195, 232, 141);            // #c3e88d
    colors.comment = Color::RGB(84, 110, 122);            // #546e7a
    colors.number = Color::RGB(247, 140, 108);            // #f78c6c
    colors.function = Color::RGB(130, 170, 255);          // #82aaff
    colors.type = Color::RGB(255, 203, 107);              // #ffcb6b
    colors.operator_color = Color::RGB(199, 146, 234);    // #c792ea
    
    colors.error = Color::RGB(255, 83, 112);             // #ff5370
    colors.warning = Color::RGB(255, 203, 107);           // #ffcb6b
    colors.info = Color::RGB(130, 170, 255);              // #82aaff
    colors.success = Color::RGB(195, 232, 141);           // #c3e88d
    
    return colors;
}

ThemeColors Theme::Ayu() {
    ThemeColors colors;
    colors.background = Color::RGB(15, 23, 42);           // #0f172a
    colors.foreground = Color::RGB(203, 213, 225);        // #cbd5e1
    colors.current_line = Color::RGB(30, 41, 59);         // #1e293b
    colors.selection = Color::RGB(30, 41, 59);             // #1e293b
    colors.line_number = Color::RGB(100, 116, 139);       // #64748b
    colors.line_number_current = Color::RGB(203, 213, 225); // #cbd5e1
    
    colors.statusbar_bg = Color::RGB(15, 23, 42);         // #0f172a
    colors.statusbar_fg = Color::RGB(203, 213, 225);       // #cbd5e1
    
    colors.menubar_bg = Color::RGB(15, 23, 42);           // #0f172a
    colors.menubar_fg = Color::RGB(203, 213, 225);        // #cbd5e1
    
    colors.helpbar_bg = Color::RGB(15, 23, 42);          // #0f172a
    colors.helpbar_fg = Color::RGB(100, 116, 139);        // #64748b
    colors.helpbar_key = Color::RGB(34, 197, 94);         // #22c55e
    
    colors.keyword = Color::RGB(139, 92, 246);            // #8b5cf6
    colors.string = Color::RGB(34, 197, 94);               // #22c55e
    colors.comment = Color::RGB(100, 116, 139);            // #64748b
    colors.number = Color::RGB(251, 146, 60);             // #fb923c
    colors.function = Color::RGB(59, 130, 246);           // #3b82f6
    colors.type = Color::RGB(251, 191, 36);               // #fbbf24
    colors.operator_color = Color::RGB(139, 92, 246);     // #8b5cf6
    
    colors.error = Color::RGB(239, 68, 68);               // #ef4444
    colors.warning = Color::RGB(251, 146, 60);           // #fb923c
    colors.info = Color::RGB(59, 130, 246);               // #3b82f6
    colors.success = Color::RGB(34, 197, 94);             // #22c55e
    
    return colors;
}

ThemeColors Theme::GitHub() {
    ThemeColors colors;
    colors.background = Color::RGB(255, 255, 255);        // #ffffff
    colors.foreground = Color::RGB(36, 41, 46);            // #24292e
    colors.current_line = Color::RGB(246, 248, 250);      // #f6f8fa
    colors.selection = Color::RGB(200, 225, 255);         // #c8e1ff
    colors.line_number = Color::RGB(106, 115, 125);       // #6a737d
    colors.line_number_current = Color::RGB(36, 41, 46);  // #24292e
    
    colors.statusbar_bg = Color::RGB(246, 248, 250);     // #f6f8fa
    colors.statusbar_fg = Color::RGB(36, 41, 46);          // #24292e
    
    colors.menubar_bg = Color::RGB(255, 255, 255);        // #ffffff
    colors.menubar_fg = Color::RGB(36, 41, 46);           // #24292e
    
    colors.helpbar_bg = Color::RGB(246, 248, 250);        // #f6f8fa
    colors.helpbar_fg = Color::RGB(106, 115, 125);        // #6a737d
    colors.helpbar_key = Color::RGB(34, 134, 58);          // #22863a
    
    colors.keyword = Color::RGB(111, 66, 193);            // #6f42c1
    colors.string = Color::RGB(0, 92, 197);                 // #005cc5
    colors.comment = Color::RGB(106, 115, 125);             // #6a737d
    colors.number = Color::RGB(0, 92, 197);                // #005cc5
    colors.function = Color::RGB(111, 66, 193);             // #6f42c1
    colors.type = Color::RGB(34, 134, 58);                 // #22863a
    colors.operator_color = Color::RGB(36, 41, 46);        // #24292e
    
    colors.error = Color::RGB(203, 36, 49);                 // #cb2431
    colors.warning = Color::RGB(219, 171, 9);              // #dbab09
    colors.info = Color::RGB(0, 92, 197);                 // #005cc5
    colors.success = Color::RGB(34, 134, 58);               // #22863a
    
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
    } else {
        colors_ = Monokai(); // 默认主题
    }
}

bool Theme::loadCustomTheme(const std::string& name, const ThemeColors& colors) {
    custom_themes_[name] = colors;
    return true;
}

bool Theme::loadThemeFromConfig(const std::vector<int>& background,
                                const std::vector<int>& foreground,
                                const std::vector<int>& current_line,
                                const std::vector<int>& selection,
                                const std::vector<int>& line_number,
                                const std::vector<int>& line_number_current,
                                const std::vector<int>& statusbar_bg,
                                const std::vector<int>& statusbar_fg,
                                const std::vector<int>& menubar_bg,
                                const std::vector<int>& menubar_fg,
                                const std::vector<int>& helpbar_bg,
                                const std::vector<int>& helpbar_fg,
                                const std::vector<int>& helpbar_key,
                                const std::vector<int>& keyword,
                                const std::vector<int>& string,
                                const std::vector<int>& comment,
                                const std::vector<int>& number,
                                const std::vector<int>& function,
                                const std::vector<int>& type,
                                const std::vector<int>& operator_color,
                                const std::vector<int>& error,
                                const std::vector<int>& warning,
                                const std::vector<int>& info,
                                const std::vector<int>& success) {
    ThemeColors colors;
    
    if (background.size() >= 3) colors.background = rgbToColor(background);
    if (foreground.size() >= 3) colors.foreground = rgbToColor(foreground);
    if (current_line.size() >= 3) colors.current_line = rgbToColor(current_line);
    if (selection.size() >= 3) colors.selection = rgbToColor(selection);
    if (line_number.size() >= 3) colors.line_number = rgbToColor(line_number);
    if (line_number_current.size() >= 3) colors.line_number_current = rgbToColor(line_number_current);
    if (statusbar_bg.size() >= 3) colors.statusbar_bg = rgbToColor(statusbar_bg);
    if (statusbar_fg.size() >= 3) colors.statusbar_fg = rgbToColor(statusbar_fg);
    if (menubar_bg.size() >= 3) colors.menubar_bg = rgbToColor(menubar_bg);
    if (menubar_fg.size() >= 3) colors.menubar_fg = rgbToColor(menubar_fg);
    if (helpbar_bg.size() >= 3) colors.helpbar_bg = rgbToColor(helpbar_bg);
    if (helpbar_fg.size() >= 3) colors.helpbar_fg = rgbToColor(helpbar_fg);
    if (helpbar_key.size() >= 3) colors.helpbar_key = rgbToColor(helpbar_key);
    if (keyword.size() >= 3) colors.keyword = rgbToColor(keyword);
    if (string.size() >= 3) colors.string = rgbToColor(string);
    if (comment.size() >= 3) colors.comment = rgbToColor(comment);
    if (number.size() >= 3) colors.number = rgbToColor(number);
    if (function.size() >= 3) colors.function = rgbToColor(function);
    if (type.size() >= 3) colors.type = rgbToColor(type);
    if (operator_color.size() >= 3) colors.operator_color = rgbToColor(operator_color);
    if (error.size() >= 3) colors.error = rgbToColor(error);
    if (warning.size() >= 3) colors.warning = rgbToColor(warning);
    if (info.size() >= 3) colors.info = rgbToColor(info);
    if (success.size() >= 3) colors.success = rgbToColor(success);
    
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
    return {
        "monokai", "dracula", "solarized-dark", "solarized-light",
        "onedark", "nord", "gruvbox", "tokyo-night",
        "catppuccin", "material", "ayu", "github"
    };
}

} // namespace ui
} // namespace pnana

