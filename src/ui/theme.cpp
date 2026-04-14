#include "ui/theme.h"
#include <array>
#include <vector>

using namespace ftxui;

namespace pnana {
namespace ui {

Theme::Theme() : current_theme_("monokai") {
    colors_ = Monokai();
}

ThemeColors Theme::Monokai() {
    ThemeColors colors;
    // Monokai Pro: 经典 Monokai，深灰绿基底，粉/绿/青/橙标志色
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

ThemeColors Theme::Orange() {
    ThemeColors colors;
    // Orange: 橙黄色系渐变，所有语法高亮色集中在橙 - 黄窄色域
    // 灵感来源：黄昏天空、琥珀色渐变、金色暖阳、蜂蜜流动
    // 设计说明：基础 UI 保持 Monokai 经典色，语法高亮色在橙黄区间细腻过渡
    colors.background = Color::RGB(39, 40, 34);    // #272822 经典 Monokai 背景
    colors.foreground = Color::RGB(248, 248, 242); // #f8f8f2 经典前景色
    colors.current_line = Color::RGB(55, 56, 48);  // #373831 当前行背景
    colors.selection = Color::RGB(73, 72, 62);     // #49483e 选择区
    colors.line_number = Color::RGB(117, 113, 94); // #75715e 行号
    colors.line_number_current = Color::RGB(248, 248, 242);

    colors.statusbar_bg = Color::RGB(36, 37, 31);
    colors.statusbar_fg = Color::RGB(248, 248, 242);

    colors.menubar_bg = Color::RGB(30, 31, 28);
    colors.menubar_fg = Color::RGB(248, 248, 242);

    colors.helpbar_bg = Color::RGB(36, 37, 31);
    colors.helpbar_fg = Color::RGB(117, 113, 94);
    colors.helpbar_key = Color::RGB(166, 226, 46);

    // 语法高亮色块渐变带：深橙→橙→橙黄→黄→浅黄（RGB 细腻过渡，色域集中在 20-60 区间）
    // 色块 1：深橙红（关键词）- 渐变起点，暖色调
    colors.keyword = Color::RGB(255, 135, 45); // #ff872d 深橙红
    // 色块 2：橙色（操作符）- 向标准橙过渡
    colors.operator_color = Color::RGB(255, 145, 50); // #ff9132 标准橙
    // 色块 3：橙红（错误）- 强调色
    colors.error = Color::RGB(255, 155, 55); // #ff9b37 亮橙红
    // 色块 4：橙黄（警告）- 橙向黄过渡
    colors.warning = Color::RGB(255, 165, 60); // #ffa53c 橙黄
    // 色块 5：金橙（数字）- 接近中点
    colors.number = Color::RGB(255, 175, 65); // #ffaf41 金橙色
    // 色块 6：金橙黄（成功）- 继续向黄过渡
    colors.success = Color::RGB(245, 185, 70); // #f5b946 金橙黄
    // 色块 7：橙黄（函数）- 渐变中点
    colors.function = Color::RGB(240, 195, 80); // #f0c350 橙黄
    // 色块 8：黄橙（信息）- 黄色调增强
    colors.info = Color::RGB(235, 205, 90); // #ebcd5a 黄橙色
    // 色块 9：金黄色（类型）- 接近纯黄
    colors.type = Color::RGB(230, 215, 100); // #e6d764 金黄色
    // 色块 10：暖黄（字符串）- 渐变终点，柔和黄
    colors.string = Color::RGB(225, 220, 110); // #e1dc6e 暖黄色
    // 注释保持独立灰色，不参与渐变
    colors.comment = Color::RGB(117, 113, 94); // #75715e 经典灰

    colors.dialog_bg = Color::RGB(49, 50, 44);
    colors.dialog_fg = Color::RGB(248, 248, 242);
    colors.dialog_title_bg = Color::RGB(39, 40, 34);
    colors.dialog_title_fg = Color::RGB(248, 248, 242);
    colors.dialog_border = Color::RGB(117, 113, 94);

    return colors;
}

ThemeColors Theme::MonokaiDark() {
    ThemeColors colors;
    // Monokai Dark: 更深的暗色版本，增强对比度
    colors.background = Color::RGB(30, 31, 26);    // #1e1f1a
    colors.foreground = Color::RGB(248, 248, 242); // #f8f8f2
    colors.current_line = Color::RGB(45, 46, 40);  // #2d2e28
    colors.selection = Color::RGB(60, 61, 54);     // #3c3d36
    colors.line_number = Color::RGB(100, 97, 80);  // #646150
    colors.line_number_current = Color::RGB(248, 248, 242);

    colors.statusbar_bg = Color::RGB(25, 26, 22);
    colors.statusbar_fg = Color::RGB(248, 248, 242);

    colors.menubar_bg = Color::RGB(20, 21, 18);
    colors.menubar_fg = Color::RGB(248, 248, 242);

    colors.helpbar_bg = Color::RGB(25, 26, 22);
    colors.helpbar_fg = Color::RGB(100, 97, 80);
    colors.helpbar_key = Color::RGB(166, 226, 46);

    colors.keyword = Color::RGB(255, 50, 120);  // 更亮的粉红
    colors.string = Color::RGB(240, 210, 90);   // 更亮的黄
    colors.comment = Color::RGB(90, 87, 72);    // 更深的灰
    colors.number = Color::RGB(255, 140, 40);   // 更亮的橙
    colors.function = Color::RGB(140, 230, 50); // 更亮的绿
    colors.type = Color::RGB(90, 220, 245);     // 更亮的青
    colors.operator_color = Color::RGB(255, 50, 120);

    colors.error = Color::RGB(255, 60, 100);
    colors.warning = Color::RGB(255, 160, 50);
    colors.info = Color::RGB(90, 220, 245);
    colors.success = Color::RGB(140, 230, 50);

    colors.dialog_bg = Color::RGB(40, 41, 36);
    colors.dialog_fg = Color::RGB(248, 248, 242);
    colors.dialog_title_bg = Color::RGB(30, 31, 26);
    colors.dialog_title_fg = Color::RGB(248, 248, 242);
    colors.dialog_border = Color::RGB(100, 97, 80);

    return colors;
}

ThemeColors Theme::MonokaiLight() {
    ThemeColors colors;
    // Monokai Light: 浅色版本，适合明亮环境
    colors.background = Color::RGB(250, 250, 248);   // #fafaf8
    colors.foreground = Color::RGB(50, 50, 45);      // #32322d
    colors.current_line = Color::RGB(240, 240, 235); // #f0f0eb
    colors.selection = Color::RGB(220, 220, 210);    // #dcdcd2
    colors.line_number = Color::RGB(150, 145, 130);  // #968d82
    colors.line_number_current = Color::RGB(50, 50, 45);

    colors.statusbar_bg = Color::RGB(235, 235, 230);
    colors.statusbar_fg = Color::RGB(50, 50, 45);

    colors.menubar_bg = Color::RGB(245, 245, 240);
    colors.menubar_fg = Color::RGB(50, 50, 45);

    colors.helpbar_bg = Color::RGB(235, 235, 230);
    colors.helpbar_fg = Color::RGB(120, 115, 100);
    colors.helpbar_key = Color::RGB(100, 160, 30);

    colors.keyword = Color::RGB(200, 30, 90);   // 深粉红
    colors.string = Color::RGB(180, 140, 40);   // 深黄
    colors.comment = Color::RGB(150, 145, 130); // 灰
    colors.number = Color::RGB(210, 110, 20);   // 深橙
    colors.function = Color::RGB(100, 160, 30); // 深绿
    colors.type = Color::RGB(50, 150, 180);     // 深青
    colors.operator_color = Color::RGB(200, 30, 90);

    colors.error = Color::RGB(220, 50, 80);
    colors.warning = Color::RGB(230, 130, 30);
    colors.info = Color::RGB(50, 150, 180);
    colors.success = Color::RGB(100, 160, 30);

    colors.dialog_bg = Color::RGB(255, 255, 253);
    colors.dialog_fg = Color::RGB(50, 50, 45);
    colors.dialog_title_bg = Color::RGB(240, 240, 235);
    colors.dialog_title_fg = Color::RGB(50, 50, 45);
    colors.dialog_border = Color::RGB(200, 195, 180);

    return colors;
}

ThemeColors Theme::MonokaiNeon() {
    ThemeColors colors;
    // Monokai Neon: 霓虹版本，增强发光效果
    colors.background = Color::RGB(20, 21, 18);    // #151512
    colors.foreground = Color::RGB(255, 255, 250); // #fffffa
    colors.current_line = Color::RGB(35, 36, 30);  // #23241e
    colors.selection = Color::RGB(50, 51, 44);     // #32332c
    colors.line_number = Color::RGB(80, 77, 64);   // #504d40
    colors.line_number_current = Color::RGB(255, 255, 250);

    colors.statusbar_bg = Color::RGB(15, 16, 13);
    colors.statusbar_fg = Color::RGB(255, 255, 250);

    colors.menubar_bg = Color::RGB(10, 11, 9);
    colors.menubar_fg = Color::RGB(255, 255, 250);

    colors.helpbar_bg = Color::RGB(15, 16, 13);
    colors.helpbar_fg = Color::RGB(120, 117, 100);
    colors.helpbar_key = Color::RGB(180, 255, 60); // 霓虹绿

    // 霓虹色
    colors.keyword = Color::RGB(255, 30, 150); // 霓虹粉红
    colors.string = Color::RGB(255, 240, 80);  // 霓虹黄
    colors.comment = Color::RGB(80, 77, 64);
    colors.number = Color::RGB(255, 130, 20);   // 霓虹橙
    colors.function = Color::RGB(150, 255, 50); // 霓虹绿
    colors.type = Color::RGB(60, 230, 255);     // 霓虹青
    colors.operator_color = Color::RGB(255, 30, 150);

    colors.error = Color::RGB(255, 50, 100);
    colors.warning = Color::RGB(255, 180, 40);
    colors.info = Color::RGB(60, 230, 255);
    colors.success = Color::RGB(150, 255, 50);

    colors.dialog_bg = Color::RGB(30, 31, 26);
    colors.dialog_fg = Color::RGB(255, 255, 250);
    colors.dialog_title_bg = Color::RGB(20, 21, 18);
    colors.dialog_title_fg = Color::RGB(255, 255, 250);
    colors.dialog_border = Color::RGB(100, 97, 84);

    return colors;
}

ThemeColors Theme::MonokaiPastel() {
    ThemeColors colors;
    // Monokai Pastel: 粉彩版本，柔和的马卡龙色系
    colors.background = Color::RGB(45, 46, 42);     // #2d2e2a
    colors.foreground = Color::RGB(245, 245, 240);  // #f5f5f0
    colors.current_line = Color::RGB(58, 59, 54);   // #3a3b36
    colors.selection = Color::RGB(72, 73, 67);      // #484943
    colors.line_number = Color::RGB(130, 127, 112); // #827f70
    colors.line_number_current = Color::RGB(245, 245, 240);

    colors.statusbar_bg = Color::RGB(40, 41, 37);
    colors.statusbar_fg = Color::RGB(245, 245, 240);

    colors.menubar_bg = Color::RGB(35, 36, 32);
    colors.menubar_fg = Color::RGB(245, 245, 240);

    colors.helpbar_bg = Color::RGB(40, 41, 37);
    colors.helpbar_fg = Color::RGB(130, 127, 112);
    colors.helpbar_key = Color::RGB(180, 220, 120); // 柔和绿

    // 粉彩色
    colors.keyword = Color::RGB(255, 140, 180); // 柔和粉红
    colors.string = Color::RGB(240, 220, 140);  // 柔和黄
    colors.comment = Color::RGB(130, 127, 112);
    colors.number = Color::RGB(255, 180, 120);   // 柔和橙
    colors.function = Color::RGB(160, 220, 130); // 柔和绿
    colors.type = Color::RGB(140, 210, 230);     // 柔和青
    colors.operator_color = Color::RGB(255, 140, 180);

    colors.error = Color::RGB(255, 150, 160);
    colors.warning = Color::RGB(255, 200, 130);
    colors.info = Color::RGB(140, 210, 230);
    colors.success = Color::RGB(160, 220, 130);

    colors.dialog_bg = Color::RGB(52, 53, 48);
    colors.dialog_fg = Color::RGB(245, 245, 240);
    colors.dialog_title_bg = Color::RGB(45, 46, 42);
    colors.dialog_title_fg = Color::RGB(245, 245, 240);
    colors.dialog_border = Color::RGB(120, 117, 102);

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
    // Nord Arctic: 经典 Nord，冷冽冰蓝北极风格
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

    colors.dialog_bg = Color::RGB(66, 74, 90);
    colors.dialog_fg = Color::RGB(220, 225, 230);
    colors.dialog_title_bg = Color::RGB(40, 46, 58);
    colors.dialog_title_fg = Color::RGB(220, 225, 230);
    colors.dialog_border = Color::RGB(100, 114, 132);

    return colors;
}

ThemeColors Theme::NordFrost() {
    ThemeColors colors;
    // Nord Frost: 霜冻版本，更浅的冰蓝色调
    colors.background = Color::RGB(46, 52, 64);
    colors.foreground = Color::RGB(236, 239, 244);
    colors.current_line = Color::RGB(62, 70, 82);
    colors.selection = Color::RGB(76, 86, 106);
    colors.line_number = Color::RGB(120, 135, 155);
    colors.line_number_current = Color::RGB(236, 239, 244);

    colors.statusbar_bg = Color::RGB(62, 70, 82);
    colors.statusbar_fg = Color::RGB(236, 239, 244);

    colors.menubar_bg = Color::RGB(46, 52, 64);
    colors.menubar_fg = Color::RGB(236, 239, 244);

    colors.helpbar_bg = Color::RGB(62, 70, 82);
    colors.helpbar_fg = Color::RGB(120, 135, 155);
    colors.helpbar_key = Color::RGB(143, 188, 187);

    colors.keyword = Color::RGB(129, 161, 193);
    colors.string = Color::RGB(163, 190, 140);
    colors.comment = Color::RGB(120, 135, 155);
    colors.number = Color::RGB(180, 142, 173);
    colors.function = Color::RGB(136, 192, 208);
    colors.type = Color::RGB(235, 203, 139);
    colors.operator_color = Color::RGB(129, 161, 193);

    colors.error = Color::RGB(191, 97, 106);
    colors.warning = Color::RGB(235, 203, 139);
    colors.info = Color::RGB(136, 192, 208);
    colors.success = Color::RGB(163, 190, 140);

    colors.dialog_bg = Color::RGB(76, 86, 106);
    colors.dialog_fg = Color::RGB(236, 239, 244);
    colors.dialog_title_bg = Color::RGB(46, 52, 64);
    colors.dialog_title_fg = Color::RGB(236, 239, 244);
    colors.dialog_border = Color::RGB(120, 135, 155);

    return colors;
}

ThemeColors Theme::NordAurora() {
    ThemeColors colors;
    // Nord Aurora: 极光版本，强调极光色彩（粉紫绿）
    colors.background = Color::RGB(36, 41, 51);
    colors.foreground = Color::RGB(229, 233, 240);
    colors.current_line = Color::RGB(48, 55, 68);
    colors.selection = Color::RGB(58, 67, 82);
    colors.line_number = Color::RGB(95, 110, 130);
    colors.line_number_current = Color::RGB(229, 233, 240);

    colors.statusbar_bg = Color::RGB(48, 55, 68);
    colors.statusbar_fg = Color::RGB(229, 233, 240);

    colors.menubar_bg = Color::RGB(36, 41, 51);
    colors.menubar_fg = Color::RGB(229, 233, 240);

    colors.helpbar_bg = Color::RGB(48, 55, 68);
    colors.helpbar_fg = Color::RGB(95, 110, 130);
    colors.helpbar_key = Color::RGB(163, 190, 140);

    // 极光色彩：粉紫绿
    colors.keyword = Color::RGB(191, 97, 106); // 极光红
    colors.string = Color::RGB(163, 190, 140); // 极光绿
    colors.comment = Color::RGB(95, 110, 130);
    colors.number = Color::RGB(180, 142, 173);   // 极光紫
    colors.function = Color::RGB(136, 192, 208); // 冰蓝
    colors.type = Color::RGB(235, 203, 139);     // 极光黄
    colors.operator_color = Color::RGB(191, 97, 106);

    colors.error = Color::RGB(191, 97, 106);
    colors.warning = Color::RGB(235, 203, 139);
    colors.info = Color::RGB(136, 192, 208);
    colors.success = Color::RGB(163, 190, 140);

    colors.dialog_bg = Color::RGB(58, 67, 82);
    colors.dialog_fg = Color::RGB(229, 233, 240);
    colors.dialog_title_bg = Color::RGB(36, 41, 51);
    colors.dialog_title_fg = Color::RGB(229, 233, 240);
    colors.dialog_border = Color::RGB(95, 110, 130);

    return colors;
}

ThemeColors Theme::NordDeep() {
    ThemeColors colors;
    // Nord Deep: 深海版本，更深的蓝色调
    colors.background = Color::RGB(30, 35, 45);
    colors.foreground = Color::RGB(215, 220, 228);
    colors.current_line = Color::RGB(42, 48, 60);
    colors.selection = Color::RGB(52, 60, 75);
    colors.line_number = Color::RGB(85, 100, 120);
    colors.line_number_current = Color::RGB(215, 220, 228);

    colors.statusbar_bg = Color::RGB(42, 48, 60);
    colors.statusbar_fg = Color::RGB(215, 220, 228);

    colors.menubar_bg = Color::RGB(30, 35, 45);
    colors.menubar_fg = Color::RGB(215, 220, 228);

    colors.helpbar_bg = Color::RGB(42, 48, 60);
    colors.helpbar_fg = Color::RGB(85, 100, 120);
    colors.helpbar_key = Color::RGB(150, 185, 130);

    colors.keyword = Color::RGB(120, 170, 210);
    colors.string = Color::RGB(150, 180, 130);
    colors.comment = Color::RGB(85, 100, 120);
    colors.number = Color::RGB(170, 135, 165);
    colors.function = Color::RGB(125, 185, 200);
    colors.type = Color::RGB(225, 195, 130);
    colors.operator_color = Color::RGB(120, 170, 210);

    colors.error = Color::RGB(180, 90, 100);
    colors.warning = Color::RGB(225, 195, 130);
    colors.info = Color::RGB(125, 185, 200);
    colors.success = Color::RGB(150, 180, 130);

    colors.dialog_bg = Color::RGB(52, 60, 75);
    colors.dialog_fg = Color::RGB(215, 220, 228);
    colors.dialog_title_bg = Color::RGB(30, 35, 45);
    colors.dialog_title_fg = Color::RGB(215, 220, 228);
    colors.dialog_border = Color::RGB(85, 100, 120);

    return colors;
}

ThemeColors Theme::NordLight() {
    ThemeColors colors;
    // Nord Light: 浅色版本，雪白色调
    colors.background = Color::RGB(236, 239, 244);
    colors.foreground = Color::RGB(46, 52, 64);
    colors.current_line = Color::RGB(220, 225, 232);
    colors.selection = Color::RGB(208, 215, 225);
    colors.line_number = Color::RGB(150, 160, 175);
    colors.line_number_current = Color::RGB(46, 52, 64);

    colors.statusbar_bg = Color::RGB(220, 225, 232);
    colors.statusbar_fg = Color::RGB(46, 52, 64);

    colors.menubar_bg = Color::RGB(230, 234, 240);
    colors.menubar_fg = Color::RGB(46, 52, 64);

    colors.helpbar_bg = Color::RGB(220, 225, 232);
    colors.helpbar_fg = Color::RGB(120, 135, 155);
    colors.helpbar_key = Color::RGB(80, 130, 100);

    colors.keyword = Color::RGB(80, 120, 160);
    colors.string = Color::RGB(100, 140, 90);
    colors.comment = Color::RGB(150, 160, 175);
    colors.number = Color::RGB(140, 100, 130);
    colors.function = Color::RGB(80, 150, 170);
    colors.type = Color::RGB(180, 150, 80);
    colors.operator_color = Color::RGB(80, 120, 160);

    colors.error = Color::RGB(160, 70, 80);
    colors.warning = Color::RGB(180, 150, 80);
    colors.info = Color::RGB(80, 150, 170);
    colors.success = Color::RGB(100, 140, 90);

    colors.dialog_bg = Color::RGB(246, 248, 252);
    colors.dialog_fg = Color::RGB(46, 52, 64);
    colors.dialog_title_bg = Color::RGB(220, 225, 232);
    colors.dialog_title_fg = Color::RGB(46, 52, 64);
    colors.dialog_border = Color::RGB(180, 190, 205);

    return colors;
}

ThemeColors Theme::NordStorm() {
    ThemeColors colors;
    // Nord Storm: 风暴版本，更强烈的对比度
    colors.background = Color::RGB(34, 40, 50);
    colors.foreground = Color::RGB(240, 243, 248);
    colors.current_line = Color::RGB(46, 54, 66);
    colors.selection = Color::RGB(58, 68, 84);
    colors.line_number = Color::RGB(100, 115, 135);
    colors.line_number_current = Color::RGB(240, 243, 248);

    colors.statusbar_bg = Color::RGB(46, 54, 66);
    colors.statusbar_fg = Color::RGB(240, 243, 248);

    colors.menubar_bg = Color::RGB(34, 40, 50);
    colors.menubar_fg = Color::RGB(240, 243, 248);

    colors.helpbar_bg = Color::RGB(46, 54, 66);
    colors.helpbar_fg = Color::RGB(100, 115, 135);
    colors.helpbar_key = Color::RGB(180, 215, 145);

    colors.keyword = Color::RGB(140, 185, 220);
    colors.string = Color::RGB(175, 210, 150);
    colors.comment = Color::RGB(100, 115, 135);
    colors.number = Color::RGB(200, 165, 195);
    colors.function = Color::RGB(145, 205, 220);
    colors.type = Color::RGB(245, 215, 145);
    colors.operator_color = Color::RGB(140, 185, 220);

    colors.error = Color::RGB(215, 100, 115);
    colors.warning = Color::RGB(245, 215, 145);
    colors.info = Color::RGB(145, 205, 220);
    colors.success = Color::RGB(175, 210, 150);

    colors.dialog_bg = Color::RGB(58, 68, 84);
    colors.dialog_fg = Color::RGB(240, 243, 248);
    colors.dialog_title_bg = Color::RGB(34, 40, 50);
    colors.dialog_title_fg = Color::RGB(240, 243, 248);
    colors.dialog_border = Color::RGB(100, 115, 135);

    return colors;
}

ThemeColors Theme::NordPolar() {
    ThemeColors colors;
    // Nord Polar: 极地版本，更冷的色调
    colors.background = Color::RGB(28, 33, 42);
    colors.foreground = Color::RGB(216, 222, 228);
    colors.current_line = Color::RGB(38, 45, 56);
    colors.selection = Color::RGB(48, 56, 70);
    colors.line_number = Color::RGB(90, 105, 125);
    colors.line_number_current = Color::RGB(216, 222, 228);

    colors.statusbar_bg = Color::RGB(38, 45, 56);
    colors.statusbar_fg = Color::RGB(216, 222, 228);

    colors.menubar_bg = Color::RGB(28, 33, 42);
    colors.menubar_fg = Color::RGB(216, 222, 228);

    colors.helpbar_bg = Color::RGB(38, 45, 56);
    colors.helpbar_fg = Color::RGB(90, 105, 125);
    colors.helpbar_key = Color::RGB(155, 195, 160);

    colors.keyword = Color::RGB(125, 175, 210);
    colors.string = Color::RGB(155, 190, 145);
    colors.comment = Color::RGB(90, 105, 125);
    colors.number = Color::RGB(175, 140, 175);
    colors.function = Color::RGB(130, 190, 205);
    colors.type = Color::RGB(230, 200, 140);
    colors.operator_color = Color::RGB(125, 175, 210);

    colors.error = Color::RGB(195, 95, 110);
    colors.warning = Color::RGB(230, 200, 140);
    colors.info = Color::RGB(130, 190, 205);
    colors.success = Color::RGB(155, 190, 145);

    colors.dialog_bg = Color::RGB(48, 56, 70);
    colors.dialog_fg = Color::RGB(216, 222, 228);
    colors.dialog_title_bg = Color::RGB(28, 33, 42);
    colors.dialog_title_fg = Color::RGB(216, 222, 228);
    colors.dialog_border = Color::RGB(90, 105, 125);

    return colors;
}

ThemeColors Theme::NordMidnight() {
    ThemeColors colors;
    // Nord Midnight: 午夜版本，最深的蓝色调
    colors.background = Color::RGB(22, 26, 34);
    colors.foreground = Color::RGB(210, 218, 226);
    colors.current_line = Color::RGB(32, 38, 48);
    colors.selection = Color::RGB(42, 50, 62);
    colors.line_number = Color::RGB(80, 95, 115);
    colors.line_number_current = Color::RGB(210, 218, 226);

    colors.statusbar_bg = Color::RGB(32, 38, 48);
    colors.statusbar_fg = Color::RGB(210, 218, 226);

    colors.menubar_bg = Color::RGB(22, 26, 34);
    colors.menubar_fg = Color::RGB(210, 218, 226);

    colors.helpbar_bg = Color::RGB(32, 38, 48);
    colors.helpbar_fg = Color::RGB(80, 95, 115);
    colors.helpbar_key = Color::RGB(145, 185, 150);

    colors.keyword = Color::RGB(115, 165, 205);
    colors.string = Color::RGB(145, 180, 140);
    colors.comment = Color::RGB(80, 95, 115);
    colors.number = Color::RGB(165, 130, 165);
    colors.function = Color::RGB(120, 180, 195);
    colors.type = Color::RGB(220, 190, 130);
    colors.operator_color = Color::RGB(115, 165, 205);

    colors.error = Color::RGB(185, 85, 100);
    colors.warning = Color::RGB(220, 190, 130);
    colors.info = Color::RGB(120, 180, 195);
    colors.success = Color::RGB(145, 180, 140);

    colors.dialog_bg = Color::RGB(42, 50, 62);
    colors.dialog_fg = Color::RGB(210, 218, 226);
    colors.dialog_title_bg = Color::RGB(22, 26, 34);
    colors.dialog_title_fg = Color::RGB(210, 218, 226);
    colors.dialog_border = Color::RGB(80, 95, 115);

    return colors;
}

ThemeColors Theme::NordGlacier() {
    ThemeColors colors;
    // Nord Glacier: 冰川版本，更亮的冰蓝色调
    colors.background = Color::RGB(52, 58, 72);
    colors.foreground = Color::RGB(232, 235, 240);
    colors.current_line = Color::RGB(66, 74, 90);
    colors.selection = Color::RGB(80, 90, 110);
    colors.line_number = Color::RGB(130, 145, 165);
    colors.line_number_current = Color::RGB(232, 235, 240);

    colors.statusbar_bg = Color::RGB(66, 74, 90);
    colors.statusbar_fg = Color::RGB(232, 235, 240);

    colors.menubar_bg = Color::RGB(52, 58, 72);
    colors.menubar_fg = Color::RGB(232, 235, 240);

    colors.helpbar_bg = Color::RGB(66, 74, 90);
    colors.helpbar_fg = Color::RGB(130, 145, 165);
    colors.helpbar_key = Color::RGB(175, 215, 155);

    colors.keyword = Color::RGB(145, 190, 225);
    colors.string = Color::RGB(180, 210, 160);
    colors.comment = Color::RGB(130, 145, 165);
    colors.number = Color::RGB(195, 160, 195);
    colors.function = Color::RGB(155, 210, 225);
    colors.type = Color::RGB(240, 210, 155);
    colors.operator_color = Color::RGB(145, 190, 225);

    colors.error = Color::RGB(205, 105, 120);
    colors.warning = Color::RGB(240, 210, 155);
    colors.info = Color::RGB(155, 210, 225);
    colors.success = Color::RGB(180, 210, 160);

    colors.dialog_bg = Color::RGB(80, 90, 110);
    colors.dialog_fg = Color::RGB(232, 235, 240);
    colors.dialog_title_bg = Color::RGB(52, 58, 72);
    colors.dialog_title_fg = Color::RGB(232, 235, 240);
    colors.dialog_border = Color::RGB(130, 145, 165);

    return colors;
}

ThemeColors Theme::PurpleDark() {
    ThemeColors colors;
    // Purple Dark: 紫黑主题，深紫底色 + 亮紫/粉紫点缀
    colors.background = Color::RGB(25, 20, 35);    // #191423 深紫黑
    colors.foreground = Color::RGB(230, 225, 240); // #e6e1f0 淡紫白
    colors.current_line = Color::RGB(40, 32, 52);  // #282034 深紫
    colors.selection = Color::RGB(55, 45, 72);     // #372d48 紫灰
    colors.line_number = Color::RGB(100, 90, 115); // #645a73 灰紫
    colors.line_number_current = Color::RGB(230, 225, 240);

    colors.statusbar_bg = Color::RGB(35, 28, 48); // #231c30 深紫
    colors.statusbar_fg = Color::RGB(230, 225, 240);

    colors.menubar_bg = Color::RGB(30, 24, 42); // #1e182a 更深紫
    colors.menubar_fg = Color::RGB(230, 225, 240);

    colors.helpbar_bg = Color::RGB(35, 28, 48);
    colors.helpbar_fg = Color::RGB(100, 90, 115);
    colors.helpbar_key = Color::RGB(180, 140, 220); // 亮紫

    // 紫色调语法高亮
    colors.keyword = Color::RGB(200, 130, 240);  // 亮紫
    colors.string = Color::RGB(240, 180, 200);   // 粉紫
    colors.comment = Color::RGB(100, 90, 115);   // 灰紫
    colors.number = Color::RGB(255, 160, 180);   // 亮粉
    colors.function = Color::RGB(160, 200, 255); // 淡蓝紫
    colors.type = Color::RGB(220, 180, 255);     // 浅紫
    colors.operator_color = Color::RGB(200, 130, 240);

    colors.error = Color::RGB(255, 120, 140);   // 亮粉红
    colors.warning = Color::RGB(255, 200, 120); // 橙黄
    colors.info = Color::RGB(160, 200, 255);    // 淡蓝
    colors.success = Color::RGB(160, 240, 180); // 淡绿

    colors.dialog_bg = Color::RGB(45, 38, 58); // 深紫灰
    colors.dialog_fg = Color::RGB(230, 225, 240);
    colors.dialog_title_bg = Color::RGB(55, 45, 75); // 紫标题
    colors.dialog_title_fg = Color::RGB(230, 225, 240);
    colors.dialog_border = Color::RGB(120, 100, 150); // 紫边框

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

ThemeColors Theme::CatppuccinLatte() {
    ThemeColors colors;
    // Catppuccin Latte: 浅色版本，暖白基底 + 柔和粉彩点缀
    colors.background = Color::RGB(239, 241, 245);        // #eff1f5 Base
    colors.foreground = Color::RGB(76, 79, 105);          // #4c4f69 Text
    colors.current_line = Color::RGB(220, 223, 231);      // #dce0e7 Surface0
    colors.selection = Color::RGB(204, 208, 218);         // #ccd0da Surface1
    colors.line_number = Color::RGB(156, 160, 176);       // #9ca0b0 Overlay0
    colors.line_number_current = Color::RGB(76, 79, 105); // #4c4f69

    colors.statusbar_bg = Color::RGB(230, 233, 242); // #e6e9f2 Mantle
    colors.statusbar_fg = Color::RGB(76, 79, 105);

    colors.menubar_bg = Color::RGB(220, 223, 231); // #dce0e7 Crust
    colors.menubar_fg = Color::RGB(76, 79, 105);

    colors.helpbar_bg = Color::RGB(230, 233, 242);
    colors.helpbar_fg = Color::RGB(131, 137, 160); // #8389a0 Overlay
    colors.helpbar_key = Color::RGB(23, 146, 153); // #179299 Teal

    // Catppuccin Latte 标志色：Mauve/Peach/Green/Teal/Sky
    colors.keyword = Color::RGB(142, 76, 177);        // #8e44b1 Mauve
    colors.string = Color::RGB(64, 160, 43);          // #40a02b Green
    colors.comment = Color::RGB(156, 160, 176);       // #9ca0b0 Overlay0
    colors.number = Color::RGB(254, 100, 11);         // #fe640b Peach
    colors.function = Color::RGB(37, 159, 154);       // #259f9a Sky
    colors.type = Color::RGB(32, 159, 181);           // #209fb5 Sapphire
    colors.operator_color = Color::RGB(142, 76, 177); // Mauve

    colors.error = Color::RGB(210, 15, 57);    // #d20f39 Red
    colors.warning = Color::RGB(254, 100, 11); // #fe640b Peach
    colors.info = Color::RGB(37, 159, 154);    // #259f9a Sky
    colors.success = Color::RGB(64, 160, 43);  // #40a02b Green

    colors.dialog_bg = Color::RGB(220, 223, 231); // #dce0e7 Surface0
    colors.dialog_fg = Color::RGB(76, 79, 105);
    colors.dialog_title_bg = Color::RGB(211, 215, 226); // #d3d7e2 Surface2
    colors.dialog_title_fg = Color::RGB(76, 79, 105);
    colors.dialog_border = Color::RGB(186, 187, 203); // #babbc0 Surface2

    return colors;
}

ThemeColors Theme::CatppuccinFrappe() {
    ThemeColors colors;
    // Catppuccin Frappé: 中等深色，冷紫灰基底 + 清爽粉彩点缀
    colors.background = Color::RGB(48, 52, 70);             // #303446 Base
    colors.foreground = Color::RGB(198, 208, 245);          // #c6d0f5 Text
    colors.current_line = Color::RGB(65, 69, 89);           // #414559 Surface0
    colors.selection = Color::RGB(81, 87, 109);             // #51576d Surface1
    colors.line_number = Color::RGB(121, 128, 151);         // #798097 Overlay0
    colors.line_number_current = Color::RGB(198, 208, 245); // #c6d0f5

    colors.statusbar_bg = Color::RGB(41, 44, 60); // #292c3c Mantle
    colors.statusbar_fg = Color::RGB(198, 208, 245);

    colors.menubar_bg = Color::RGB(35, 38, 52); // #232634 Crust
    colors.menubar_fg = Color::RGB(198, 208, 245);

    colors.helpbar_bg = Color::RGB(41, 44, 60);
    colors.helpbar_fg = Color::RGB(165, 173, 206);  // #a5adce Overlay
    colors.helpbar_key = Color::RGB(125, 207, 255); // #7dcfff Teal

    // Catppuccin Frappé 标志色：Mauve/Peach/Green/Teal/Sky
    colors.keyword = Color::RGB(202, 158, 230);        // #ca9ee6 Mauve
    colors.string = Color::RGB(166, 209, 137);         // #a6d189 Green
    colors.comment = Color::RGB(121, 128, 151);        // #798097 Overlay0
    colors.number = Color::RGB(239, 159, 118);         // #ef9f76 Peach
    colors.function = Color::RGB(153, 209, 219);       // #99d1db Sky
    colors.type = Color::RGB(128, 205, 232);           // #80cdd4 Sapphire
    colors.operator_color = Color::RGB(202, 158, 230); // Mauve

    colors.error = Color::RGB(227, 135, 145);   // #e38c8f Red
    colors.warning = Color::RGB(239, 159, 118); // #ef9f76 Peach
    colors.info = Color::RGB(153, 209, 219);    // #99d1db Sky
    colors.success = Color::RGB(166, 209, 137); // #a6d189 Green

    colors.dialog_bg = Color::RGB(65, 69, 89); // #414559
    colors.dialog_fg = Color::RGB(198, 208, 245);
    colors.dialog_title_bg = Color::RGB(75, 79, 101); // #4b4f65 Surface2
    colors.dialog_title_fg = Color::RGB(198, 208, 245);
    colors.dialog_border = Color::RGB(107, 112, 129); // #6b7081 Surface2

    return colors;
}

ThemeColors Theme::CatppuccinMacchiato() {
    ThemeColors colors;
    // Catppuccin Macchiato: 较深版本，紫灰基底 + 鲜艳粉彩点缀
    colors.background = Color::RGB(36, 39, 57);             // #24273a Base
    colors.foreground = Color::RGB(202, 211, 245);          // #cad3f5 Text
    colors.current_line = Color::RGB(54, 58, 79);           // #363a4f Surface0
    colors.selection = Color::RGB(69, 73, 96);              // #454960 Surface1
    colors.line_number = Color::RGB(114, 121, 149);         // #727995 Overlay0
    colors.line_number_current = Color::RGB(202, 211, 245); // #cad3f5

    colors.statusbar_bg = Color::RGB(30, 32, 48); // #1e2030 Mantle
    colors.statusbar_fg = Color::RGB(202, 211, 245);

    colors.menubar_bg = Color::RGB(24, 26, 40); // #181a28 Crust
    colors.menubar_fg = Color::RGB(202, 211, 245);

    colors.helpbar_bg = Color::RGB(30, 32, 48);
    colors.helpbar_fg = Color::RGB(162, 171, 210);  // #a2a9d2 Overlay
    colors.helpbar_key = Color::RGB(139, 213, 202); // #8bd5ca Teal

    // Catppuccin Macchiato 标志色：Mauve/Peach/Green/Teal/Sky
    colors.keyword = Color::RGB(198, 160, 246);        // #c6a0f6 Mauve
    colors.string = Color::RGB(166, 227, 161);         // #a6e3a1 Green
    colors.comment = Color::RGB(114, 121, 149);        // #727995 Overlay0
    colors.number = Color::RGB(245, 169, 127);         // #f5a97f Peach
    colors.function = Color::RGB(145, 211, 235);       // #91d3eb Sky
    colors.type = Color::RGB(125, 207, 255);           // #7dcfff Sapphire
    colors.operator_color = Color::RGB(198, 160, 246); // Mauve

    colors.error = Color::RGB(237, 135, 150);   // #ed8796 Red
    colors.warning = Color::RGB(245, 169, 127); // #f5a97f Peach
    colors.info = Color::RGB(145, 211, 235);    // #91d3eb Sky
    colors.success = Color::RGB(166, 227, 161); // #a6e3a1 Green

    colors.dialog_bg = Color::RGB(54, 58, 79); // #363a4f
    colors.dialog_fg = Color::RGB(202, 211, 245);
    colors.dialog_title_bg = Color::RGB(64, 68, 91); // #40445b Surface2
    colors.dialog_title_fg = Color::RGB(202, 211, 245);
    colors.dialog_border = Color::RGB(94, 101, 124); // #5e657c Surface2

    return colors;
}

ThemeColors Theme::Doraemon() {
    ThemeColors colors;
    // 叮当猫/哆啦 A 梦主题：经典蓝白红黄配色，明亮活泼
    // 灵感来自哆啦 A 梦的蓝色身体、白色肚皮、红色鼻子和黄色铃铛
    colors.background = Color::RGB(25, 25, 112);    // #191970 Midnight Blue - 哆啦 A 梦蓝
    colors.foreground = Color::RGB(255, 255, 255);  // #ffffff 白色肚皮
    colors.current_line = Color::RGB(40, 40, 140);  // #28288c 稍亮的蓝色
    colors.selection = Color::RGB(60, 60, 160);     // #3c3ca0 选中背景
    colors.line_number = Color::RGB(135, 206, 235); // #87ceeb Sky Blue - 柔和蓝色
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff 当前行号

    colors.statusbar_bg = Color::RGB(20, 20, 100); // #141464 深蓝
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(15, 15, 90); // #0f0f5a 更深蓝
    colors.menubar_fg = Color::RGB(255, 255, 255);

    colors.helpbar_bg = Color::RGB(20, 20, 100);
    colors.helpbar_fg = Color::RGB(200, 200, 255); // 浅蓝白
    colors.helpbar_key = Color::RGB(255, 215, 0);  // #ffd700 金色铃铛

    // 叮当猫主题语法色：红鼻子/金铃铛/白肚皮/蓝身体
    colors.keyword = Color::RGB(220, 20, 60);        // #dc143c Crimson - 红色鼻子
    colors.string = Color::RGB(255, 255, 255);       // #ffffff 白色肚皮
    colors.comment = Color::RGB(135, 206, 235);      // #87ceeb Sky Blue
    colors.number = Color::RGB(255, 215, 0);         // #ffd700 金色铃铛
    colors.function = Color::RGB(100, 149, 237);     // #6495ed Cornflower Blue
    colors.type = Color::RGB(176, 196, 222);         // #b0c4de Light Steel Blue
    colors.operator_color = Color::RGB(220, 20, 60); // Crimson - 红色

    colors.error = Color::RGB(255, 69, 0);      // #ff4500 Orange Red
    colors.warning = Color::RGB(255, 215, 0);   // #ffd700 金色
    colors.info = Color::RGB(135, 206, 235);    // #87ceeb Sky Blue
    colors.success = Color::RGB(255, 255, 255); // #ffffff 白色

    colors.dialog_bg = Color::RGB(40, 40, 140); // #28288c
    colors.dialog_fg = Color::RGB(255, 255, 255);
    colors.dialog_title_bg = Color::RGB(220, 20, 60); // #dc143c 红色鼻子
    colors.dialog_title_fg = Color::RGB(255, 255, 255);
    colors.dialog_border = Color::RGB(255, 215, 0); // #ffd700 金色铃铛

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

ThemeColors Theme::GitHubDarkDimmed() {
    ThemeColors colors;
    // GitHub Dark Dimmed: GitHub 官方柔和暗色主题，对比度更低，适合长时间阅读
    colors.background = Color::RGB(34, 39, 46);             // #22272e - Dimmed background
    colors.foreground = Color::RGB(201, 209, 217);          // #c9d1d9 - Dimmed foreground
    colors.current_line = Color::RGB(42, 48, 56);           // #2a3038 - Current line
    colors.selection = Color::RGB(54, 61, 71);              // #363d47 - Selection
    colors.line_number = Color::RGB(110, 118, 129);         // #6e7681 - Line numbers
    colors.line_number_current = Color::RGB(201, 209, 217); // #c9d1d9

    colors.statusbar_bg = Color::RGB(42, 48, 56);    // #2a3038
    colors.statusbar_fg = Color::RGB(201, 209, 217); // #c9d1d9

    colors.menubar_bg = Color::RGB(34, 39, 46);    // #22272e
    colors.menubar_fg = Color::RGB(201, 209, 217); // #c9d1d9

    colors.helpbar_bg = Color::RGB(42, 48, 56);
    colors.helpbar_fg = Color::RGB(110, 118, 129); // #6e7681
    colors.helpbar_key = Color::RGB(88, 166, 255); // #58a6ff GitHub 蓝

    // GitHub Dark Dimmed 语法高亮：粉关键词/蓝字符串/紫函数/橙数字/青类型
    colors.keyword = Color::RGB(255, 123, 172);        // #ff7bac 粉
    colors.string = Color::RGB(163, 186, 202);         // #a3bac8 浅蓝
    colors.comment = Color::RGB(110, 118, 129);        // #6e7681 灰
    colors.number = Color::RGB(121, 192, 255);         // #79c0ff 蓝
    colors.function = Color::RGB(210, 168, 255);       // #d2a8ff 紫
    colors.type = Color::RGB(56, 178, 172);            // #38b2ac 青
    colors.operator_color = Color::RGB(255, 123, 172); // #ff7bac 粉

    colors.error = Color::RGB(255, 123, 172);  // #ff7bac 粉
    colors.warning = Color::RGB(210, 153, 34); // #d29922 黄
    colors.info = Color::RGB(88, 166, 255);    // #58a6ff 蓝
    colors.success = Color::RGB(63, 185, 80);  // #3fb950 绿

    colors.dialog_bg = Color::RGB(42, 48, 56);          // #2a3038
    colors.dialog_fg = Color::RGB(201, 209, 217);       // #c9d1d9
    colors.dialog_title_bg = Color::RGB(34, 39, 46);    // #22272e
    colors.dialog_title_fg = Color::RGB(201, 209, 217); // #c9d1d9
    colors.dialog_border = Color::RGB(110, 118, 129);   // #6e7681

    return colors;
}

ThemeColors Theme::GitHubDarkHighContrast() {
    ThemeColors colors;
    // GitHub Dark High Contrast: GitHub 官方高对比度暗色主题，边界清晰，适合视力需求
    colors.background = Color::RGB(13, 17, 23);             // #0d1117 - 深黑背景
    colors.foreground = Color::RGB(255, 255, 255);          // #ffffff - 纯白前景
    colors.current_line = Color::RGB(39, 46, 56);           // #272e38 - 明显当前行
    colors.selection = Color::RGB(54, 61, 71);              // #363d47 - 选中背景
    colors.line_number = Color::RGB(184, 191, 200);         // #b8bfc8 - 亮灰行号
    colors.line_number_current = Color::RGB(255, 255, 255); // #ffffff - 当前行号

    colors.statusbar_bg = Color::RGB(39, 46, 56);    // #272e38
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.menubar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.helpbar_bg = Color::RGB(39, 46, 56);
    colors.helpbar_fg = Color::RGB(184, 191, 200);  // #b8bfc8
    colors.helpbar_key = Color::RGB(100, 180, 255); // #64b4ff 亮蓝

    // GitHub Dark High Contrast 语法高亮：高饱和度色彩
    colors.keyword = Color::RGB(255, 150, 180);        // #ff96b4 亮粉
    colors.string = Color::RGB(180, 205, 220);         // #b4cddc 亮蓝
    colors.comment = Color::RGB(184, 191, 200);        // #b8bfc8 亮灰
    colors.number = Color::RGB(140, 210, 255);         // #8cd2ff 亮蓝
    colors.function = Color::RGB(230, 190, 255);       // #e6beff 亮紫
    colors.type = Color::RGB(80, 200, 190);            // #50c8be 亮青
    colors.operator_color = Color::RGB(255, 150, 180); // #ff96b4 亮粉

    colors.error = Color::RGB(255, 120, 120);  // #ff7878 亮红
    colors.warning = Color::RGB(255, 200, 80); // #ffc850 亮黄
    colors.info = Color::RGB(100, 180, 255);   // #64b4ff 亮蓝
    colors.success = Color::RGB(80, 210, 100); // #50d264 亮绿

    colors.dialog_bg = Color::RGB(39, 46, 56);          // #272e38
    colors.dialog_fg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_title_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #ffffff
    colors.dialog_border = Color::RGB(184, 191, 200);   // #b8bfc8

    return colors;
}

ThemeColors Theme::GitHubLightHighContrast() {
    ThemeColors colors;
    // GitHub Light High Contrast: GitHub 官方高对比度亮色主题，边界清晰
    colors.background = Color::RGB(255, 255, 255);    // #ffffff
    colors.foreground = Color::RGB(1, 4, 9);          // #010409
    colors.current_line = Color::RGB(235, 238, 242);  // #ebeef2
    colors.selection = Color::RGB(181, 210, 255);     // #b5d2ff
    colors.line_number = Color::RGB(143, 150, 159);   // #8f969f
    colors.line_number_current = Color::RGB(1, 4, 9); // #010409

    colors.statusbar_bg = Color::RGB(235, 238, 242); // #ebeef2
    colors.statusbar_fg = Color::RGB(1, 4, 9);       // #010409

    colors.menubar_bg = Color::RGB(255, 255, 255); // #ffffff
    colors.menubar_fg = Color::RGB(1, 4, 9);       // #010409

    colors.helpbar_bg = Color::RGB(235, 238, 242); // #ebeef2
    colors.helpbar_fg = Color::RGB(1, 4, 9);       // #010409
    colors.helpbar_key = Color::RGB(9, 105, 218);  // #0969da

    colors.keyword = Color::RGB(207, 43, 10);        // #cf2b0a - Red
    colors.string = Color::RGB(10, 109, 48);         // #0a6d30 - Green
    colors.comment = Color::RGB(106, 115, 125);      // #6a737d
    colors.number = Color::RGB(9, 105, 218);         // #0969da
    colors.function = Color::RGB(130, 33, 169);      // #8221a9 - Purple
    colors.type = Color::RGB(10, 109, 48);           // #0a6d30
    colors.operator_color = Color::RGB(207, 43, 10); // #cf2b0a

    colors.error = Color::RGB(207, 43, 10);   // #cf2b0a
    colors.warning = Color::RGB(191, 107, 0); // #bf6b00
    colors.info = Color::RGB(9, 105, 218);    // #0969da
    colors.success = Color::RGB(10, 109, 48); // #0a6d30

    colors.dialog_bg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_fg = Color::RGB(1, 4, 9);             // #010409
    colors.dialog_title_bg = Color::RGB(235, 238, 242); // #ebeef2
    colors.dialog_title_fg = Color::RGB(1, 4, 9);       // #010409
    colors.dialog_border = Color::RGB(143, 150, 159);   // #8f969f

    return colors;
}

ThemeColors Theme::GitHubColorblind() {
    ThemeColors colors;
    // GitHub Colorblind: 专为色盲用户优化的主题，使用更易区分的颜色
    colors.background = Color::RGB(13, 17, 23);             // #0d1117
    colors.foreground = Color::RGB(230, 237, 243);          // #e6edf3
    colors.current_line = Color::RGB(22, 27, 34);           // #161b22
    colors.selection = Color::RGB(33, 38, 45);              // #21262d
    colors.line_number = Color::RGB(139, 148, 158);         // #8b949e
    colors.line_number_current = Color::RGB(230, 237, 243); // #e6edf3

    colors.statusbar_bg = Color::RGB(22, 27, 34);    // #161b22
    colors.statusbar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.menubar_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.menubar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.helpbar_bg = Color::RGB(22, 27, 34);     // #161b22
    colors.helpbar_fg = Color::RGB(230, 237, 243);  // #e6edf3
    colors.helpbar_key = Color::RGB(255, 184, 108); // #ffb86c

    colors.keyword = Color::RGB(255, 184, 108);        // #ffb86c - Orange (易区分)
    colors.string = Color::RGB(163, 203, 255);         // #a3cbff - Blue (易区分)
    colors.comment = Color::RGB(139, 148, 158);        // #8b949e
    colors.number = Color::RGB(163, 203, 255);         // #a3cbff
    colors.function = Color::RGB(255, 136, 32);        // #ff8820 - Orange
    colors.type = Color::RGB(163, 203, 255);           // #a3cbff
    colors.operator_color = Color::RGB(255, 184, 108); // #ffb86c

    colors.error = Color::RGB(255, 136, 32);    // #ff8820 (Orange 替代 Red)
    colors.warning = Color::RGB(255, 215, 0);   // #ffd700
    colors.info = Color::RGB(88, 166, 255);     // #58a6ff
    colors.success = Color::RGB(163, 203, 255); // #a3cbff

    colors.dialog_bg = Color::RGB(22, 27, 34);          // #161b22
    colors.dialog_fg = Color::RGB(230, 237, 243);       // #e6edf3
    colors.dialog_title_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.dialog_title_fg = Color::RGB(230, 237, 243); // #e6edf3
    colors.dialog_border = Color::RGB(139, 148, 158);   // #8b949e

    return colors;
}

ThemeColors Theme::GitHubTritanopia() {
    ThemeColors colors;
    // GitHub Tritanopia: 专为红绿色盲优化的主题
    colors.background = Color::RGB(13, 17, 23);             // #0d1117
    colors.foreground = Color::RGB(230, 237, 243);          // #e6edf3
    colors.current_line = Color::RGB(22, 27, 34);           // #161b22
    colors.selection = Color::RGB(33, 38, 45);              // #21262d
    colors.line_number = Color::RGB(139, 148, 158);         // #8b949e
    colors.line_number_current = Color::RGB(230, 237, 243); // #e6edf3

    colors.statusbar_bg = Color::RGB(22, 27, 34);    // #161b22
    colors.statusbar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.menubar_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.menubar_fg = Color::RGB(230, 237, 243); // #e6edf3

    colors.helpbar_bg = Color::RGB(22, 27, 34);     // #161b22
    colors.helpbar_fg = Color::RGB(230, 237, 243);  // #e6edf3
    colors.helpbar_key = Color::RGB(255, 184, 108); // #ffb86c

    colors.keyword = Color::RGB(255, 184, 108);        // #ffb86c - Orange
    colors.string = Color::RGB(88, 166, 255);          // #58a6ff - Blue
    colors.comment = Color::RGB(139, 148, 158);        // #8b949e
    colors.number = Color::RGB(88, 166, 255);          // #58a6ff
    colors.function = Color::RGB(255, 136, 32);        // #ff8820
    colors.type = Color::RGB(88, 166, 255);            // #58a6ff
    colors.operator_color = Color::RGB(255, 184, 108); // #ffb86c

    colors.error = Color::RGB(255, 136, 32);   // #ff8820
    colors.warning = Color::RGB(255, 215, 0);  // #ffd700
    colors.info = Color::RGB(88, 166, 255);    // #58a6ff
    colors.success = Color::RGB(88, 166, 255); // #58a6ff

    colors.dialog_bg = Color::RGB(22, 27, 34);          // #161b22
    colors.dialog_fg = Color::RGB(230, 237, 243);       // #e6edf3
    colors.dialog_title_bg = Color::RGB(13, 17, 23);    // #0d1117
    colors.dialog_title_fg = Color::RGB(230, 237, 243); // #e6edf3
    colors.dialog_border = Color::RGB(139, 148, 158);   // #8b949e

    return colors;
}

ThemeColors Theme::GitHubSoft() {
    ThemeColors colors;
    // GitHub Soft: GitHub 柔和暗色主题，降低对比度，适合长时间使用
    colors.background = Color::RGB(20, 24, 30);             // #14181e
    colors.foreground = Color::RGB(210, 215, 220);          // #d2d7dc
    colors.current_line = Color::RGB(28, 33, 40);           // #1c2128
    colors.selection = Color::RGB(38, 43, 50);              // #262b32
    colors.line_number = Color::RGB(120, 130, 140);         // #78828c
    colors.line_number_current = Color::RGB(210, 215, 220); // #d2d7dc

    colors.statusbar_bg = Color::RGB(28, 33, 40);    // #1c2128
    colors.statusbar_fg = Color::RGB(210, 215, 220); // #d2d7dc

    colors.menubar_bg = Color::RGB(20, 24, 30);    // #14181e
    colors.menubar_fg = Color::RGB(210, 215, 220); // #d2d7dc

    colors.helpbar_bg = Color::RGB(28, 33, 40);     // #1c2128
    colors.helpbar_fg = Color::RGB(180, 190, 200);  // #b4bec8
    colors.helpbar_key = Color::RGB(120, 180, 240); // #78b4f0

    colors.keyword = Color::RGB(240, 140, 120);        // #f08c78 - Soft Red
    colors.string = Color::RGB(140, 200, 140);         // #8cc88c - Soft Green
    colors.comment = Color::RGB(120, 130, 140);        // #78828c - Soft Gray
    colors.number = Color::RGB(120, 180, 240);         // #78b4f0 - Soft Blue
    colors.function = Color::RGB(180, 140, 220);       // #b48cdc - Soft Purple
    colors.type = Color::RGB(140, 200, 140);           // #8cc88c
    colors.operator_color = Color::RGB(240, 140, 120); // #f08c78

    colors.error = Color::RGB(240, 140, 120);   // #f08c78
    colors.warning = Color::RGB(240, 180, 100); // #fcb464
    colors.info = Color::RGB(120, 180, 240);    // #78b4f0
    colors.success = Color::RGB(140, 200, 140); // #8cc88c

    colors.dialog_bg = Color::RGB(28, 33, 40);          // #1c2128
    colors.dialog_fg = Color::RGB(210, 215, 220);       // #d2d7dc
    colors.dialog_title_bg = Color::RGB(20, 24, 30);    // #14181e
    colors.dialog_title_fg = Color::RGB(210, 215, 220); // #d2d7dc
    colors.dialog_border = Color::RGB(50, 60, 70);      // #323c46

    return colors;
}

ThemeColors Theme::GitHubMidnight() {
    ThemeColors colors;
    // GitHub Midnight: GitHub 深夜主题，更深的蓝色调
    colors.background = Color::RGB(10, 15, 25);             // #0a0f19
    colors.foreground = Color::RGB(220, 230, 240);          // #dce6f0
    colors.current_line = Color::RGB(18, 25, 35);           // #121923
    colors.selection = Color::RGB(28, 35, 50);              // #1c2332
    colors.line_number = Color::RGB(110, 125, 140);         // #6e7d8c
    colors.line_number_current = Color::RGB(220, 230, 240); // #dce6f0

    colors.statusbar_bg = Color::RGB(18, 25, 35);    // #121923
    colors.statusbar_fg = Color::RGB(220, 230, 240); // #dce6f0

    colors.menubar_bg = Color::RGB(10, 15, 25);    // #0a0f19
    colors.menubar_fg = Color::RGB(220, 230, 240); // #dce6f0

    colors.helpbar_bg = Color::RGB(18, 25, 35);     // #121923
    colors.helpbar_fg = Color::RGB(170, 185, 200);  // #aab9c8
    colors.helpbar_key = Color::RGB(100, 170, 250); // #64aafa

    colors.keyword = Color::RGB(255, 130, 110);        // #ff826e - Midnight Red
    colors.string = Color::RGB(130, 210, 130);         // #82d282 - Midnight Green
    colors.comment = Color::RGB(110, 125, 140);        // #6e7d8c - Midnight Gray
    colors.number = Color::RGB(100, 170, 250);         // #64aafa - Midnight Blue
    colors.function = Color::RGB(170, 130, 230);       // #aa82e6 - Midnight Purple
    colors.type = Color::RGB(130, 210, 130);           // #82d282
    colors.operator_color = Color::RGB(255, 130, 110); // #ff826e

    colors.error = Color::RGB(255, 130, 110);   // #ff826e
    colors.warning = Color::RGB(255, 190, 90);  // #ffbe5a
    colors.info = Color::RGB(100, 170, 250);    // #64aafa
    colors.success = Color::RGB(130, 210, 130); // #82d282

    colors.dialog_bg = Color::RGB(18, 25, 35);          // #121923
    colors.dialog_fg = Color::RGB(220, 230, 240);       // #dce6f0
    colors.dialog_title_bg = Color::RGB(10, 15, 25);    // #0a0f19
    colors.dialog_title_fg = Color::RGB(220, 230, 240); // #dce6f0
    colors.dialog_border = Color::RGB(40, 55, 75);      // #28374b

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

ThemeColors Theme::VSCodeLight() {
    ThemeColors colors;
    // VSCode Light: VSCode 官方亮色主题，清爽明亮
    colors.background = Color::RGB(255, 255, 255);       // #ffffff
    colors.foreground = Color::RGB(64, 64, 64);          // #404040
    colors.current_line = Color::RGB(232, 232, 232);     // #e8e8e8
    colors.selection = Color::RGB(179, 219, 255);        // #b3dbff
    colors.line_number = Color::RGB(128, 128, 128);      // #808080
    colors.line_number_current = Color::RGB(64, 64, 64); // #404040

    colors.statusbar_bg = Color::RGB(0, 120, 215);   // #0078d7
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(221, 221, 221); // #dddddd
    colors.menubar_fg = Color::RGB(0, 0, 0);       // #000000

    colors.helpbar_bg = Color::RGB(244, 244, 244); // #f4f4f4
    colors.helpbar_fg = Color::RGB(100, 100, 100); // #646464
    colors.helpbar_key = Color::RGB(0, 120, 215);  // #0078d7

    colors.keyword = Color::RGB(0, 0, 255);         // #0000ff - 蓝色关键字
    colors.string = Color::RGB(163, 21, 21);        // #a31515 - 红色字符串
    colors.comment = Color::RGB(0, 128, 0);         // #008000 - 绿色注释
    colors.number = Color::RGB(9, 134, 88);         // #098658 - 青色数字
    colors.function = Color::RGB(121, 93, 141);     // #795d8f - 紫色函数
    colors.type = Color::RGB(43, 145, 175);         // #2b91af - 蓝色类型
    colors.operator_color = Color::RGB(64, 64, 64); // #404040

    colors.error = Color::RGB(229, 84, 84);   // #e55454
    colors.warning = Color::RGB(230, 160, 0); // #e6a000
    colors.info = Color::RGB(0, 120, 215);    // #0078d7
    colors.success = Color::RGB(34, 134, 58); // #22863a

    colors.dialog_bg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_fg = Color::RGB(64, 64, 64);          // #404040
    colors.dialog_title_bg = Color::RGB(221, 221, 221); // #dddddd
    colors.dialog_title_fg = Color::RGB(0, 0, 0);       // #000000
    colors.dialog_border = Color::RGB(200, 200, 200);   // #c8c8c8

    return colors;
}

ThemeColors Theme::VSCodeLightModern() {
    ThemeColors colors;
    // VSCode Light Modern: 现代化亮色主题，柔和对比
    colors.background = Color::RGB(250, 250, 250);       // #fafafa
    colors.foreground = Color::RGB(61, 61, 61);          // #3d3d3d
    colors.current_line = Color::RGB(235, 235, 235);     // #ebebeb
    colors.selection = Color::RGB(200, 220, 240);        // #c8dcf0
    colors.line_number = Color::RGB(130, 130, 130);      // #828282
    colors.line_number_current = Color::RGB(61, 61, 61); // #3d3d3d

    colors.statusbar_bg = Color::RGB(70, 130, 180);  // #4682b4
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(230, 230, 230); // #e6e6e6
    colors.menubar_fg = Color::RGB(50, 50, 50);    // #323232

    colors.helpbar_bg = Color::RGB(240, 240, 240); // #f0f0f0
    colors.helpbar_fg = Color::RGB(110, 110, 110); // #6e6e6e
    colors.helpbar_key = Color::RGB(70, 130, 180); // #4682b4

    colors.keyword = Color::RGB(86, 61, 149);       // #563d95 - 紫色关键字
    colors.string = Color::RGB(199, 41, 41);        // #c72929 - 红色字符串
    colors.comment = Color::RGB(106, 153, 85);      // #6a9955 - 绿色注释
    colors.number = Color::RGB(9, 134, 88);         // #098658 - 青色数字
    colors.function = Color::RGB(111, 81, 131);     // #6f5183 - 紫色函数
    colors.type = Color::RGB(43, 145, 175);         // #2b91af - 蓝色类型
    colors.operator_color = Color::RGB(61, 61, 61); // #3d3d3d

    colors.error = Color::RGB(235, 90, 90);   // #eb5a5a
    colors.warning = Color::RGB(235, 170, 0); // #ebaa00
    colors.info = Color::RGB(70, 130, 180);   // #4682b4
    colors.success = Color::RGB(40, 140, 60); // #288c3c

    colors.dialog_bg = Color::RGB(255, 255, 255);       // #ffffff
    colors.dialog_fg = Color::RGB(61, 61, 61);          // #3d3d3d
    colors.dialog_title_bg = Color::RGB(230, 230, 230); // #e6e6e6
    colors.dialog_title_fg = Color::RGB(50, 50, 50);    // #323232
    colors.dialog_border = Color::RGB(200, 200, 200);   // #c8c8c8

    return colors;
}

ThemeColors Theme::VSCodeDarkModern() {
    ThemeColors colors;
    // VSCode Dark Modern: 现代化暗色主题，柔和对比
    colors.background = Color::RGB(30, 30, 30);             // #1e1e1e
    colors.foreground = Color::RGB(204, 204, 204);          // #cccccc
    colors.current_line = Color::RGB(40, 40, 40);           // #282828
    colors.selection = Color::RGB(50, 60, 80);              // #323c50
    colors.line_number = Color::RGB(128, 128, 128);         // #808080
    colors.line_number_current = Color::RGB(204, 204, 204); // #cccccc

    colors.statusbar_bg = Color::RGB(0, 120, 215);   // #0078d7
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(38, 38, 38);    // #262626
    colors.menubar_fg = Color::RGB(204, 204, 204); // #cccccc

    colors.helpbar_bg = Color::RGB(38, 38, 38);    // #262626
    colors.helpbar_fg = Color::RGB(150, 150, 150); // #969696
    colors.helpbar_key = Color::RGB(70, 150, 220); // #4696dc

    colors.keyword = Color::RGB(86, 156, 214);         // #569cd6 - 蓝色关键字
    colors.string = Color::RGB(206, 145, 120);         // #ce9178 - 橙色字符串
    colors.comment = Color::RGB(106, 153, 85);         // #6a9955 - 绿色注释
    colors.number = Color::RGB(181, 206, 168);         // #b5ceaa - 浅绿数字
    colors.function = Color::RGB(220, 220, 170);       // #dcdcaa - 黄色函数
    colors.type = Color::RGB(78, 201, 176);            // #4ec9b0 - 青色类型
    colors.operator_color = Color::RGB(204, 204, 204); // #cccccc

    colors.error = Color::RGB(244, 67, 54);     // #f44336
    colors.warning = Color::RGB(255, 183, 77);  // #ffb74d
    colors.info = Color::RGB(70, 150, 220);     // #4696dc
    colors.success = Color::RGB(100, 180, 100); // #64b464

    colors.dialog_bg = Color::RGB(40, 40, 40);          // #282828
    colors.dialog_fg = Color::RGB(204, 204, 204);       // #cccccc
    colors.dialog_title_bg = Color::RGB(30, 30, 30);    // #1e1e1e
    colors.dialog_title_fg = Color::RGB(204, 204, 204); // #cccccc
    colors.dialog_border = Color::RGB(100, 100, 100);   // #646464

    return colors;
}

ThemeColors Theme::VSCodeMonokai() {
    ThemeColors colors;
    // VSCode Monokai: VSCode 内置 Monokai 主题
    colors.background = Color::RGB(39, 39, 39);             // #272727
    colors.foreground = Color::RGB(248, 248, 242);          // #f8f8f2
    colors.current_line = Color::RGB(50, 50, 50);           // #323232
    colors.selection = Color::RGB(70, 70, 70);              // #464646
    colors.line_number = Color::RGB(128, 128, 128);         // #808080
    colors.line_number_current = Color::RGB(248, 248, 242); // #f8f8f2

    colors.statusbar_bg = Color::RGB(72, 60, 50);    // #483c32
    colors.statusbar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.menubar_bg = Color::RGB(45, 45, 45);    // #2d2d2d
    colors.menubar_fg = Color::RGB(248, 248, 242); // #f8f8f2

    colors.helpbar_bg = Color::RGB(45, 45, 45);    // #2d2d2d
    colors.helpbar_fg = Color::RGB(180, 180, 180); // #b4b4b4
    colors.helpbar_key = Color::RGB(166, 226, 46); // #a6e22e

    colors.keyword = Color::RGB(249, 38, 114);        // #f92672 - 粉红关键字
    colors.string = Color::RGB(230, 219, 116);        // #e6db74 - 黄色字符串
    colors.comment = Color::RGB(117, 113, 94);        // #75715e - 褐色注释
    colors.number = Color::RGB(174, 129, 255);        // #ae81ff - 紫色数字
    colors.function = Color::RGB(166, 226, 46);       // #a6e22e - 绿色函数
    colors.type = Color::RGB(102, 217, 239);          // #66d9ef - 青色类型
    colors.operator_color = Color::RGB(249, 38, 114); // #f92672

    colors.error = Color::RGB(249, 38, 114);   // #f92672
    colors.warning = Color::RGB(253, 151, 31); // #fd971f
    colors.info = Color::RGB(102, 217, 239);   // #66d9ef
    colors.success = Color::RGB(166, 226, 46); // #a6e22e

    colors.dialog_bg = Color::RGB(50, 50, 50);          // #323232
    colors.dialog_fg = Color::RGB(248, 248, 242);       // #f8f8f2
    colors.dialog_title_bg = Color::RGB(39, 39, 39);    // #272727
    colors.dialog_title_fg = Color::RGB(248, 248, 242); // #f8f8f2
    colors.dialog_border = Color::RGB(100, 100, 100);   // #646464

    return colors;
}

ThemeColors Theme::VSCodeDarkPlus() {
    ThemeColors colors;
    // VSCode Dark+: VSCode 默认 Dark Plus 主题，增强对比度
    colors.background = Color::RGB(30, 30, 30);             // #1e1e1e
    colors.foreground = Color::RGB(212, 212, 212);          // #d4d4d4
    colors.current_line = Color::RGB(42, 42, 42);           // #2a2a2a
    colors.selection = Color::RGB(54, 54, 54);              // #363636
    colors.line_number = Color::RGB(135, 135, 135);         // #878787
    colors.line_number_current = Color::RGB(212, 212, 212); // #d4d4d4

    colors.statusbar_bg = Color::RGB(0, 120, 215);   // #0078d7
    colors.statusbar_fg = Color::RGB(255, 255, 255); // #ffffff

    colors.menubar_bg = Color::RGB(38, 38, 38);    // #262626
    colors.menubar_fg = Color::RGB(212, 212, 212); // #d4d4d4

    colors.helpbar_bg = Color::RGB(38, 38, 38);    // #262626
    colors.helpbar_fg = Color::RGB(160, 160, 160); // #a0a0a0
    colors.helpbar_key = Color::RGB(80, 160, 230); // #50a0e6

    colors.keyword = Color::RGB(86, 156, 214);         // #569cd6 - 蓝色关键字
    colors.string = Color::RGB(206, 145, 120);         // #ce9178 - 橙色字符串
    colors.comment = Color::RGB(106, 153, 85);         // #6a9955 - 绿色注释
    colors.number = Color::RGB(181, 206, 168);         // #b5ceaa - 浅绿数字
    colors.function = Color::RGB(220, 220, 170);       // #dcdcaa - 黄色函数
    colors.type = Color::RGB(78, 201, 176);            // #4ec9b0 - 青色类型
    colors.operator_color = Color::RGB(212, 212, 212); // #d4d4d4

    colors.error = Color::RGB(244, 67, 54);     // #f44336
    colors.warning = Color::RGB(255, 203, 107); // #ffcb6b
    colors.info = Color::RGB(80, 160, 230);     // #50a0e6
    colors.success = Color::RGB(110, 190, 110); // #6ebe6e

    colors.dialog_bg = Color::RGB(42, 42, 42);          // #2a2a2a
    colors.dialog_fg = Color::RGB(212, 212, 212);       // #d4d4d4
    colors.dialog_title_bg = Color::RGB(30, 30, 30);    // #1e1e1e
    colors.dialog_title_fg = Color::RGB(212, 212, 212); // #d4d4d4
    colors.dialog_border = Color::RGB(90, 90, 90);      // #5a5a5a

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

ThemeColors Theme::KanagawaLight() {
    ThemeColors colors;
    // Kanagawa Light: 明亮版本，柔和的浅色背景
    colors.background = Color::RGB(243, 244, 231);   // #F3F4E7 springWhite
    colors.foreground = Color::RGB(72, 71, 61);      // #48473D sumiInk1
    colors.current_line = Color::RGB(220, 222, 212); // #DCDED4 waveGray1
    colors.selection = Color::RGB(209, 213, 201);    // #D1D5C9 waveGray2
    colors.line_number = Color::RGB(163, 162, 152);  // #A3A298 fujiGrayLight
    colors.line_number_current = Color::RGB(72, 71, 61);

    colors.statusbar_bg = Color::RGB(220, 222, 212);
    colors.statusbar_fg = Color::RGB(72, 71, 61);

    colors.menubar_bg = Color::RGB(243, 244, 231);
    colors.menubar_fg = Color::RGB(72, 71, 61);

    colors.helpbar_bg = Color::RGB(220, 222, 212);
    colors.helpbar_fg = Color::RGB(114, 113, 105);
    colors.helpbar_key = Color::RGB(67, 128, 174); // #4380AE waveBlue

    // Kanagawa Light 语法：深紫关键词/春绿字符串/蓝函数/橙黄数字
    colors.keyword = Color::RGB(92, 70, 140);        // #5C468C oniVioletDark
    colors.string = Color::RGB(78, 120, 57);         // #4E7839 springGreenDark
    colors.comment = Color::RGB(114, 113, 105);      // #727169 fujiGray
    colors.number = Color::RGB(179, 121, 43);        // #B3792B autumnYellowDark
    colors.function = Color::RGB(57, 95, 156);       // #395F9C crystalBlueDark
    colors.type = Color::RGB(67, 128, 174);          // #4380AE springBlue
    colors.operator_color = Color::RGB(57, 103, 91); // #39675B waveAquaDark

    colors.error = Color::RGB(192, 32, 32);    // #C02020 samuraiRedDark
    colors.warning = Color::RGB(204, 120, 10); // #CC780A roninYellowDark
    colors.info = Color::RGB(57, 95, 156);     // #395F9C crystalBlueDark
    colors.success = Color::RGB(78, 120, 57);  // #4E7839 springGreenDark

    colors.dialog_bg = Color::RGB(220, 222, 212);
    colors.dialog_fg = Color::RGB(72, 71, 61);
    colors.dialog_title_bg = Color::RGB(209, 213, 201);
    colors.dialog_title_fg = Color::RGB(72, 71, 61);
    colors.dialog_border = Color::RGB(163, 162, 152);

    return colors;
}

ThemeColors Theme::KanagawaSakura() {
    ThemeColors colors;
    // Kanagawa Sakura: 樱花主题，粉色系变体
    colors.background = Color::RGB(29, 29, 37);     // #1D1D25 sakuraInk
    colors.foreground = Color::RGB(225, 220, 191);  // #E1DCBF sakuraWhite
    colors.current_line = Color::RGB(45, 40, 55);   // #2D2837 sakuraPurple1
    colors.selection = Color::RGB(60, 50, 70);      // #3C3246 sakuraPurple2
    colors.line_number = Color::RGB(120, 115, 110); // #78736E sakuraGray
    colors.line_number_current = Color::RGB(240, 180, 200);

    colors.statusbar_bg = Color::RGB(36, 36, 46);
    colors.statusbar_fg = Color::RGB(225, 220, 191);

    colors.menubar_bg = Color::RGB(29, 29, 37);
    colors.menubar_fg = Color::RGB(225, 220, 191);

    colors.helpbar_bg = Color::RGB(36, 36, 46);
    colors.helpbar_fg = Color::RGB(120, 115, 110);
    colors.helpbar_key = Color::RGB(240, 180, 200); // #F0B4C8 sakuraPink

    // Sakura 语法：粉紫关键词/嫩绿字符串/淡蓝函数/橙黄数字
    colors.keyword = Color::RGB(180, 140, 200);        // #B48CC8 sakuraViolet
    colors.string = Color::RGB(160, 200, 120);         // #A0C878 sakuraGreen
    colors.comment = Color::RGB(120, 115, 110);        // #78736E sakuraGray
    colors.number = Color::RGB(240, 180, 100);         // #F0B464 sakuraOrange
    colors.function = Color::RGB(140, 180, 220);       // #8CB4DC sakuraBlue
    colors.type = Color::RGB(240, 180, 200);           // #F0B4C8 sakuraPink
    colors.operator_color = Color::RGB(130, 180, 170); // #82B4AA sakuraAqua

    colors.error = Color::RGB(240, 100, 120);   // #F06478 sakuraRed
    colors.warning = Color::RGB(255, 170, 80);  // #FFAA50 sakuraAmber
    colors.info = Color::RGB(140, 180, 220);    // #8CB4DC sakuraBlue
    colors.success = Color::RGB(130, 180, 140); // #82B48C sakuraMint

    colors.dialog_bg = Color::RGB(45, 40, 55);
    colors.dialog_fg = Color::RGB(225, 220, 191);
    colors.dialog_title_bg = Color::RGB(60, 50, 70);
    colors.dialog_title_fg = Color::RGB(225, 220, 191);
    colors.dialog_border = Color::RGB(120, 115, 110);

    return colors;
}

ThemeColors Theme::KanagawaMidnight() {
    ThemeColors colors;
    // Kanagawa Midnight: 深蓝午夜版本
    colors.background = Color::RGB(15, 15, 25);    // #0F0F19 midnightInk
    colors.foreground = Color::RGB(215, 210, 181); // #D7D2B5 midnightWhite
    colors.current_line = Color::RGB(25, 30, 50);  // #191E32 midnightBlue1
    colors.selection = Color::RGB(35, 45, 70);     // #232D46 midnightBlue2
    colors.line_number = Color::RGB(95, 95, 105);  // #5F5F69 midnightGray
    colors.line_number_current = Color::RGB(120, 160, 200);

    colors.statusbar_bg = Color::RGB(20, 25, 40);
    colors.statusbar_fg = Color::RGB(215, 210, 181);

    colors.menubar_bg = Color::RGB(15, 15, 25);
    colors.menubar_fg = Color::RGB(215, 210, 181);

    colors.helpbar_bg = Color::RGB(20, 25, 40);
    colors.helpbar_fg = Color::RGB(95, 95, 105);
    colors.helpbar_key = Color::RGB(100, 150, 200); // #6496C8 midnightAzure

    // Midnight 语法：蓝紫关键词/青绿字符串/亮蓝函数/金黄数字
    colors.keyword = Color::RGB(130, 110, 180);        // #826EB4 midnightViolet
    colors.string = Color::RGB(130, 170, 100);         // #82AA64 midnightGreen
    colors.comment = Color::RGB(95, 95, 105);          // #5F5F69 midnightGray
    colors.number = Color::RGB(220, 160, 80);          // #DCA050 midnightGold
    colors.function = Color::RGB(100, 140, 200);       // #648CC8 midnightBlue
    colors.type = Color::RGB(120, 160, 200);           // #78A0C8 midnightSky
    colors.operator_color = Color::RGB(100, 150, 140); // #64968C midnightTeal

    colors.error = Color::RGB(220, 60, 60);     // #DC3C3C midnightRed
    colors.warning = Color::RGB(240, 140, 60);  // #F08C3C midnightOrange
    colors.info = Color::RGB(100, 140, 200);    // #648CC8 midnightBlue
    colors.success = Color::RGB(110, 150, 100); // #6E9664 midnightOlive

    colors.dialog_bg = Color::RGB(25, 30, 50);
    colors.dialog_fg = Color::RGB(215, 210, 181);
    colors.dialog_title_bg = Color::RGB(35, 45, 70);
    colors.dialog_title_fg = Color::RGB(215, 210, 181);
    colors.dialog_border = Color::RGB(95, 95, 105);

    return colors;
}

ThemeColors Theme::KanagawaFuji() {
    ThemeColors colors;
    // Kanagawa Fuji: 富士山主题，冷色调
    colors.background = Color::RGB(20, 22, 30);     // #14161E fujiInk
    colors.foreground = Color::RGB(218, 213, 184);  // #DAD5B8 fujiWhite
    colors.current_line = Color::RGB(30, 38, 55);   // #1E2637 fujiBlue1
    colors.selection = Color::RGB(40, 55, 75);      // #28374B fujiBlue2
    colors.line_number = Color::RGB(105, 110, 115); // #696E73 fujiMist
    colors.line_number_current = Color::RGB(180, 200, 220);

    colors.statusbar_bg = Color::RGB(28, 33, 45);
    colors.statusbar_fg = Color::RGB(218, 213, 184);

    colors.menubar_bg = Color::RGB(20, 22, 30);
    colors.menubar_fg = Color::RGB(218, 213, 184);

    colors.helpbar_bg = Color::RGB(28, 33, 45);
    colors.helpbar_fg = Color::RGB(105, 110, 115);
    colors.helpbar_key = Color::RGB(140, 180, 210); // #8CB4D2 fujiIce

    // Fuji 语法：冰紫关键词/雪青字符串/霜蓝函数/冰黄数字
    colors.keyword = Color::RGB(140, 130, 185);        // #8C82B9 fujiLavender
    colors.string = Color::RGB(145, 185, 165);         // #91B9A5 fujiMint
    colors.comment = Color::RGB(105, 110, 115);        // #696E73 fujiMist
    colors.number = Color::RGB(210, 170, 120);         // #D2AA78 fujiSand
    colors.function = Color::RGB(120, 160, 200);       // #78A0C8 fujiFrost
    colors.type = Color::RGB(140, 180, 210);           // #8CB4D2 fujiIce
    colors.operator_color = Color::RGB(115, 165, 155); // #73A59B fujiSpring

    colors.error = Color::RGB(210, 80, 90);     // #D2505A fujiCrimson
    colors.warning = Color::RGB(235, 150, 70);  // #EB9646 fujiAmber
    colors.info = Color::RGB(120, 160, 200);    // #78A0C8 fujiFrost
    colors.success = Color::RGB(125, 170, 145); // #7DAA91 fujiJade

    colors.dialog_bg = Color::RGB(30, 38, 55);
    colors.dialog_fg = Color::RGB(218, 213, 184);
    colors.dialog_title_bg = Color::RGB(40, 55, 75);
    colors.dialog_title_fg = Color::RGB(218, 213, 184);
    colors.dialog_border = Color::RGB(105, 110, 115);

    return colors;
}

ThemeColors Theme::KanagawaWave() {
    ThemeColors colors;
    // Kanagawa Wave: 波浪主题，强调蓝绿色调
    colors.background = Color::RGB(18, 26, 32);     // #121A20 waveInk
    colors.foreground = Color::RGB(216, 217, 188);  // #D8D9BC waveWhite
    colors.current_line = Color::RGB(28, 42, 55);   // #1C2A37 waveBlue1
    colors.selection = Color::RGB(38, 58, 72);      // #263A48 waveBlue2
    colors.line_number = Color::RGB(100, 115, 120); // #647378 waveGray
    colors.line_number_current = Color::RGB(130, 185, 205);

    colors.statusbar_bg = Color::RGB(25, 38, 48);
    colors.statusbar_fg = Color::RGB(216, 217, 188);

    colors.menubar_bg = Color::RGB(18, 26, 32);
    colors.menubar_fg = Color::RGB(216, 217, 188);

    colors.helpbar_bg = Color::RGB(25, 38, 48);
    colors.helpbar_fg = Color::RGB(100, 115, 120);
    colors.helpbar_key = Color::RGB(130, 185, 205); // #82B9CD waveCyan

    // Wave 语法：海紫关键词/海藻绿字符串/海浪蓝函数/贝壳黄数字
    colors.keyword = Color::RGB(135, 125, 180);        // #877DB4 wavePurple
    colors.string = Color::RGB(140, 180, 150);         // #8CB496 waveSeaGreen
    colors.comment = Color::RGB(100, 115, 120);        // #647378 waveGray
    colors.number = Color::RGB(215, 165, 100);         // #D7A564 waveShell
    colors.function = Color::RGB(110, 155, 195);       // #6E9BC3 waveSurf
    colors.type = Color::RGB(130, 185, 205);           // #82B9CD waveCyan
    colors.operator_color = Color::RGB(110, 165, 155); // #6EA59B waveAqua

    colors.error = Color::RGB(215, 70, 80);     // #D74650 waveCoral
    colors.warning = Color::RGB(235, 145, 65);  // #EB9141 waveAmber
    colors.info = Color::RGB(110, 155, 195);    // #6E9BC3 waveSurf
    colors.success = Color::RGB(120, 165, 135); // #78A587 waveKelp

    colors.dialog_bg = Color::RGB(28, 42, 55);
    colors.dialog_fg = Color::RGB(216, 217, 188);
    colors.dialog_title_bg = Color::RGB(38, 58, 72);
    colors.dialog_title_fg = Color::RGB(216, 217, 188);
    colors.dialog_border = Color::RGB(100, 115, 120);

    return colors;
}

ThemeColors Theme::KanagawaOni() {
    ThemeColors colors;
    // Kanagawa Oni: 鬼主题，深紫色调，更加激进
    colors.background = Color::RGB(22, 20, 30);     // #16141E oniInk
    colors.foreground = Color::RGB(222, 217, 188);  // #DED9BC oniWhite
    colors.current_line = Color::RGB(38, 32, 52);   // #262034 oniPurple1
    colors.selection = Color::RGB(52, 42, 68);      // #342A44 oniPurple2
    colors.line_number = Color::RGB(110, 105, 115); // #6E6973 oniGray
    colors.line_number_current = Color::RGB(180, 140, 200);

    colors.statusbar_bg = Color::RGB(32, 28, 45);
    colors.statusbar_fg = Color::RGB(222, 217, 188);

    colors.menubar_bg = Color::RGB(22, 20, 30);
    colors.menubar_fg = Color::RGB(222, 217, 188);

    colors.helpbar_bg = Color::RGB(32, 28, 45);
    colors.helpbar_fg = Color::RGB(110, 105, 115);
    colors.helpbar_key = Color::RGB(160, 120, 190); // #A078BE oniViolet

    // Oni 语法：鬼紫关键词/冥绿字符串/幽灵蓝函数/魔黄数字
    colors.keyword = Color::RGB(160, 120, 200);        // #A078C8 oniViolet
    colors.string = Color::RGB(145, 185, 115);         // #91B973 oniShadow
    colors.comment = Color::RGB(110, 105, 115);        // #6E6973 oniGray
    colors.number = Color::RGB(230, 160, 90);          // #E6A05A oniGold
    colors.function = Color::RGB(130, 150, 210);       // #8296D2 oniSpirit
    colors.type = Color::RGB(160, 120, 200);           // #A078C8 oniViolet
    colors.operator_color = Color::RGB(125, 170, 160); // #7DAAA0 oniMystic

    colors.error = Color::RGB(230, 50, 70);     // #E63246 oniBlood
    colors.warning = Color::RGB(245, 155, 55);  // #F59B37 oniFire
    colors.info = Color::RGB(130, 150, 210);    // #8296D2 oniSpirit
    colors.success = Color::RGB(125, 170, 140); // #7DAA8C oniJade

    colors.dialog_bg = Color::RGB(38, 32, 52);
    colors.dialog_fg = Color::RGB(222, 217, 188);
    colors.dialog_title_bg = Color::RGB(52, 42, 68);
    colors.dialog_title_fg = Color::RGB(222, 217, 188);
    colors.dialog_border = Color::RGB(110, 105, 115);

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

    // 弹窗颜色 - 使用稍微不同����背景色来突出弹窗
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

ThemeColors Theme::Minions() {
    ThemeColors colors;
    // 小黄人主题：深蓝牛仔布基底 + 明黄/蓝色点缀（小黄人黄身子 + 蓝色背带裤）
    colors.background = Color::RGB(28, 40, 65);     // #1c2841 深蓝灰（牛仔布感）
    colors.foreground = Color::RGB(255, 248, 220);  // #fff8dc 玉米丝色（柔和白黄）
    colors.current_line = Color::RGB(38, 55, 88);   // #263758
    colors.selection = Color::RGB(65, 105, 225);    // #4169e1 皇家蓝（背带裤蓝）
    colors.line_number = Color::RGB(130, 150, 190); // 灰蓝
    colors.line_number_current = Color::RGB(255, 224, 102); // #ffe066 小黄人黄

    colors.statusbar_bg = Color::RGB(30, 50, 85);
    colors.statusbar_fg = Color::RGB(255, 248, 220);

    colors.menubar_bg = Color::RGB(25, 38, 62);
    colors.menubar_fg = Color::RGB(255, 248, 220);

    colors.helpbar_bg = Color::RGB(30, 50, 85);
    colors.helpbar_fg = Color::RGB(130, 150, 190);
    colors.helpbar_key = Color::RGB(255, 224, 102); // 小黄人黄

    // 小黄人配色：黄关键字/蓝字符串/黄函数/蓝类型
    colors.keyword = Color::RGB(255, 224, 102); // #ffe066 小黄人黄
    colors.string = Color::RGB(135, 206, 250);  // #87ceeb 天蓝
    colors.comment = Color::RGB(150, 170, 200);
    colors.number = Color::RGB(255, 215, 0);     // #ffd700 金黄
    colors.function = Color::RGB(100, 149, 237); // #6495ed 矢车菊蓝
    colors.type = Color::RGB(70, 130, 180);      // #4682b4 钢青蓝
    colors.operator_color = Color::RGB(255, 224, 102);

    colors.error = Color::RGB(255, 99, 71);   // #ff6347 番茄红（眼镜/反派）
    colors.warning = Color::RGB(255, 215, 0); // 金黄
    colors.info = Color::RGB(100, 149, 237);
    colors.success = Color::RGB(154, 205, 50); // #9acd32 黄绿（香蕉）

    colors.dialog_bg = Color::RGB(38, 55, 88);
    colors.dialog_fg = Color::RGB(255, 248, 220);
    colors.dialog_title_bg = Color::RGB(65, 105, 225); // 背带裤蓝
    colors.dialog_title_fg = Color::RGB(255, 248, 220);
    colors.dialog_border = Color::RGB(255, 224, 102); // 小黄人黄

    return colors;
}

ThemeColors Theme::Batman() {
    ThemeColors colors;
    // 蝙蝠侠主题：纯黑/深灰基底 + 金黄点缀（蝙蝠标志、腰带、哥谭夜色）
    colors.background = Color::RGB(18, 18, 18);           // #121212 近黑
    colors.foreground = Color::RGB(220, 218, 210);        // 柔和灰白
    colors.current_line = Color::RGB(28, 28, 28);         // #1c1c1c
    colors.selection = Color::RGB(45, 45, 48);            // 深灰
    colors.line_number = Color::RGB(90, 90, 90);          // 中灰
    colors.line_number_current = Color::RGB(255, 193, 7); // #ffc107 蝙蝠金

    colors.statusbar_bg = Color::RGB(25, 25, 25);
    colors.statusbar_fg = Color::RGB(220, 218, 210);

    colors.menubar_bg = Color::RGB(15, 15, 15);
    colors.menubar_fg = Color::RGB(220, 218, 210);

    colors.helpbar_bg = Color::RGB(25, 25, 25);
    colors.helpbar_fg = Color::RGB(90, 90, 90);
    colors.helpbar_key = Color::RGB(255, 193, 7); // 蝙蝠金

    // 蝙蝠侠配色：金关键字/琥珀字符串/灰注释/金数字
    colors.keyword = Color::RGB(255, 193, 7); // #ffc107 蝙蝠金
    colors.string = Color::RGB(255, 213, 79); // #ffd54f 琥珀黄
    colors.comment = Color::RGB(100, 100, 100);
    colors.number = Color::RGB(255, 193, 7);
    colors.function = Color::RGB(255, 213, 79);
    colors.type = Color::RGB(187, 134, 252); // 淡紫（哥谭霓虹）
    colors.operator_color = Color::RGB(255, 193, 7);

    colors.error = Color::RGB(244, 67, 54);   // 红（警灯/危险）
    colors.warning = Color::RGB(255, 152, 0); // 橙
    colors.info = Color::RGB(33, 150, 243);   // 蓝（夜空/信号）
    colors.success = Color::RGB(255, 193, 7); // 蝙蝠金

    colors.dialog_bg = Color::RGB(28, 28, 28);
    colors.dialog_fg = Color::RGB(220, 218, 210);
    colors.dialog_title_bg = Color::RGB(45, 45, 45);  // 深灰
    colors.dialog_title_fg = Color::RGB(255, 193, 7); // 蝙蝠金
    colors.dialog_border = Color::RGB(255, 193, 7);

    return colors;
}

ThemeColors Theme::SpongeBob() {
    ThemeColors colors;
    // 海绵宝宝主题：深海蓝基底 + 海绵黄/珊瑚粉/海青点缀（比奇堡海底风）
    colors.background = Color::RGB(13, 61, 110);           // #0d3d6e 深海蓝
    colors.foreground = Color::RGB(255, 250, 205);         // #fffacd 柠檬绸（海绵黄白）
    colors.current_line = Color::RGB(20, 80, 130);         // #145082
    colors.selection = Color::RGB(30, 100, 160);           // #1e64a0 海蓝
    colors.line_number = Color::RGB(100, 170, 220);        // #64aadc 浅海蓝
    colors.line_number_current = Color::RGB(255, 217, 61); // #ffd93d 海绵黄

    colors.statusbar_bg = Color::RGB(18, 75, 125);
    colors.statusbar_fg = Color::RGB(255, 250, 205);

    colors.menubar_bg = Color::RGB(10, 55, 95);
    colors.menubar_fg = Color::RGB(255, 250, 205);

    colors.helpbar_bg = Color::RGB(18, 75, 125);
    colors.helpbar_fg = Color::RGB(100, 170, 220);
    colors.helpbar_key = Color::RGB(255, 217, 61); // 海绵黄

    // 比奇堡配色：黄关键字/珊瑚粉字符串/海青函数/黄绿类型
    colors.keyword = Color::RGB(255, 217, 61);  // #ffd93d 海绵黄
    colors.string = Color::RGB(255, 107, 107);  // #ff6b6b 珊瑚粉（派大星屋）
    colors.comment = Color::RGB(116, 185, 255); // #74b9ff 浅海蓝
    colors.number = Color::RGB(255, 183, 77);   // #ffb74d 橙黄
    colors.function = Color::RGB(0, 184, 148);  // #00b894 海绿（珊迪）
    colors.type = Color::RGB(85, 239, 196);     // #55efc4 薄荷青
    colors.operator_color = Color::RGB(255, 217, 61);

    colors.error = Color::RGB(255, 107, 107);  // 珊瑚红
    colors.warning = Color::RGB(255, 183, 77); // 橙黄
    colors.info = Color::RGB(116, 185, 255);   // 海蓝
    colors.success = Color::RGB(0, 184, 148);  // 海绿

    colors.dialog_bg = Color::RGB(20, 80, 130);
    colors.dialog_fg = Color::RGB(255, 250, 205);
    colors.dialog_title_bg = Color::RGB(30, 100, 160); // 海蓝
    colors.dialog_title_fg = Color::RGB(255, 217, 61); // 海绵黄
    colors.dialog_border = Color::RGB(255, 217, 61);

    return colors;
}

ThemeColors Theme::ModusVivendi() {
    ThemeColors colors;
    // Modus Vivendi：高对比度、护眼深色主题（灵感来自 Emacs modus-themes）
    colors.background = Color::RGB(17, 17, 21);     // #111115 深灰黑
    colors.foreground = Color::RGB(239, 239, 239);  // #efefef 柔和白
    colors.current_line = Color::RGB(28, 28, 34);   // #1c1c22
    colors.selection = Color::RGB(51, 51, 63);      // #33333f
    colors.line_number = Color::RGB(115, 115, 135); // #737387
    colors.line_number_current = Color::RGB(239, 239, 239);

    colors.statusbar_bg = Color::RGB(28, 28, 34);
    colors.statusbar_fg = Color::RGB(239, 239, 239);

    colors.menubar_bg = Color::RGB(22, 22, 28);
    colors.menubar_fg = Color::RGB(239, 239, 239);

    colors.helpbar_bg = Color::RGB(28, 28, 34);
    colors.helpbar_fg = Color::RGB(115, 115, 135);
    colors.helpbar_key = Color::RGB(138, 226, 52); // 绿

    colors.keyword = Color::RGB(255, 121, 198);       // 粉
    colors.string = Color::RGB(166, 226, 46);         // 绿
    colors.comment = Color::RGB(115, 115, 135);       // 灰
    colors.number = Color::RGB(253, 151, 31);         // 橙
    colors.function = Color::RGB(138, 226, 52);       // 绿
    colors.type = Color::RGB(102, 217, 239);          // 青
    colors.operator_color = Color::RGB(249, 38, 114); // 红粉

    colors.error = Color::RGB(255, 85, 85);
    colors.warning = Color::RGB(253, 151, 31);
    colors.info = Color::RGB(102, 217, 239);
    colors.success = Color::RGB(138, 226, 52);

    colors.dialog_bg = Color::RGB(28, 28, 34);
    colors.dialog_fg = Color::RGB(239, 239, 239);
    colors.dialog_title_bg = Color::RGB(51, 51, 63);
    colors.dialog_title_fg = Color::RGB(239, 239, 239);
    colors.dialog_border = Color::RGB(115, 115, 135);

    return colors;
}

ThemeColors Theme::ModusOperandi() {
    ThemeColors colors;
    // Modus Operandi：高对比度、护眼浅色主题（Emacs modus-themes 浅色版）
    colors.background = Color::RGB(248, 248, 248);   // #f8f8f8 浅灰白
    colors.foreground = Color::RGB(34, 34, 34);      // #222222 深灰
    colors.current_line = Color::RGB(236, 236, 240); // #ececf0
    colors.selection = Color::RGB(220, 220, 228);    // #dcdce4
    colors.line_number = Color::RGB(115, 115, 135);  // #737387
    colors.line_number_current = Color::RGB(34, 34, 34);

    colors.statusbar_bg = Color::RGB(236, 236, 240);
    colors.statusbar_fg = Color::RGB(34, 34, 34);

    colors.menubar_bg = Color::RGB(242, 242, 246);
    colors.menubar_fg = Color::RGB(34, 34, 34);

    colors.helpbar_bg = Color::RGB(236, 236, 240);
    colors.helpbar_fg = Color::RGB(115, 115, 135);
    colors.helpbar_key = Color::RGB(0, 128, 0); // 深绿

    colors.keyword = Color::RGB(160, 0, 160);        // 紫
    colors.string = Color::RGB(0, 128, 0);           // 绿
    colors.comment = Color::RGB(115, 115, 135);      // 灰
    colors.number = Color::RGB(180, 80, 0);          // 棕橙
    colors.function = Color::RGB(0, 100, 80);        // 青绿
    colors.type = Color::RGB(0, 80, 160);            // 蓝
    colors.operator_color = Color::RGB(160, 0, 160); // 紫

    colors.error = Color::RGB(180, 0, 0);
    colors.warning = Color::RGB(180, 80, 0);
    colors.info = Color::RGB(0, 80, 160);
    colors.success = Color::RGB(0, 128, 0);

    colors.dialog_bg = Color::RGB(248, 248, 248);
    colors.dialog_fg = Color::RGB(34, 34, 34);
    colors.dialog_title_bg = Color::RGB(220, 220, 228);
    colors.dialog_title_fg = Color::RGB(34, 34, 34);
    colors.dialog_border = Color::RGB(115, 115, 135);

    return colors;
}

ThemeColors Theme::Horizon() {
    ThemeColors colors;
    // Horizon：暖色深色主题，粉/珊瑚/橙点缀（灵感来自 Horizon Theme）
    colors.background = Color::RGB(26, 24, 38);             // #1a1826
    colors.foreground = Color::RGB(203, 198, 215);          // #cbc6d7
    colors.current_line = Color::RGB(35, 33, 50);           // #232132
    colors.selection = Color::RGB(64, 61, 82);              // #403d52
    colors.line_number = Color::RGB(99, 94, 117);           // #635e75
    colors.line_number_current = Color::RGB(246, 224, 200); // #f6e0c8

    colors.statusbar_bg = Color::RGB(35, 33, 50);
    colors.statusbar_fg = Color::RGB(246, 224, 200);

    colors.menubar_bg = Color::RGB(30, 28, 44);
    colors.menubar_fg = Color::RGB(246, 224, 200);

    colors.helpbar_bg = Color::RGB(35, 33, 50);
    colors.helpbar_fg = Color::RGB(99, 94, 117);
    colors.helpbar_key = Color::RGB(159, 234, 159); // 薄荷绿

    colors.keyword = Color::RGB(233, 122, 178);        // 粉
    colors.string = Color::RGB(246, 224, 200);         // 米色
    colors.comment = Color::RGB(99, 94, 117);          // 灰紫
    colors.number = Color::RGB(250, 183, 149);         // 珊瑚橙
    colors.function = Color::RGB(159, 234, 159);       // 薄荷绿
    colors.type = Color::RGB(125, 207, 255);           // 天蓝
    colors.operator_color = Color::RGB(233, 122, 178); // 粉

    colors.error = Color::RGB(234, 118, 118);
    colors.warning = Color::RGB(250, 183, 149);
    colors.info = Color::RGB(125, 207, 255);
    colors.success = Color::RGB(159, 234, 159);

    colors.dialog_bg = Color::RGB(35, 33, 50);
    colors.dialog_fg = Color::RGB(203, 198, 215);
    colors.dialog_title_bg = Color::RGB(64, 61, 82);
    colors.dialog_title_fg = Color::RGB(246, 224, 200);
    colors.dialog_border = Color::RGB(99, 94, 117);

    return colors;
}

ThemeColors Theme::Oxocarbon() {
    ThemeColors colors;
    // Oxocarbon：IBM Carbon 风格，简洁深灰蓝，低饱和度
    colors.background = Color::RGB(22, 22, 24);    // #161618
    colors.foreground = Color::RGB(210, 216, 222); // #d2d8de
    colors.current_line = Color::RGB(32, 32, 36);  // #202024
    colors.selection = Color::RGB(51, 54, 60);     // #33363c
    colors.line_number = Color::RGB(82, 86, 94);   // #52565e
    colors.line_number_current = Color::RGB(210, 216, 222);

    colors.statusbar_bg = Color::RGB(32, 32, 36);
    colors.statusbar_fg = Color::RGB(210, 216, 222);

    colors.menubar_bg = Color::RGB(26, 26, 30);
    colors.menubar_fg = Color::RGB(210, 216, 222);

    colors.helpbar_bg = Color::RGB(32, 32, 36);
    colors.helpbar_fg = Color::RGB(82, 86, 94);
    colors.helpbar_key = Color::RGB(78, 205, 196); // 青

    colors.keyword = Color::RGB(255, 123, 114); // 珊瑚红
    colors.string = Color::RGB(168, 255, 96);   // 浅绿
    colors.comment = Color::RGB(82, 86, 94);    // 灰
    colors.number = Color::RGB(255, 184, 108);  // 橙
    colors.function = Color::RGB(78, 205, 196); // 青
    colors.type = Color::RGB(255, 154, 158);    // 粉
    colors.operator_color = Color::RGB(255, 123, 114);

    colors.error = Color::RGB(255, 123, 114);
    colors.warning = Color::RGB(255, 184, 108);
    colors.info = Color::RGB(78, 205, 196);
    colors.success = Color::RGB(168, 255, 96);

    colors.dialog_bg = Color::RGB(32, 32, 36);
    colors.dialog_fg = Color::RGB(210, 216, 222);
    colors.dialog_title_bg = Color::RGB(51, 54, 60);
    colors.dialog_title_fg = Color::RGB(210, 216, 222);
    colors.dialog_border = Color::RGB(82, 86, 94);

    return colors;
}

ThemeColors Theme::Poimandres() {
    ThemeColors colors;
    // Poimandres：紫/青/Teal 深色主题，柔和对比
    colors.background = Color::RGB(27, 27, 38);     // #1b1b26
    colors.foreground = Color::RGB(205, 214, 244);  // #cdd6f4
    colors.current_line = Color::RGB(36, 36, 52);   // #242434
    colors.selection = Color::RGB(54, 58, 79);      // #363a4f
    colors.line_number = Color::RGB(111, 118, 150); // #6f7696
    colors.line_number_current = Color::RGB(205, 214, 244);

    colors.statusbar_bg = Color::RGB(36, 36, 52);
    colors.statusbar_fg = Color::RGB(205, 214, 244);

    colors.menubar_bg = Color::RGB(30, 30, 44);
    colors.menubar_fg = Color::RGB(205, 214, 244);

    colors.helpbar_bg = Color::RGB(36, 36, 52);
    colors.helpbar_fg = Color::RGB(111, 118, 150);
    colors.helpbar_key = Color::RGB(166, 227, 161); // 绿

    colors.keyword = Color::RGB(196, 167, 231);  // 紫
    colors.string = Color::RGB(166, 227, 161);   // 绿
    colors.comment = Color::RGB(111, 118, 150);  // 灰蓝
    colors.number = Color::RGB(249, 226, 175);   // 黄
    colors.function = Color::RGB(137, 180, 250); // 蓝
    colors.type = Color::RGB(249, 226, 175);     // 黄
    colors.operator_color = Color::RGB(137, 180, 250);

    colors.error = Color::RGB(243, 139, 168);
    colors.warning = Color::RGB(249, 226, 175);
    colors.info = Color::RGB(137, 180, 250);
    colors.success = Color::RGB(166, 227, 161);

    colors.dialog_bg = Color::RGB(36, 36, 52);
    colors.dialog_fg = Color::RGB(205, 214, 244);
    colors.dialog_title_bg = Color::RGB(54, 58, 79);
    colors.dialog_title_fg = Color::RGB(205, 214, 244);
    colors.dialog_border = Color::RGB(111, 118, 150);

    return colors;
}

ThemeColors Theme::Terafox() {
    ThemeColors colors;
    // Terafox：暖棕深色，橙/黄/绿点缀
    colors.background = Color::RGB(26, 24, 22);    // #1a1816
    colors.foreground = Color::RGB(222, 214, 202); // #ded6ca
    colors.current_line = Color::RGB(38, 35, 32);  // #262320
    colors.selection = Color::RGB(54, 50, 46);     // #36322e
    colors.line_number = Color::RGB(115, 106, 90); // #736a5a
    colors.line_number_current = Color::RGB(222, 214, 202);

    colors.statusbar_bg = Color::RGB(38, 35, 32);
    colors.statusbar_fg = Color::RGB(222, 214, 202);

    colors.menubar_bg = Color::RGB(30, 28, 26);
    colors.menubar_fg = Color::RGB(222, 214, 202);

    colors.helpbar_bg = Color::RGB(38, 35, 32);
    colors.helpbar_fg = Color::RGB(115, 106, 90);
    colors.helpbar_key = Color::RGB(159, 198, 115); // 绿

    colors.keyword = Color::RGB(230, 155, 95);   // 橙
    colors.string = Color::RGB(159, 198, 115);   // 绿
    colors.comment = Color::RGB(115, 106, 90);   // 灰棕
    colors.number = Color::RGB(230, 155, 95);    // 橙
    colors.function = Color::RGB(159, 198, 115); // 绿
    colors.type = Color::RGB(230, 185, 130);     // 黄橙
    colors.operator_color = Color::RGB(230, 155, 95);

    colors.error = Color::RGB(230, 100, 95);
    colors.warning = Color::RGB(230, 155, 95);
    colors.info = Color::RGB(130, 180, 210);
    colors.success = Color::RGB(159, 198, 115);

    colors.dialog_bg = Color::RGB(38, 35, 32);
    colors.dialog_fg = Color::RGB(222, 214, 202);
    colors.dialog_title_bg = Color::RGB(54, 50, 46);
    colors.dialog_title_fg = Color::RGB(222, 214, 202);
    colors.dialog_border = Color::RGB(115, 106, 90);

    return colors;
}

ThemeColors Theme::Galaxy() {
    ThemeColors colors;
    colors.background = Color::RGB(10, 8, 20);
    colors.foreground = Color::RGB(220, 215, 255);
    colors.current_line = Color::RGB(20, 15, 40);
    colors.selection = Color::RGB(40, 30, 80);
    colors.line_number = Color::RGB(80, 70, 120);
    colors.line_number_current = Color::RGB(220, 215, 255);

    colors.statusbar_bg = Color::RGB(20, 15, 40);
    colors.statusbar_fg = Color::RGB(220, 215, 255);

    colors.menubar_bg = Color::RGB(15, 12, 30);
    colors.menubar_fg = Color::RGB(220, 215, 255);

    colors.helpbar_bg = Color::RGB(20, 15, 40);
    colors.helpbar_fg = Color::RGB(80, 70, 120);
    colors.helpbar_key = Color::RGB(180, 160, 255);

    colors.keyword = Color::RGB(180, 140, 255);
    colors.string = Color::RGB(140, 220, 200);
    colors.comment = Color::RGB(80, 70, 120);
    colors.number = Color::RGB(255, 180, 140);
    colors.function = Color::RGB(140, 200, 255);
    colors.type = Color::RGB(255, 200, 140);
    colors.operator_color = Color::RGB(180, 140, 255);

    colors.error = Color::RGB(255, 120, 140);
    colors.warning = Color::RGB(255, 200, 100);
    colors.info = Color::RGB(100, 200, 255);
    colors.success = Color::RGB(140, 220, 200);

    colors.dialog_bg = Color::RGB(20, 15, 40);
    colors.dialog_fg = Color::RGB(220, 215, 255);
    colors.dialog_title_bg = Color::RGB(40, 30, 80);
    colors.dialog_title_fg = Color::RGB(220, 215, 255);
    colors.dialog_border = Color::RGB(80, 70, 120);

    return colors;
}

ThemeColors Theme::Lightning() {
    ThemeColors colors;
    colors.background = Color::RGB(15, 15, 25);
    colors.foreground = Color::RGB(230, 230, 250);
    colors.current_line = Color::RGB(25, 25, 45);
    colors.selection = Color::RGB(50, 50, 100);
    colors.line_number = Color::RGB(90, 90, 130);
    colors.line_number_current = Color::RGB(230, 230, 250);

    colors.statusbar_bg = Color::RGB(25, 25, 45);
    colors.statusbar_fg = Color::RGB(230, 230, 250);

    colors.menubar_bg = Color::RGB(20, 20, 35);
    colors.menubar_fg = Color::RGB(230, 230, 250);

    colors.helpbar_bg = Color::RGB(25, 25, 45);
    colors.helpbar_fg = Color::RGB(90, 90, 130);
    colors.helpbar_key = Color::RGB(255, 255, 120);

    colors.keyword = Color::RGB(200, 180, 255);
    colors.string = Color::RGB(120, 255, 180);
    colors.comment = Color::RGB(90, 90, 130);
    colors.number = Color::RGB(255, 220, 100);
    colors.function = Color::RGB(100, 200, 255);
    colors.type = Color::RGB(255, 200, 100);
    colors.operator_color = Color::RGB(200, 180, 255);

    colors.error = Color::RGB(255, 120, 120);
    colors.warning = Color::RGB(255, 220, 100);
    colors.info = Color::RGB(100, 200, 255);
    colors.success = Color::RGB(120, 255, 180);

    colors.dialog_bg = Color::RGB(25, 25, 45);
    colors.dialog_fg = Color::RGB(230, 230, 250);
    colors.dialog_title_bg = Color::RGB(50, 50, 100);
    colors.dialog_title_fg = Color::RGB(255, 255, 200);
    colors.dialog_border = Color::RGB(90, 90, 130);

    return colors;
}

ThemeColors Theme::Storm() {
    ThemeColors colors;
    colors.background = Color::RGB(20, 22, 28);
    colors.foreground = Color::RGB(200, 205, 215);
    colors.current_line = Color::RGB(30, 33, 42);
    colors.selection = Color::RGB(45, 55, 70);
    colors.line_number = Color::RGB(80, 90, 110);
    colors.line_number_current = Color::RGB(200, 205, 215);

    colors.statusbar_bg = Color::RGB(30, 33, 42);
    colors.statusbar_fg = Color::RGB(200, 205, 215);

    colors.menubar_bg = Color::RGB(25, 28, 35);
    colors.menubar_fg = Color::RGB(200, 205, 215);

    colors.helpbar_bg = Color::RGB(30, 33, 42);
    colors.helpbar_fg = Color::RGB(80, 90, 110);
    colors.helpbar_key = Color::RGB(100, 180, 220);

    colors.keyword = Color::RGB(140, 170, 220);
    colors.string = Color::RGB(160, 210, 170);
    colors.comment = Color::RGB(80, 90, 110);
    colors.number = Color::RGB(220, 180, 140);
    colors.function = Color::RGB(130, 190, 220);
    colors.type = Color::RGB(220, 190, 150);
    colors.operator_color = Color::RGB(140, 170, 220);

    colors.error = Color::RGB(220, 130, 130);
    colors.warning = Color::RGB(220, 190, 100);
    colors.info = Color::RGB(100, 180, 220);
    colors.success = Color::RGB(150, 210, 160);

    colors.dialog_bg = Color::RGB(30, 33, 42);
    colors.dialog_fg = Color::RGB(200, 205, 215);
    colors.dialog_title_bg = Color::RGB(45, 55, 70);
    colors.dialog_title_fg = Color::RGB(200, 210, 230);
    colors.dialog_border = Color::RGB(80, 90, 110);

    return colors;
}

ThemeColors Theme::Mellow() {
    ThemeColors colors;
    // Mellow：柔和 pastel 深色，低对比
    colors.background = Color::RGB(35, 38, 42);     // #23262a
    colors.foreground = Color::RGB(224, 222, 216);  // #e0ded8
    colors.current_line = Color::RGB(42, 46, 52);   // #2a2e34
    colors.selection = Color::RGB(55, 60, 68);      // #373c44
    colors.line_number = Color::RGB(120, 125, 135); // #787d87
    colors.line_number_current = Color::RGB(224, 222, 216);

    colors.statusbar_bg = Color::RGB(42, 46, 52);
    colors.statusbar_fg = Color::RGB(224, 222, 216);

    colors.menubar_bg = Color::RGB(38, 42, 48);
    colors.menubar_fg = Color::RGB(224, 222, 216);

    colors.helpbar_bg = Color::RGB(42, 46, 52);
    colors.helpbar_fg = Color::RGB(120, 125, 135);
    colors.helpbar_key = Color::RGB(180, 210, 180); // 浅绿

    colors.keyword = Color::RGB(220, 170, 200);  // 粉
    colors.string = Color::RGB(180, 210, 180);   // 浅绿
    colors.comment = Color::RGB(120, 125, 135);  // 灰
    colors.number = Color::RGB(230, 200, 160);   // 米黄
    colors.function = Color::RGB(180, 210, 180); // 浅绿
    colors.type = Color::RGB(170, 200, 230);     // 浅蓝
    colors.operator_color = Color::RGB(220, 170, 200);

    colors.error = Color::RGB(230, 160, 160);
    colors.warning = Color::RGB(230, 200, 160);
    colors.info = Color::RGB(170, 200, 230);
    colors.success = Color::RGB(180, 210, 180);

    colors.dialog_bg = Color::RGB(42, 46, 52);
    colors.dialog_fg = Color::RGB(224, 222, 216);
    colors.dialog_title_bg = Color::RGB(55, 60, 68);
    colors.dialog_title_fg = Color::RGB(224, 222, 216);
    colors.dialog_border = Color::RGB(120, 125, 135);

    return colors;
}

ThemeColors Theme::Fleet() {
    ThemeColors colors;
    // Fleet：JetBrains Fleet 风格，简洁深灰蓝
    colors.background = Color::RGB(30, 32, 36);     // #1e2024
    colors.foreground = Color::RGB(200, 205, 212);  // #c8cdd4
    colors.current_line = Color::RGB(38, 41, 46);   // #26292e
    colors.selection = Color::RGB(50, 54, 60);      // #32363c
    colors.line_number = Color::RGB(100, 108, 120); // #646c78
    colors.line_number_current = Color::RGB(200, 205, 212);

    colors.statusbar_bg = Color::RGB(38, 41, 46);
    colors.statusbar_fg = Color::RGB(200, 205, 212);

    colors.menubar_bg = Color::RGB(34, 36, 41);
    colors.menubar_fg = Color::RGB(200, 205, 212);

    colors.helpbar_bg = Color::RGB(38, 41, 46);
    colors.helpbar_fg = Color::RGB(100, 108, 120);
    colors.helpbar_key = Color::RGB(129, 161, 193); // 蓝灰

    colors.keyword = Color::RGB(204, 120, 170);  // 粉紫
    colors.string = Color::RGB(152, 195, 121);   // 绿
    colors.comment = Color::RGB(100, 108, 120);  // 灰
    colors.number = Color::RGB(184, 215, 163);   // 浅绿
    colors.function = Color::RGB(129, 161, 193); // 蓝
    colors.type = Color::RGB(230, 180, 130);     // 橙
    colors.operator_color = Color::RGB(200, 205, 212);

    colors.error = Color::RGB(230, 120, 120);
    colors.warning = Color::RGB(230, 180, 130);
    colors.info = Color::RGB(129, 161, 193);
    colors.success = Color::RGB(152, 195, 121);

    colors.dialog_bg = Color::RGB(38, 41, 46);
    colors.dialog_fg = Color::RGB(200, 205, 212);
    colors.dialog_title_bg = Color::RGB(50, 54, 60);
    colors.dialog_title_fg = Color::RGB(200, 205, 212);
    colors.dialog_border = Color::RGB(100, 108, 120);

    return colors;
}

ThemeColors Theme::Luna() {
    ThemeColors colors;
    // Luna：柔和紫蓝深色，低饱和
    colors.background = Color::RGB(28, 28, 38);    // #1c1c26
    colors.foreground = Color::RGB(220, 218, 235); // #dcdaeb
    colors.current_line = Color::RGB(38, 38, 52);  // #262634
    colors.selection = Color::RGB(52, 50, 72);     // #343248
    colors.line_number = Color::RGB(100, 98, 120); // #646278
    colors.line_number_current = Color::RGB(220, 218, 235);

    colors.statusbar_bg = Color::RGB(38, 38, 52);
    colors.statusbar_fg = Color::RGB(220, 218, 235);

    colors.menubar_bg = Color::RGB(32, 32, 44);
    colors.menubar_fg = Color::RGB(220, 218, 235);

    colors.helpbar_bg = Color::RGB(38, 38, 52);
    colors.helpbar_fg = Color::RGB(100, 98, 120);
    colors.helpbar_key = Color::RGB(180, 165, 220); // 淡紫

    colors.keyword = Color::RGB(195, 155, 220);  // 紫
    colors.string = Color::RGB(165, 215, 180);   // 薄荷绿
    colors.comment = Color::RGB(100, 98, 120);   // 灰紫
    colors.number = Color::RGB(240, 195, 140);   // 杏黄
    colors.function = Color::RGB(130, 185, 220); // 天蓝
    colors.type = Color::RGB(195, 155, 220);     // 紫
    colors.operator_color = Color::RGB(220, 218, 235);

    colors.error = Color::RGB(230, 150, 160);
    colors.warning = Color::RGB(240, 195, 140);
    colors.info = Color::RGB(130, 185, 220);
    colors.success = Color::RGB(165, 215, 180);

    colors.dialog_bg = Color::RGB(38, 38, 52);
    colors.dialog_fg = Color::RGB(220, 218, 235);
    colors.dialog_title_bg = Color::RGB(52, 50, 72);
    colors.dialog_title_fg = Color::RGB(220, 218, 235);
    colors.dialog_border = Color::RGB(100, 98, 120);

    return colors;
}

ThemeColors Theme::Retro() {
    ThemeColors colors;
    // Retro：复古 CRT 终端风，琥珀色磷光 on 纯黑（老式显示器/VT100 感）
    colors.background = Color::RGB(0, 0, 0);      // #000000 纯黑
    colors.foreground = Color::RGB(255, 191, 0);  // #ffbf00 琥珀
    colors.current_line = Color::RGB(15, 15, 0);  // 极暗琥珀底
    colors.selection = Color::RGB(40, 30, 0);     // 深琥珀
    colors.line_number = Color::RGB(180, 140, 0); // 暗琥珀
    colors.line_number_current = Color::RGB(255, 191, 0);

    colors.statusbar_bg = Color::RGB(20, 15, 0);
    colors.statusbar_fg = Color::RGB(255, 191, 0);

    colors.menubar_bg = Color::RGB(10, 10, 0);
    colors.menubar_fg = Color::RGB(255, 191, 0);

    colors.helpbar_bg = Color::RGB(20, 15, 0);
    colors.helpbar_fg = Color::RGB(180, 140, 0);
    colors.helpbar_key = Color::RGB(255, 220, 100); // 亮琥珀

    colors.keyword = Color::RGB(255, 220, 100);  // 亮琥珀
    colors.string = Color::RGB(255, 230, 150);   // 浅琥珀
    colors.comment = Color::RGB(140, 110, 0);    // 暗琥珀
    colors.number = Color::RGB(255, 200, 50);    // 金琥珀
    colors.function = Color::RGB(255, 230, 150); // 浅琥珀
    colors.type = Color::RGB(255, 210, 80);      // 中琥珀
    colors.operator_color = Color::RGB(255, 191, 0);

    colors.error = Color::RGB(255, 100, 80); // 琥珀红
    colors.warning = Color::RGB(255, 200, 50);
    colors.info = Color::RGB(255, 220, 100);
    colors.success = Color::RGB(200, 255, 150); // 淡绿（仅作区分）

    colors.dialog_bg = Color::RGB(15, 12, 0);
    colors.dialog_fg = Color::RGB(255, 191, 0);
    colors.dialog_title_bg = Color::RGB(40, 30, 0);
    colors.dialog_title_fg = Color::RGB(255, 191, 0);
    colors.dialog_border = Color::RGB(180, 140, 0);

    return colors;
}

ThemeColors Theme::Sunset() {
    ThemeColors colors;
    // Sunset：日落/黄昏风，暖橙红主色，金色小时感
    colors.background = Color::RGB(40, 28, 32);     // #281c20 暖深紫褐
    colors.foreground = Color::RGB(255, 235, 215);  // #ffebdc 杏白
    colors.current_line = Color::RGB(55, 38, 42);   // 略亮
    colors.selection = Color::RGB(75, 50, 55);      // 暖紫褐
    colors.line_number = Color::RGB(160, 110, 100); // 暖灰
    colors.line_number_current = Color::RGB(255, 200, 150);

    colors.statusbar_bg = Color::RGB(55, 38, 42);
    colors.statusbar_fg = Color::RGB(255, 235, 215);

    colors.menubar_bg = Color::RGB(45, 32, 36);
    colors.menubar_fg = Color::RGB(255, 235, 215);

    colors.helpbar_bg = Color::RGB(55, 38, 42);
    colors.helpbar_fg = Color::RGB(160, 110, 100);
    colors.helpbar_key = Color::RGB(255, 180, 130); // 日落橙

    colors.keyword = Color::RGB(255, 140, 120);  // 珊瑚红
    colors.string = Color::RGB(255, 215, 170);   // 桃杏
    colors.comment = Color::RGB(140, 100, 95);   // 暖灰
    colors.number = Color::RGB(255, 180, 100);   // 橙
    colors.function = Color::RGB(255, 200, 150); // 浅橙
    colors.type = Color::RGB(255, 160, 130);     // 暖红
    colors.operator_color = Color::RGB(255, 140, 120);

    colors.error = Color::RGB(255, 100, 90);
    colors.warning = Color::RGB(255, 180, 100);
    colors.info = Color::RGB(200, 180, 255);    // 淡紫（点缀）
    colors.success = Color::RGB(180, 255, 180); // 淡绿（点缀）

    colors.dialog_bg = Color::RGB(50, 35, 40);
    colors.dialog_fg = Color::RGB(255, 235, 215);
    colors.dialog_title_bg = Color::RGB(75, 50, 55);
    colors.dialog_title_fg = Color::RGB(255, 200, 150);
    colors.dialog_border = Color::RGB(180, 120, 110);

    return colors;
}

ThemeColors Theme::Forest() {
    ThemeColors colors;
    // Forest：森林绿深色，自然绿/苔藓/暖白
    colors.background = Color::RGB(22, 28, 24);    // #161c18
    colors.foreground = Color::RGB(220, 228, 218); // #dce4da
    colors.current_line = Color::RGB(30, 38, 32);  // #1e2620
    colors.selection = Color::RGB(42, 54, 46);     // #2a362e
    colors.line_number = Color::RGB(90, 115, 95);  // #5a735f
    colors.line_number_current = Color::RGB(180, 210, 175);

    colors.statusbar_bg = Color::RGB(30, 38, 32);
    colors.statusbar_fg = Color::RGB(220, 228, 218);

    colors.menubar_bg = Color::RGB(26, 32, 28);
    colors.menubar_fg = Color::RGB(220, 228, 218);

    colors.helpbar_bg = Color::RGB(30, 38, 32);
    colors.helpbar_fg = Color::RGB(90, 115, 95);
    colors.helpbar_key = Color::RGB(150, 200, 140); // 嫩绿

    colors.keyword = Color::RGB(120, 180, 130);  // 叶绿
    colors.string = Color::RGB(180, 210, 160);   // 浅绿
    colors.comment = Color::RGB(90, 115, 95);    // 灰绿
    colors.number = Color::RGB(200, 180, 130);   // 暖黄
    colors.function = Color::RGB(150, 200, 140); // 嫩绿
    colors.type = Color::RGB(130, 190, 150);     // 青绿
    colors.operator_color = Color::RGB(160, 195, 145);

    colors.error = Color::RGB(220, 120, 110);
    colors.warning = Color::RGB(200, 180, 100);
    colors.info = Color::RGB(130, 190, 150);
    colors.success = Color::RGB(150, 200, 140);

    colors.dialog_bg = Color::RGB(30, 38, 32);
    colors.dialog_fg = Color::RGB(220, 228, 218);
    colors.dialog_title_bg = Color::RGB(42, 54, 46);
    colors.dialog_title_fg = Color::RGB(180, 210, 175);
    colors.dialog_border = Color::RGB(90, 115, 95);

    return colors;
}

ThemeColors Theme::Ocean() {
    ThemeColors colors;
    // Ocean：深海蓝青，冷静蓝/青/白
    colors.background = Color::RGB(18, 24, 32);    // #121820
    colors.foreground = Color::RGB(210, 222, 235); // #d2deeb
    colors.current_line = Color::RGB(26, 34, 46);  // #1a222e
    colors.selection = Color::RGB(38, 50, 68);     // #263244
    colors.line_number = Color::RGB(80, 105, 130); // #506982
    colors.line_number_current = Color::RGB(170, 200, 230);

    colors.statusbar_bg = Color::RGB(26, 34, 46);
    colors.statusbar_fg = Color::RGB(210, 222, 235);

    colors.menubar_bg = Color::RGB(22, 28, 38);
    colors.menubar_fg = Color::RGB(210, 222, 235);

    colors.helpbar_bg = Color::RGB(26, 34, 46);
    colors.helpbar_fg = Color::RGB(80, 105, 130);
    colors.helpbar_key = Color::RGB(120, 190, 220); // 天青

    colors.keyword = Color::RGB(120, 190, 220);  // 天青
    colors.string = Color::RGB(150, 220, 200);   // 浅青
    colors.comment = Color::RGB(80, 105, 130);   // 灰蓝
    colors.number = Color::RGB(200, 180, 230);   // 淡紫
    colors.function = Color::RGB(150, 220, 200); // 青绿
    colors.type = Color::RGB(170, 210, 240);     // 浅蓝
    colors.operator_color = Color::RGB(120, 190, 220);

    colors.error = Color::RGB(230, 130, 140);
    colors.warning = Color::RGB(230, 200, 130);
    colors.info = Color::RGB(120, 190, 220);
    colors.success = Color::RGB(150, 220, 200);

    colors.dialog_bg = Color::RGB(26, 34, 46);
    colors.dialog_fg = Color::RGB(210, 222, 235);
    colors.dialog_title_bg = Color::RGB(38, 50, 68);
    colors.dialog_title_fg = Color::RGB(170, 200, 230);
    colors.dialog_border = Color::RGB(80, 105, 130);

    return colors;
}

ThemeColors Theme::TangoDark() {
    ThemeColors colors;
    // Tango Dark: GNOME/Ubuntu 经典 Tango 调色板深色，暖橙/蓝/绿
    colors.background = Color::RGB(32, 32, 32);     // #202020
    colors.foreground = Color::RGB(238, 238, 236);  // #eeeeec
    colors.current_line = Color::RGB(46, 52, 54);   // #2e3436
    colors.selection = Color::RGB(52, 58, 60);      // #343a3c
    colors.line_number = Color::RGB(136, 138, 133); // #888a85
    colors.line_number_current = Color::RGB(238, 238, 236);

    colors.statusbar_bg = Color::RGB(46, 52, 54);
    colors.statusbar_fg = Color::RGB(238, 238, 236);

    colors.menubar_bg = Color::RGB(32, 32, 32);
    colors.menubar_fg = Color::RGB(238, 238, 236);

    colors.helpbar_bg = Color::RGB(46, 52, 54);
    colors.helpbar_fg = Color::RGB(136, 138, 133);
    colors.helpbar_key = Color::RGB(138, 226, 52); // Tango Chameleon

    colors.keyword = Color::RGB(239, 41, 41);   // Tango Scarlet Red
    colors.string = Color::RGB(252, 175, 62);   // Tango Butter
    colors.comment = Color::RGB(136, 138, 133); // Tango Aluminium
    colors.number = Color::RGB(252, 175, 62);   // Tango Butter
    colors.function = Color::RGB(138, 226, 52); // Tango Chameleon
    colors.type = Color::RGB(114, 159, 207);    // Tango Sky Blue
    colors.operator_color = Color::RGB(239, 41, 41);

    colors.error = Color::RGB(239, 41, 41);
    colors.warning = Color::RGB(252, 175, 62);
    colors.info = Color::RGB(114, 159, 207);
    colors.success = Color::RGB(138, 226, 52);

    colors.dialog_bg = Color::RGB(46, 52, 54);
    colors.dialog_fg = Color::RGB(238, 238, 236);
    colors.dialog_title_bg = Color::RGB(32, 32, 32);
    colors.dialog_title_fg = Color::RGB(238, 238, 236);
    colors.dialog_border = Color::RGB(136, 138, 133);

    return colors;
}

ThemeColors Theme::Synthwave() {
    ThemeColors colors;
    // Synthwave / Outrun: 80 年代霓虹紫青粉，赛博落日风
    colors.background = Color::RGB(36, 27, 52);             // #241b34 深紫
    colors.foreground = Color::RGB(255, 230, 255);          // #ffe6ff 浅粉白
    colors.current_line = Color::RGB(52, 38, 72);           // #342648
    colors.selection = Color::RGB(72, 52, 98);              // #483462
    colors.line_number = Color::RGB(140, 100, 180);         // #8c64b4
    colors.line_number_current = Color::RGB(255, 120, 255); // 霓虹粉

    colors.statusbar_bg = Color::RGB(48, 35, 68);
    colors.statusbar_fg = Color::RGB(255, 230, 255);

    colors.menubar_bg = Color::RGB(36, 27, 52);
    colors.menubar_fg = Color::RGB(255, 230, 255);

    colors.helpbar_bg = Color::RGB(48, 35, 68);
    colors.helpbar_fg = Color::RGB(180, 140, 220);
    colors.helpbar_key = Color::RGB(0, 255, 255); // 霓虹青

    colors.keyword = Color::RGB(255, 100, 255); // 霓虹粉/品红
    colors.string = Color::RGB(255, 200, 100);  // 暖黄
    colors.comment = Color::RGB(140, 100, 180);
    colors.number = Color::RGB(0, 255, 255);     // 霓虹青
    colors.function = Color::RGB(100, 220, 255); // 亮青
    colors.type = Color::RGB(200, 120, 255);     // 紫
    colors.operator_color = Color::RGB(255, 100, 255);

    colors.error = Color::RGB(255, 80, 120);
    colors.warning = Color::RGB(255, 200, 100);
    colors.info = Color::RGB(0, 255, 255);
    colors.success = Color::RGB(100, 255, 180); // 霓虹绿

    colors.dialog_bg = Color::RGB(52, 38, 72);
    colors.dialog_fg = Color::RGB(255, 230, 255);
    colors.dialog_title_bg = Color::RGB(48, 35, 68);
    colors.dialog_title_fg = Color::RGB(255, 230, 255);
    colors.dialog_border = Color::RGB(140, 100, 180);

    return colors;
}

ThemeColors Theme::RetroFuture() {
    ThemeColors colors;
    // Retro-Future: 复古未来主义，80 年代 CRT 显示器荧光风格
    // 灵感来自早期科幻电影和老式计算机终端
    colors.background = Color::RGB(10, 15, 20);           // 深蓝黑背景
    colors.foreground = Color::RGB(0, 255, 128);          // 经典磷光绿
    colors.current_line = Color::RGB(15, 25, 35);         // 稍亮的背景
    colors.selection = Color::RGB(20, 35, 50);            // 选中区域
    colors.line_number = Color::RGB(0, 180, 100);         // 暗绿色行号
    colors.line_number_current = Color::RGB(0, 255, 150); // 亮绿当前行

    colors.statusbar_bg = Color::RGB(15, 25, 35);
    colors.statusbar_fg = Color::RGB(0, 255, 128);

    colors.menubar_bg = Color::RGB(10, 15, 20);
    colors.menubar_fg = Color::RGB(0, 255, 128);

    colors.helpbar_bg = Color::RGB(15, 25, 35);
    colors.helpbar_fg = Color::RGB(0, 220, 120);
    colors.helpbar_key = Color::RGB(255, 0, 255); // 品红快捷键

    colors.keyword = Color::RGB(255, 0, 255);        // 品红关键词
    colors.string = Color::RGB(255, 200, 0);         // 琥珀色字符串
    colors.comment = Color::RGB(0, 150, 100);        // 暗绿注释
    colors.number = Color::RGB(0, 255, 255);         // 青色数字
    colors.function = Color::RGB(255, 100, 0);       // 橙色函数
    colors.type = Color::RGB(0, 200, 255);           // 天蓝色类型
    colors.operator_color = Color::RGB(255, 0, 255); // 品红操作符

    colors.error = Color::RGB(255, 50, 50);   // 红色错误
    colors.warning = Color::RGB(255, 200, 0); // 琥珀色警告
    colors.info = Color::RGB(0, 200, 255);    // 青色信息
    colors.success = Color::RGB(0, 255, 128); // 磷光绿成功

    colors.dialog_bg = Color::RGB(15, 25, 35);
    colors.dialog_fg = Color::RGB(0, 255, 128);
    colors.dialog_title_bg = Color::RGB(20, 35, 50);
    colors.dialog_title_fg = Color::RGB(255, 0, 255);
    colors.dialog_border = Color::RGB(0, 200, 255);

    return colors;
}

ThemeColors Theme::Decay() {
    ThemeColors colors;
    // Decay: 冷灰蓝低饱和，柔和护眼
    colors.background = Color::RGB(30, 33, 39);    // #1e2127
    colors.foreground = Color::RGB(171, 178, 191); // #abb2bf
    colors.current_line = Color::RGB(40, 44, 52);  // #282c34
    colors.selection = Color::RGB(55, 60, 70);     // #373c46
    colors.line_number = Color::RGB(92, 99, 112);  // #5c6370
    colors.line_number_current = Color::RGB(171, 178, 191);

    colors.statusbar_bg = Color::RGB(40, 44, 52);
    colors.statusbar_fg = Color::RGB(171, 178, 191);

    colors.menubar_bg = Color::RGB(30, 33, 39);
    colors.menubar_fg = Color::RGB(171, 178, 191);

    colors.helpbar_bg = Color::RGB(40, 44, 52);
    colors.helpbar_fg = Color::RGB(92, 99, 112);
    colors.helpbar_key = Color::RGB(152, 195, 121); // 柔和绿

    colors.keyword = Color::RGB(198, 120, 221); // 淡紫
    colors.string = Color::RGB(230, 192, 123);  // 淡黄
    colors.comment = Color::RGB(92, 99, 112);
    colors.number = Color::RGB(209, 154, 102);  // 淡橙
    colors.function = Color::RGB(97, 175, 239); // 淡蓝
    colors.type = Color::RGB(86, 182, 194);     // 青
    colors.operator_color = Color::RGB(198, 120, 221);

    colors.error = Color::RGB(224, 108, 117);
    colors.warning = Color::RGB(209, 154, 102);
    colors.info = Color::RGB(97, 175, 239);
    colors.success = Color::RGB(152, 195, 121);

    colors.dialog_bg = Color::RGB(40, 44, 52);
    colors.dialog_fg = Color::RGB(171, 178, 191);
    colors.dialog_title_bg = Color::RGB(30, 33, 39);
    colors.dialog_title_fg = Color::RGB(171, 178, 191);
    colors.dialog_border = Color::RGB(92, 99, 112);

    return colors;
}

ThemeColors Theme::RiderDark() {
    ThemeColors colors;
    // Rider Dark: JetBrains Rider 风格，灰底紫蓝
    colors.background = Color::RGB(43, 43, 43);     // #2b2b2b
    colors.foreground = Color::RGB(169, 183, 198);  // #a9b7c6
    colors.current_line = Color::RGB(52, 52, 52);   // #343434
    colors.selection = Color::RGB(62, 68, 82);      // #3e4452
    colors.line_number = Color::RGB(100, 110, 120); // #646e78
    colors.line_number_current = Color::RGB(169, 183, 198);

    colors.statusbar_bg = Color::RGB(52, 52, 52);
    colors.statusbar_fg = Color::RGB(169, 183, 198);

    colors.menubar_bg = Color::RGB(43, 43, 43);
    colors.menubar_fg = Color::RGB(169, 183, 198);

    colors.helpbar_bg = Color::RGB(52, 52, 52);
    colors.helpbar_fg = Color::RGB(100, 110, 120);
    colors.helpbar_key = Color::RGB(165, 194, 97); // 绿

    colors.keyword = Color::RGB(204, 120, 204); // 紫
    colors.string = Color::RGB(104, 151, 187);  // 蓝
    colors.comment = Color::RGB(100, 110, 120);
    colors.number = Color::RGB(104, 151, 187);
    colors.function = Color::RGB(255, 198, 109); // 橙黄
    colors.type = Color::RGB(165, 194, 97);      // 绿
    colors.operator_color = Color::RGB(169, 183, 198);

    colors.error = Color::RGB(220, 100, 100);
    colors.warning = Color::RGB(255, 198, 109);
    colors.info = Color::RGB(104, 151, 187);
    colors.success = Color::RGB(165, 194, 97);

    colors.dialog_bg = Color::RGB(52, 52, 52);
    colors.dialog_fg = Color::RGB(169, 183, 198);
    colors.dialog_title_bg = Color::RGB(43, 43, 43);
    colors.dialog_title_fg = Color::RGB(169, 183, 198);
    colors.dialog_border = Color::RGB(80, 80, 80);

    return colors;
}

ThemeColors Theme::ParchmentDark() {
    ThemeColors colors;
    // Parchment Dark: 深褐/羊皮纸风，暖棕底奶油字，琥珀/橄榄点缀（与 Terafox/Desert
    // 区分：更偏古纸褐）
    colors.background = Color::RGB(42, 38, 34);    // #2a2622 深褐
    colors.foreground = Color::RGB(232, 228, 220); // #e8e4dc 奶油
    colors.current_line = Color::RGB(52, 46, 40);  // #342e28
    colors.selection = Color::RGB(62, 55, 48);     // #3e3730
    colors.line_number = Color::RGB(120, 108, 95); // #786c5f
    colors.line_number_current = Color::RGB(232, 228, 220);

    colors.statusbar_bg = Color::RGB(52, 46, 40);
    colors.statusbar_fg = Color::RGB(232, 228, 220);

    colors.menubar_bg = Color::RGB(42, 38, 34);
    colors.menubar_fg = Color::RGB(232, 228, 220);

    colors.helpbar_bg = Color::RGB(52, 46, 40);
    colors.helpbar_fg = Color::RGB(120, 108, 95);
    colors.helpbar_key = Color::RGB(180, 165, 100); // 橄榄金

    colors.keyword = Color::RGB(180, 130, 70); // 琥珀棕
    colors.string = Color::RGB(140, 160, 110); // 橄榄绿
    colors.comment = Color::RGB(120, 108, 95);
    colors.number = Color::RGB(210, 170, 100);   // 金琥珀
    colors.function = Color::RGB(160, 145, 100); // 暗金
    colors.type = Color::RGB(190, 155, 90);      // 琥珀
    colors.operator_color = Color::RGB(180, 130, 70);

    colors.error = Color::RGB(200, 90, 70);
    colors.warning = Color::RGB(210, 170, 100);
    colors.info = Color::RGB(140, 160, 130);
    colors.success = Color::RGB(140, 160, 110);

    colors.dialog_bg = Color::RGB(52, 46, 40);
    colors.dialog_fg = Color::RGB(232, 228, 220);
    colors.dialog_title_bg = Color::RGB(42, 38, 34);
    colors.dialog_title_fg = Color::RGB(232, 228, 220);
    colors.dialog_border = Color::RGB(100, 90, 78);

    return colors;
}

ThemeColors Theme::Crimson() {
    ThemeColors colors;
    // Crimson: 深红黑底，绯红主色，与现有紫/蓝/橙系均不同
    colors.background = Color::RGB(28, 22, 24);    // #1c1618 深红黑
    colors.foreground = Color::RGB(224, 220, 218); // #e0dcda
    colors.current_line = Color::RGB(40, 32, 34);  // #282022
    colors.selection = Color::RGB(58, 45, 48);     // #3a2d30
    colors.line_number = Color::RGB(100, 80, 85);  // #645055
    colors.line_number_current = Color::RGB(224, 220, 218);

    colors.statusbar_bg = Color::RGB(40, 32, 34);
    colors.statusbar_fg = Color::RGB(224, 220, 218);

    colors.menubar_bg = Color::RGB(28, 22, 24);
    colors.menubar_fg = Color::RGB(224, 220, 218);

    colors.helpbar_bg = Color::RGB(40, 32, 34);
    colors.helpbar_fg = Color::RGB(100, 80, 85);
    colors.helpbar_key = Color::RGB(201, 60, 75); // 绯红

    colors.keyword = Color::RGB(201, 60, 75);  // 绯红 #c93c4b
    colors.string = Color::RGB(218, 180, 120); // 暖金
    colors.comment = Color::RGB(100, 80, 85);
    colors.number = Color::RGB(220, 140, 130);   // 浅红
    colors.function = Color::RGB(180, 100, 110); // 玫瑰
    colors.type = Color::RGB(200, 120, 100);     // 砖红
    colors.operator_color = Color::RGB(201, 60, 75);

    colors.error = Color::RGB(220, 80, 90);
    colors.warning = Color::RGB(218, 180, 120);
    colors.info = Color::RGB(160, 130, 150);    // 淡紫灰
    colors.success = Color::RGB(140, 160, 120); // 暗绿

    colors.dialog_bg = Color::RGB(40, 32, 34);
    colors.dialog_fg = Color::RGB(224, 220, 218);
    colors.dialog_title_bg = Color::RGB(28, 22, 24);
    colors.dialog_title_fg = Color::RGB(224, 220, 218);
    colors.dialog_border = Color::RGB(90, 70, 75);

    return colors;
}

ThemeColors Theme::Frost() {
    ThemeColors colors;
    // Frost: 冷冽冰蓝深色，深蓝灰底+冰蓝点缀，与 Nord/Ocean 区分（更偏冰白感）
    colors.background = Color::RGB(30, 35, 46);     // #1e232e 深蓝灰
    colors.foreground = Color::RGB(226, 230, 239);  // #e2e6ef 冰灰白
    colors.current_line = Color::RGB(38, 44, 56);   // #262c38
    colors.selection = Color::RGB(50, 58, 74);      // #323a4a
    colors.line_number = Color::RGB(100, 115, 140); // #64738c
    colors.line_number_current = Color::RGB(226, 230, 239);

    colors.statusbar_bg = Color::RGB(38, 44, 56);
    colors.statusbar_fg = Color::RGB(226, 230, 239);

    colors.menubar_bg = Color::RGB(30, 35, 46);
    colors.menubar_fg = Color::RGB(226, 230, 239);

    colors.helpbar_bg = Color::RGB(38, 44, 56);
    colors.helpbar_fg = Color::RGB(100, 115, 140);
    colors.helpbar_key = Color::RGB(126, 184, 218); // 冰蓝 #7eb8da

    colors.keyword = Color::RGB(126, 184, 218); // 冰蓝
    colors.string = Color::RGB(160, 210, 180);  // 冰薄荷
    colors.comment = Color::RGB(100, 115, 140);
    colors.number = Color::RGB(220, 190, 150);   // 淡琥珀
    colors.function = Color::RGB(156, 196, 224); // 浅冰蓝 #9cc4e0
    colors.type = Color::RGB(180, 160, 220);     // 淡紫
    colors.operator_color = Color::RGB(126, 184, 218);

    colors.error = Color::RGB(220, 120, 130);
    colors.warning = Color::RGB(220, 190, 150);
    colors.info = Color::RGB(126, 184, 218);
    colors.success = Color::RGB(160, 210, 180);

    colors.dialog_bg = Color::RGB(38, 44, 56);
    colors.dialog_fg = Color::RGB(226, 230, 239);
    colors.dialog_title_bg = Color::RGB(30, 35, 46);
    colors.dialog_title_fg = Color::RGB(226, 230, 239);
    colors.dialog_border = Color::RGB(100, 115, 140);

    return colors;
}

ThemeColors Theme::Lavender() {
    ThemeColors colors;
    // Lavender: 薰衣草浅色，浅紫灰底+紫/罗兰点缀，补足浅色紫系
    colors.background = Color::RGB(245, 243, 248);   // #f5f3f8 浅薰衣草
    colors.foreground = Color::RGB(74, 68, 88);      // #4a4458 深紫灰
    colors.current_line = Color::RGB(238, 235, 245); // #eeebf5
    colors.selection = Color::RGB(228, 222, 238);    // #e4deee
    colors.line_number = Color::RGB(160, 152, 180);  // #a098b4
    colors.line_number_current = Color::RGB(74, 68, 88);

    colors.statusbar_bg = Color::RGB(235, 232, 242);
    colors.statusbar_fg = Color::RGB(74, 68, 88);

    colors.menubar_bg = Color::RGB(245, 243, 248);
    colors.menubar_fg = Color::RGB(74, 68, 88);

    colors.helpbar_bg = Color::RGB(235, 232, 242);
    colors.helpbar_fg = Color::RGB(160, 152, 180);
    colors.helpbar_key = Color::RGB(120, 95, 160); // 罗兰 #785fa0

    colors.keyword = Color::RGB(139, 122, 168); // 紫 #8b7aa8
    colors.string = Color::RGB(100, 130, 100);  // 暗绿
    colors.comment = Color::RGB(160, 152, 180);
    colors.number = Color::RGB(160, 100, 140);  // 紫红
    colors.function = Color::RGB(95, 120, 165); // 灰蓝
    colors.type = Color::RGB(120, 95, 160);     // 罗兰
    colors.operator_color = Color::RGB(139, 122, 168);

    colors.error = Color::RGB(180, 80, 90);
    colors.warning = Color::RGB(180, 140, 80);
    colors.info = Color::RGB(95, 120, 165);
    colors.success = Color::RGB(100, 140, 100);

    colors.dialog_bg = Color::RGB(238, 235, 245);
    colors.dialog_fg = Color::RGB(74, 68, 88);
    colors.dialog_title_bg = Color::RGB(235, 232, 242);
    colors.dialog_title_fg = Color::RGB(74, 68, 88);
    colors.dialog_border = Color::RGB(200, 192, 212);

    return colors;
}

ThemeColors Theme::Matcha() {
    ThemeColors colors;
    // Matcha: 日式抹茶主题，深抹茶绿底+米色字，暖绿/棕点缀，柔和护眼
    colors.background = Color::RGB(38, 48, 42);             // #26302a 深抹茶
    colors.foreground = Color::RGB(220, 218, 205);          // #dcdacd 米色
    colors.current_line = Color::RGB(48, 58, 50);           // #303a32
    colors.selection = Color::RGB(58, 72, 60);              // #3a483c
    colors.line_number = Color::RGB(100, 120, 105);         // #647869
    colors.line_number_current = Color::RGB(180, 200, 160); // 抹茶亮绿

    colors.statusbar_bg = Color::RGB(42, 54, 46);
    colors.statusbar_fg = Color::RGB(220, 218, 205);

    colors.menubar_bg = Color::RGB(32, 42, 36);
    colors.menubar_fg = Color::RGB(220, 218, 205);

    colors.helpbar_bg = Color::RGB(42, 54, 46);
    colors.helpbar_fg = Color::RGB(100, 120, 105);
    colors.helpbar_key = Color::RGB(160, 190, 130); // 抹茶绿

    colors.keyword = Color::RGB(160, 190, 130); // 抹茶绿 #a0be82
    colors.string = Color::RGB(200, 175, 120);  // 暖棕
    colors.comment = Color::RGB(100, 120, 105);
    colors.number = Color::RGB(210, 160, 100);   // 琥珀
    colors.function = Color::RGB(180, 200, 150); // 浅抹茶
    colors.type = Color::RGB(140, 170, 110);     // 深抹茶
    colors.operator_color = Color::RGB(160, 190, 130);

    colors.error = Color::RGB(200, 100, 90);
    colors.warning = Color::RGB(210, 160, 100);
    colors.info = Color::RGB(130, 160, 180);
    colors.success = Color::RGB(140, 170, 110);

    colors.dialog_bg = Color::RGB(48, 58, 50);
    colors.dialog_fg = Color::RGB(220, 218, 205);
    colors.dialog_title_bg = Color::RGB(42, 54, 46);
    colors.dialog_title_fg = Color::RGB(220, 218, 205);
    colors.dialog_border = Color::RGB(80, 100, 85);

    return colors;
}

ThemeColors Theme::Aurora() {
    ThemeColors colors;
    // Aurora: 极光主题，深蓝绿基底+极光青/紫/粉霓虹点缀，梦幻感
    colors.background = Color::RGB(18, 28, 38);             // #121c26 深蓝黑
    colors.foreground = Color::RGB(200, 210, 220);          // #c8d2dc
    colors.current_line = Color::RGB(24, 36, 48);           // #182430
    colors.selection = Color::RGB(35, 50, 65);              // #233241
    colors.line_number = Color::RGB(70, 95, 120);           // #465f78
    colors.line_number_current = Color::RGB(120, 200, 220); // 极光青

    colors.statusbar_bg = Color::RGB(24, 36, 48);
    colors.statusbar_fg = Color::RGB(200, 210, 220);

    colors.menubar_bg = Color::RGB(18, 28, 38);
    colors.menubar_fg = Color::RGB(200, 210, 220);

    colors.helpbar_bg = Color::RGB(24, 36, 48);
    colors.helpbar_fg = Color::RGB(70, 95, 120);
    colors.helpbar_key = Color::RGB(100, 220, 255); // 极光青

    colors.keyword = Color::RGB(180, 120, 255); // 极光紫 #b478ff
    colors.string = Color::RGB(100, 220, 255);  // 极光青 #64dcff
    colors.comment = Color::RGB(70, 95, 120);
    colors.number = Color::RGB(255, 150, 200);         // 极光粉
    colors.function = Color::RGB(100, 220, 255);       // 极光青
    colors.type = Color::RGB(180, 120, 255);           // 极光紫
    colors.operator_color = Color::RGB(255, 150, 200); // 极光粉

    colors.error = Color::RGB(255, 100, 130);
    colors.warning = Color::RGB(255, 200, 100);
    colors.info = Color::RGB(100, 220, 255);
    colors.success = Color::RGB(100, 255, 180); // 极光绿

    colors.dialog_bg = Color::RGB(24, 36, 48);
    colors.dialog_fg = Color::RGB(200, 210, 220);
    colors.dialog_title_bg = Color::RGB(30, 45, 60);
    colors.dialog_title_fg = Color::RGB(200, 210, 220);
    colors.dialog_border = Color::RGB(70, 120, 160);

    return colors;
}

ThemeColors Theme::Amber() {
    ThemeColors colors;
    // Amber: 近黑暖底 + 琥珀/金高亮，类似暖灯下的稿纸，护眼不刺眼
    colors.background = Color::RGB(22, 20, 18);             // #161412 暖黑
    colors.foreground = Color::RGB(220, 212, 198);          // #dcd4c6 暖米白
    colors.current_line = Color::RGB(32, 28, 24);           // #201c18
    colors.selection = Color::RGB(48, 42, 36);              // #302a24
    colors.line_number = Color::RGB(100, 88, 72);           // #645848
    colors.line_number_current = Color::RGB(220, 180, 100); // 琥珀

    colors.statusbar_bg = Color::RGB(28, 24, 20);
    colors.statusbar_fg = Color::RGB(220, 212, 198);

    colors.menubar_bg = Color::RGB(18, 16, 14);
    colors.menubar_fg = Color::RGB(220, 212, 198);

    colors.helpbar_bg = Color::RGB(28, 24, 20);
    colors.helpbar_fg = Color::RGB(100, 88, 72);
    colors.helpbar_key = Color::RGB(220, 170, 80); // 琥珀

    colors.keyword = Color::RGB(220, 170, 80); // 琥珀 #dcaa50
    colors.string = Color::RGB(180, 200, 140); // 橄榄绿
    colors.comment = Color::RGB(100, 88, 72);
    colors.number = Color::RGB(210, 150, 70);    // 深琥珀
    colors.function = Color::RGB(230, 195, 120); // 浅金
    colors.type = Color::RGB(200, 160, 90);      // 金棕
    colors.operator_color = Color::RGB(220, 170, 80);

    colors.error = Color::RGB(200, 90, 70);
    colors.warning = Color::RGB(210, 150, 70);
    colors.info = Color::RGB(160, 180, 200);
    colors.success = Color::RGB(140, 180, 100);

    colors.dialog_bg = Color::RGB(32, 28, 24);
    colors.dialog_fg = Color::RGB(220, 212, 198);
    colors.dialog_title_bg = Color::RGB(28, 24, 20);
    colors.dialog_title_fg = Color::RGB(220, 212, 198);
    colors.dialog_border = Color::RGB(100, 88, 72);

    return colors;
}

ThemeColors Theme::Mint() {
    ThemeColors colors;
    // Mint: 深灰底 + 薄荷青/冷色终端风，清爽不偏绿不偏蓝
    colors.background = Color::RGB(28, 34, 38);             // #1c2226 深灰
    colors.foreground = Color::RGB(200, 210, 208);          // #c8d2d0 冷灰白
    colors.current_line = Color::RGB(38, 46, 50);           // #262e32
    colors.selection = Color::RGB(48, 58, 62);              // #303a3e
    colors.line_number = Color::RGB(90, 110, 115);          // #5a6e73
    colors.line_number_current = Color::RGB(100, 230, 220); // 薄荷青

    colors.statusbar_bg = Color::RGB(38, 46, 50);
    colors.statusbar_fg = Color::RGB(200, 210, 208);

    colors.menubar_bg = Color::RGB(22, 28, 32);
    colors.menubar_fg = Color::RGB(200, 210, 208);

    colors.helpbar_bg = Color::RGB(38, 46, 50);
    colors.helpbar_fg = Color::RGB(90, 110, 115);
    colors.helpbar_key = Color::RGB(80, 220, 210); // 薄荷青

    colors.keyword = Color::RGB(80, 220, 210); // 薄荷青 #50dcd2
    colors.string = Color::RGB(170, 220, 200); // 浅薄荷
    colors.comment = Color::RGB(90, 110, 115);
    colors.number = Color::RGB(150, 210, 230);   // 冷青
    colors.function = Color::RGB(120, 230, 220); // 亮薄荷
    colors.type = Color::RGB(100, 200, 190);     // 深薄荷
    colors.operator_color = Color::RGB(80, 220, 210);

    colors.error = Color::RGB(230, 100, 110);
    colors.warning = Color::RGB(230, 200, 120);
    colors.info = Color::RGB(80, 220, 210);
    colors.success = Color::RGB(120, 220, 160); // 薄荷绿

    colors.dialog_bg = Color::RGB(38, 46, 50);
    colors.dialog_fg = Color::RGB(200, 210, 208);
    colors.dialog_title_bg = Color::RGB(32, 40, 44);
    colors.dialog_title_fg = Color::RGB(200, 210, 208);
    colors.dialog_border = Color::RGB(90, 120, 125);

    return colors;
}

ThemeColors Theme::Obsidian() {
    ThemeColors colors;
    // Obsidian: 深黑偏蓝灰，冷静克制，类似 Obsidian 编辑器默认暗色
    colors.background = Color::RGB(24, 25, 30);    // #18191e 深黑蓝灰
    colors.foreground = Color::RGB(200, 202, 210); // #c8cad2 冷灰白
    colors.current_line = Color::RGB(32, 34, 42);  // #20222a
    colors.selection = Color::RGB(45, 48, 58);     // #2d303a
    colors.line_number = Color::RGB(90, 95, 110);  // #5a5f6e
    colors.line_number_current = Color::RGB(140, 145, 165);

    colors.statusbar_bg = Color::RGB(32, 34, 42);
    colors.statusbar_fg = Color::RGB(200, 202, 210);

    colors.menubar_bg = Color::RGB(20, 21, 26);
    colors.menubar_fg = Color::RGB(200, 202, 210);

    colors.helpbar_bg = Color::RGB(32, 34, 42);
    colors.helpbar_fg = Color::RGB(90, 95, 110);
    colors.helpbar_key = Color::RGB(120, 200, 220); // 青

    colors.keyword = Color::RGB(180, 130, 230); // 淡紫
    colors.string = Color::RGB(150, 210, 170);  // 柔和绿
    colors.comment = Color::RGB(100, 108, 125);
    colors.number = Color::RGB(230, 180, 130);   // 暖橙
    colors.function = Color::RGB(120, 200, 220); // 青
    colors.type = Color::RGB(160, 180, 230);     // 淡蓝紫
    colors.operator_color = Color::RGB(180, 130, 230);

    colors.error = Color::RGB(230, 110, 120);
    colors.warning = Color::RGB(230, 180, 130);
    colors.info = Color::RGB(120, 200, 220);
    colors.success = Color::RGB(150, 210, 170);

    colors.dialog_bg = Color::RGB(32, 34, 42);
    colors.dialog_fg = Color::RGB(200, 202, 210);
    colors.dialog_title_bg = Color::RGB(45, 48, 58);
    colors.dialog_title_fg = Color::RGB(200, 202, 210);
    colors.dialog_border = Color::RGB(90, 95, 110);

    return colors;
}

ThemeColors Theme::Coffee() {
    ThemeColors colors;
    // Coffee: 咖啡棕深色，暖棕底+奶白/焦糖色，护眼不刺眼
    colors.background = Color::RGB(42, 34, 28);             // #2a221c 深咖啡
    colors.foreground = Color::RGB(220, 210, 198);          // #dcd2c6 奶白
    colors.current_line = Color::RGB(52, 42, 36);           // #342a24
    colors.selection = Color::RGB(72, 58, 48);              // #483a30
    colors.line_number = Color::RGB(120, 100, 85);          // #786455
    colors.line_number_current = Color::RGB(230, 200, 160); // 焦糖

    colors.statusbar_bg = Color::RGB(52, 42, 36);
    colors.statusbar_fg = Color::RGB(220, 210, 198);

    colors.menubar_bg = Color::RGB(35, 28, 24);
    colors.menubar_fg = Color::RGB(220, 210, 198);

    colors.helpbar_bg = Color::RGB(52, 42, 36);
    colors.helpbar_fg = Color::RGB(120, 100, 85);
    colors.helpbar_key = Color::RGB(200, 160, 100); // 焦糖

    colors.keyword = Color::RGB(210, 140, 90); // 棕橙
    colors.string = Color::RGB(160, 190, 140); // 橄榄绿
    colors.comment = Color::RGB(130, 110, 95);
    colors.number = Color::RGB(220, 180, 120);   // 焦糖
    colors.function = Color::RGB(200, 160, 100); // 焦糖
    colors.type = Color::RGB(180, 150, 110);     // 浅棕
    colors.operator_color = Color::RGB(210, 140, 90);

    colors.error = Color::RGB(200, 100, 90);
    colors.warning = Color::RGB(220, 160, 80);
    colors.info = Color::RGB(140, 180, 180); // 灰青
    colors.success = Color::RGB(160, 190, 140);

    colors.dialog_bg = Color::RGB(52, 42, 36);
    colors.dialog_fg = Color::RGB(220, 210, 198);
    colors.dialog_title_bg = Color::RGB(72, 58, 48);
    colors.dialog_title_fg = Color::RGB(220, 210, 198);
    colors.dialog_border = Color::RGB(120, 100, 85);

    return colors;
}

ThemeColors Theme::Ink() {
    ThemeColors colors;
    // Ink 墨色：深靛蓝黑底 + 金/米色字，传统墨水+金箔风格
    colors.background = Color::RGB(22, 22, 32);    // #161620 深靛黑
    colors.foreground = Color::RGB(230, 222, 200); // #e6dec8 米色
    colors.current_line = Color::RGB(30, 30, 42);
    colors.selection = Color::RGB(45, 42, 58);
    colors.line_number = Color::RGB(100, 95, 120);
    colors.line_number_current = Color::RGB(218, 195, 130); // 淡金

    colors.statusbar_bg = Color::RGB(28, 28, 40);
    colors.statusbar_fg = Color::RGB(230, 222, 200);

    colors.menubar_bg = Color::RGB(18, 18, 28);
    colors.menubar_fg = Color::RGB(230, 222, 200);

    colors.helpbar_bg = Color::RGB(28, 28, 40);
    colors.helpbar_fg = Color::RGB(100, 95, 120);
    colors.helpbar_key = Color::RGB(218, 195, 130);

    colors.keyword = Color::RGB(198, 165, 100); // 金
    colors.string = Color::RGB(160, 190, 165);  // 青绿
    colors.comment = Color::RGB(110, 105, 130);
    colors.number = Color::RGB(218, 195, 130);   // 淡金
    colors.function = Color::RGB(180, 170, 140); // 米黄
    colors.type = Color::RGB(170, 160, 195);     // 淡紫
    colors.operator_color = Color::RGB(198, 165, 100);

    colors.error = Color::RGB(200, 120, 110);
    colors.warning = Color::RGB(218, 175, 100);
    colors.info = Color::RGB(140, 180, 180);
    colors.success = Color::RGB(160, 190, 165);

    colors.dialog_bg = Color::RGB(30, 30, 42);
    colors.dialog_fg = Color::RGB(230, 222, 200);
    colors.dialog_title_bg = Color::RGB(45, 42, 58);
    colors.dialog_title_fg = Color::RGB(218, 195, 130);
    colors.dialog_border = Color::RGB(100, 95, 120);

    return colors;
}

ThemeColors Theme::Sakura() {
    ThemeColors colors;
    // Sakura 樱：浅粉白底 + 深粉/玫瑰灰字，樱花感浅色主题
    colors.background = Color::RGB(255, 248, 250); // #fff8fa 浅樱白
    colors.foreground = Color::RGB(80, 65, 75);    // #50414b 玫瑰灰
    colors.current_line = Color::RGB(250, 240, 245);
    colors.selection = Color::RGB(255, 230, 240);
    colors.line_number = Color::RGB(180, 155, 170);
    colors.line_number_current = Color::RGB(140, 90, 110);

    colors.statusbar_bg = Color::RGB(248, 238, 245);
    colors.statusbar_fg = Color::RGB(80, 65, 75);

    colors.menubar_bg = Color::RGB(252, 245, 248);
    colors.menubar_fg = Color::RGB(80, 65, 75);

    colors.helpbar_bg = Color::RGB(248, 238, 245);
    colors.helpbar_fg = Color::RGB(160, 130, 150);
    colors.helpbar_key = Color::RGB(200, 100, 130); // 樱粉

    colors.keyword = Color::RGB(180, 80, 120); // 深粉
    colors.string = Color::RGB(120, 140, 100); // 橄榄绿
    colors.comment = Color::RGB(170, 155, 165);
    colors.number = Color::RGB(200, 120, 100);   // 暖珊瑚
    colors.function = Color::RGB(140, 100, 130); // 梅紫
    colors.type = Color::RGB(100, 130, 150);     // 灰蓝
    colors.operator_color = Color::RGB(180, 80, 120);

    colors.error = Color::RGB(200, 70, 90);
    colors.warning = Color::RGB(210, 140, 80);
    colors.info = Color::RGB(100, 130, 150);
    colors.success = Color::RGB(120, 140, 100);

    colors.dialog_bg = Color::RGB(252, 245, 250);
    colors.dialog_fg = Color::RGB(80, 65, 75);
    colors.dialog_title_bg = Color::RGB(255, 235, 245);
    colors.dialog_title_fg = Color::RGB(140, 90, 110);
    colors.dialog_border = Color::RGB(220, 190, 205);

    return colors;
}

ThemeColors Theme::SakuraDark() {
    ThemeColors colors;
    // Sakura Dark 樱暗色版：深玫/紫灰底 + 粉/米色字，与 Sakura 同色系暗色
    colors.background = Color::RGB(42, 35, 45);    // #2a232d 深玫灰
    colors.foreground = Color::RGB(230, 218, 225); // #e6dae1 淡粉白
    colors.current_line = Color::RGB(52, 44, 55);
    colors.selection = Color::RGB(70, 58, 72);
    colors.line_number = Color::RGB(130, 110, 125);
    colors.line_number_current = Color::RGB(255, 200, 220); // 樱粉

    colors.statusbar_bg = Color::RGB(50, 42, 53);
    colors.statusbar_fg = Color::RGB(230, 218, 225);

    colors.menubar_bg = Color::RGB(35, 30, 38);
    colors.menubar_fg = Color::RGB(230, 218, 225);

    colors.helpbar_bg = Color::RGB(50, 42, 53);
    colors.helpbar_fg = Color::RGB(130, 110, 125);
    colors.helpbar_key = Color::RGB(255, 180, 200); // 樱粉

    colors.keyword = Color::RGB(255, 160, 200); // 亮粉
    colors.string = Color::RGB(170, 210, 170);  // 柔绿
    colors.comment = Color::RGB(120, 105, 115);
    colors.number = Color::RGB(255, 190, 160);   // 暖珊瑚
    colors.function = Color::RGB(220, 180, 210); // 梅紫
    colors.type = Color::RGB(180, 200, 220);     // 灰蓝
    colors.operator_color = Color::RGB(255, 160, 200);

    colors.error = Color::RGB(255, 120, 140);
    colors.warning = Color::RGB(255, 200, 140);
    colors.info = Color::RGB(180, 200, 220);
    colors.success = Color::RGB(170, 210, 170);

    colors.dialog_bg = Color::RGB(52, 44, 55);
    colors.dialog_fg = Color::RGB(230, 218, 225);
    colors.dialog_title_bg = Color::RGB(70, 58, 72);
    colors.dialog_title_fg = Color::RGB(255, 200, 220);
    colors.dialog_border = Color::RGB(130, 110, 125);

    return colors;
}

ThemeColors Theme::DarkPlusMoonLight() {
    ThemeColors colors;
    // Dark Plus Moon Light: 深邃夜空黑底 + 月光银白 + 蓝紫渐变，现代优雅暗色主题
    // 灵感来源：VSCode Dark+、皎洁月光、深夜编程、静谧优雅
    colors.background = Color::RGB(10, 12, 20);             // #0A0C14 深邃夜空黑
    colors.foreground = Color::RGB(220, 225, 235);          // #DCE1EB 月光银白
    colors.current_line = Color::RGB(25, 30, 45);           // #191E2D 月光下的深蓝
    colors.selection = Color::RGB(40, 48, 70);              // #283046 夜色选择区
    colors.line_number = Color::RGB(100, 110, 130);         // #646E82 灰蓝行号
    colors.line_number_current = Color::RGB(180, 200, 255); // #B4C8FF 月光高亮✨

    colors.statusbar_bg = Color::RGB(20, 25, 40);    // #141928 深夜蓝
    colors.statusbar_fg = Color::RGB(200, 210, 230); // #C8D2E6 银灰

    colors.menubar_bg = Color::RGB(15, 18, 28);    // #0F121C 深空蓝
    colors.menubar_fg = Color::RGB(220, 225, 235); // #DCE1EB 月光银白

    colors.helpbar_bg = Color::RGB(20, 25, 40);
    colors.helpbar_fg = Color::RGB(150, 165, 195);  // #96A5C3 月灰
    colors.helpbar_key = Color::RGB(120, 180, 255); // #78B4FF 月光蓝

    // Dark Plus Moon Light 语法高亮：月光银 + 蓝紫渐变
    colors.keyword = Color::RGB(200, 120, 255);        // #C878FF 紫罗兰（月光魔法）
    colors.string = Color::RGB(150, 220, 180);         // #96DCB4 月光绿（极光）
    colors.comment = Color::RGB(90, 100, 120);         // #5A6478 夜灰（低调注释）
    colors.number = Color::RGB(255, 180, 120);         // #FFB478 月橙（温暖光晕）
    colors.function = Color::RGB(100, 180, 255);       // #64B4FF 天空蓝（函数调用）
    colors.type = Color::RGB(255, 150, 200);           // #FF96C8 樱花粉（类型）
    colors.operator_color = Color::RGB(180, 200, 255); // #B4C8FF 月光银白

    colors.error = Color::RGB(255, 100, 120);   // #FF6478 月蚀红
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 警戒橙
    colors.info = Color::RGB(100, 180, 255);    // #64B4FF 天空蓝
    colors.success = Color::RGB(120, 220, 180); // #78DCB4 极光绿

    colors.dialog_bg = Color::RGB(30, 38, 58);          // #1E263A 深夜蓝（对话框）
    colors.dialog_fg = Color::RGB(220, 225, 235);       // #DCE1EB 月光银白
    colors.dialog_title_bg = Color::RGB(80, 120, 200);  // #5078C8 月光蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(90, 100, 120);    // #5A6478 夜灰边框

    return colors;
}

ThemeColors Theme::Monochrome() {
    ThemeColors colors;
    // Monochrome 黑白版：纯黑/白/灰，无彩色
    colors.background = Color::RGB(18, 18, 18);    // #121212 近黑
    colors.foreground = Color::RGB(240, 240, 240); // 亮灰白
    colors.current_line = Color::RGB(28, 28, 28);
    colors.selection = Color::RGB(50, 50, 50);
    colors.line_number = Color::RGB(100, 100, 100);
    colors.line_number_current = Color::RGB(220, 220, 220);

    colors.statusbar_bg = Color::RGB(30, 30, 30);
    colors.statusbar_fg = Color::RGB(240, 240, 240);

    colors.menubar_bg = Color::RGB(22, 22, 22);
    colors.menubar_fg = Color::RGB(240, 240, 240);

    colors.helpbar_bg = Color::RGB(30, 30, 30);
    colors.helpbar_fg = Color::RGB(140, 140, 140);
    colors.helpbar_key = Color::RGB(200, 200, 200);

    colors.keyword = Color::RGB(255, 255, 255);
    colors.string = Color::RGB(180, 180, 180);
    colors.comment = Color::RGB(110, 110, 110);
    colors.number = Color::RGB(200, 200, 200);
    colors.function = Color::RGB(220, 220, 220);
    colors.type = Color::RGB(190, 190, 190);
    colors.operator_color = Color::RGB(255, 255, 255);

    colors.error = Color::RGB(255, 255, 255); // 用白表示错误（B&W 下用粗体/样式区分）
    colors.warning = Color::RGB(180, 180, 180);
    colors.info = Color::RGB(160, 160, 160);
    colors.success = Color::RGB(200, 200, 200);

    colors.dialog_bg = Color::RGB(28, 28, 28);
    colors.dialog_fg = Color::RGB(240, 240, 240);
    colors.dialog_title_bg = Color::RGB(40, 40, 40);
    colors.dialog_title_fg = Color::RGB(240, 240, 240);
    colors.dialog_border = Color::RGB(90, 90, 90);

    return colors;
}

ThemeColors Theme::NeonNoir() {
    ThemeColors colors;
    // Neon Noir 霓虹 noir：纯黑底 + 青/品红高对比，冷峻赛博感
    colors.background = Color::RGB(8, 8, 12);      // #08080c 近黑
    colors.foreground = Color::RGB(230, 232, 240); // 冷灰白
    colors.current_line = Color::RGB(18, 18, 26);
    colors.selection = Color::RGB(35, 35, 50);
    colors.line_number = Color::RGB(80, 85, 110);
    colors.line_number_current = Color::RGB(0, 255, 255); // 氰青

    colors.statusbar_bg = Color::RGB(15, 15, 22);
    colors.statusbar_fg = Color::RGB(230, 232, 240);

    colors.menubar_bg = Color::RGB(10, 10, 16);
    colors.menubar_fg = Color::RGB(230, 232, 240);

    colors.helpbar_bg = Color::RGB(15, 15, 22);
    colors.helpbar_fg = Color::RGB(100, 105, 130);
    colors.helpbar_key = Color::RGB(0, 255, 200); // 青绿

    colors.keyword = Color::RGB(255, 0, 128); // 品红
    colors.string = Color::RGB(0, 255, 200);  // 青绿
    colors.comment = Color::RGB(90, 95, 120);
    colors.number = Color::RGB(255, 128, 200); // 粉
    colors.function = Color::RGB(0, 255, 255); // 氰青
    colors.type = Color::RGB(180, 100, 255);   // 紫
    colors.operator_color = Color::RGB(255, 0, 128);

    colors.error = Color::RGB(255, 50, 100);
    colors.warning = Color::RGB(255, 200, 0);
    colors.info = Color::RGB(0, 255, 255);
    colors.success = Color::RGB(0, 255, 200);

    colors.dialog_bg = Color::RGB(14, 14, 22);
    colors.dialog_fg = Color::RGB(230, 232, 240);
    colors.dialog_title_bg = Color::RGB(25, 25, 38);
    colors.dialog_title_fg = Color::RGB(0, 255, 255);
    colors.dialog_border = Color::RGB(80, 0, 120); // 深品紫

    return colors;
}

ThemeColors Theme::WarmSepia() {
    ThemeColors colors;
    // Warm Sepia 暖棕褐：旧报纸/羊皮纸感，米黄底+褐字
    colors.background = Color::RGB(245, 238, 220); // #f5eedc 暖米
    colors.foreground = Color::RGB(80, 65, 50);    // 深褐
    colors.current_line = Color::RGB(235, 226, 205);
    colors.selection = Color::RGB(220, 205, 175);
    colors.line_number = Color::RGB(140, 120, 95);
    colors.line_number_current = Color::RGB(100, 75, 50);

    colors.statusbar_bg = Color::RGB(230, 218, 195);
    colors.statusbar_fg = Color::RGB(80, 65, 50);

    colors.menubar_bg = Color::RGB(238, 228, 208);
    colors.menubar_fg = Color::RGB(80, 65, 50);

    colors.helpbar_bg = Color::RGB(230, 218, 195);
    colors.helpbar_fg = Color::RGB(120, 100, 75);
    colors.helpbar_key = Color::RGB(140, 90, 50); // 焦茶

    colors.keyword = Color::RGB(120, 70, 30); // 深棕
    colors.string = Color::RGB(90, 110, 70);  // 橄榄褐
    colors.comment = Color::RGB(150, 130, 100);
    colors.number = Color::RGB(140, 90, 50);   // 焦糖
    colors.function = Color::RGB(100, 75, 45); // 褐
    colors.type = Color::RGB(110, 85, 55);
    colors.operator_color = Color::RGB(100, 70, 40);

    colors.error = Color::RGB(160, 60, 50);
    colors.warning = Color::RGB(150, 100, 40);
    colors.info = Color::RGB(70, 90, 100);
    colors.success = Color::RGB(80, 110, 70);

    colors.dialog_bg = Color::RGB(238, 230, 212);
    colors.dialog_fg = Color::RGB(80, 65, 50);
    colors.dialog_title_bg = Color::RGB(220, 205, 175);
    colors.dialog_title_fg = Color::RGB(80, 65, 50);
    colors.dialog_border = Color::RGB(160, 140, 110);

    return colors;
}

ThemeColors Theme::Colorful() {
    ThemeColors colors;
    // Colorful 彩色：深色底 + 红/橙/黄/绿/青/蓝/紫高饱和语法高亮
    colors.background = Color::RGB(28, 28, 35);    // 深灰紫底
    colors.foreground = Color::RGB(240, 240, 245); // 近白
    colors.current_line = Color::RGB(38, 38, 48);
    colors.selection = Color::RGB(60, 55, 75);
    colors.line_number = Color::RGB(130, 120, 150);
    colors.line_number_current = Color::RGB(255, 200, 100); // 橙黄

    colors.statusbar_bg = Color::RGB(40, 38, 52);
    colors.statusbar_fg = Color::RGB(240, 240, 245);

    colors.menubar_bg = Color::RGB(32, 30, 42);
    colors.menubar_fg = Color::RGB(240, 240, 245);

    colors.helpbar_bg = Color::RGB(40, 38, 52);
    colors.helpbar_fg = Color::RGB(180, 170, 200);
    colors.helpbar_key = Color::RGB(100, 220, 255); // 青

    colors.keyword = Color::RGB(255, 100, 150);  // 粉红
    colors.string = Color::RGB(255, 220, 100);   // 黄
    colors.comment = Color::RGB(120, 200, 120);  // 绿
    colors.number = Color::RGB(255, 160, 80);    // 橙
    colors.function = Color::RGB(100, 220, 255); // 青
    colors.type = Color::RGB(180, 140, 255);     // 紫
    colors.operator_color = Color::RGB(255, 120, 180);

    colors.error = Color::RGB(255, 80, 100);
    colors.warning = Color::RGB(255, 200, 80);
    colors.info = Color::RGB(100, 200, 255);
    colors.success = Color::RGB(120, 255, 140);

    colors.dialog_bg = Color::RGB(35, 34, 48);
    colors.dialog_fg = Color::RGB(240, 240, 245);
    colors.dialog_title_bg = Color::RGB(55, 50, 75);
    colors.dialog_title_fg = Color::RGB(255, 220, 150);
    colors.dialog_border = Color::RGB(150, 100, 220);

    return colors;
}

ThemeColors Theme::Microsoft() {
    ThemeColors colors;
    // Microsoft 微软/Fluent：深灰底 + 蓝色主色 #0078D4
    colors.background = Color::RGB(30, 30, 30);    // #1E1E1E
    colors.foreground = Color::RGB(212, 212, 212); // #D4D4D4
    colors.current_line = Color::RGB(42, 42, 42);
    colors.selection = Color::RGB(38, 79, 120); // 深蓝选中
    colors.line_number = Color::RGB(110, 110, 110);
    colors.line_number_current = Color::RGB(0, 120, 215); // 微软蓝 #0078D7

    colors.statusbar_bg = Color::RGB(0, 122, 204); // 蓝底状态栏
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(45, 45, 48); // #2D2D30
    colors.menubar_fg = Color::RGB(241, 241, 241);

    colors.helpbar_bg = Color::RGB(45, 45, 48);
    colors.helpbar_fg = Color::RGB(180, 180, 180);
    colors.helpbar_key = Color::RGB(78, 201, 176); // 青绿 #4EC9B0

    colors.keyword = Color::RGB(197, 134, 192);  // 紫 #C586C0
    colors.string = Color::RGB(206, 145, 120);   // 橙褐 #CE9178
    colors.comment = Color::RGB(106, 153, 85);   // 绿 #6A9955
    colors.number = Color::RGB(181, 206, 168);   // 浅绿 #B5CEA8
    colors.function = Color::RGB(220, 220, 170); // 黄 #DCDCAA
    colors.type = Color::RGB(78, 201, 176);      // 青 #4EC9B0
    colors.operator_color = Color::RGB(212, 212, 212);

    colors.error = Color::RGB(244, 63, 94);    // 红 #F43F5E
    colors.warning = Color::RGB(245, 158, 11); // 橙 #F59E0B
    colors.info = Color::RGB(0, 122, 204);     // 蓝 #0078D4
    colors.success = Color::RGB(106, 153, 85); // 绿 #6A9955

    colors.dialog_bg = Color::RGB(37, 37, 38); // #252526
    colors.dialog_fg = Color::RGB(212, 212, 212);
    colors.dialog_title_bg = Color::RGB(0, 122, 204);
    colors.dialog_title_fg = Color::RGB(255, 255, 255);
    colors.dialog_border = Color::RGB(62, 62, 66); // #3E3E42

    return colors;
}

ThemeColors Theme::Google() {
    ThemeColors colors;
    // Google 谷歌：浅色底 + 品牌四色 #4285F4蓝 #EA4335红 #FBBC04黄 #34A853绿
    colors.background = Color::RGB(255, 255, 255);         // 白底
    colors.foreground = Color::RGB(60, 64, 67);            // #3C4043 深灰
    colors.current_line = Color::RGB(248, 249, 250);       // 浅灰
    colors.selection = Color::RGB(232, 240, 254);          // 淡蓝
    colors.line_number = Color::RGB(128, 134, 139);        // 中灰
    colors.line_number_current = Color::RGB(66, 133, 244); // 谷歌蓝 #4285F4

    colors.statusbar_bg = Color::RGB(66, 133, 244); // 蓝底
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(248, 249, 250);
    colors.menubar_fg = Color::RGB(60, 64, 67);

    colors.helpbar_bg = Color::RGB(241, 243, 244); // #F1F3F4
    colors.helpbar_fg = Color::RGB(95, 99, 104);
    colors.helpbar_key = Color::RGB(66, 133, 244); // 蓝

    colors.keyword = Color::RGB(234, 67, 53);   // 红 #EA4335
    colors.string = Color::RGB(52, 168, 83);    // 绿 #34A853
    colors.comment = Color::RGB(128, 134, 139); // 灰
    colors.number = Color::RGB(251, 188, 4);    // 黄 #FBBC04
    colors.function = Color::RGB(66, 133, 244); // 蓝 #4285F4
    colors.type = Color::RGB(52, 168, 83);      // 绿
    colors.operator_color = Color::RGB(60, 64, 67);

    colors.error = Color::RGB(234, 67, 53);   // 红
    colors.warning = Color::RGB(251, 188, 4); // 黄
    colors.info = Color::RGB(66, 133, 244);   // 蓝
    colors.success = Color::RGB(52, 168, 83); // 绿

    colors.dialog_bg = Color::RGB(255, 255, 255);
    colors.dialog_fg = Color::RGB(60, 64, 67);
    colors.dialog_title_bg = Color::RGB(66, 133, 244);
    colors.dialog_title_fg = Color::RGB(255, 255, 255);
    colors.dialog_border = Color::RGB(218, 220, 224); // #DADCE0

    return colors;
}

ThemeColors Theme::Meta() {
    ThemeColors colors;
    // Meta 深色：深灰底 + 主色蓝 #1877F2，类似 Meta 开发者/应用风格
    colors.background = Color::RGB(24, 25, 26);            // #18191A 近黑
    colors.foreground = Color::RGB(228, 230, 235);         // #E4E6EB
    colors.current_line = Color::RGB(36, 37, 38);          // #242526
    colors.selection = Color::RGB(55, 58, 63);             // #373A40
    colors.line_number = Color::RGB(96, 103, 112);         // #606770
    colors.line_number_current = Color::RGB(24, 119, 242); // Meta 蓝 #1877F2

    colors.statusbar_bg = Color::RGB(24, 119, 242); // Meta 蓝
    colors.statusbar_fg = Color::RGB(255, 255, 255);

    colors.menubar_bg = Color::RGB(36, 37, 38); // #242526
    colors.menubar_fg = Color::RGB(228, 230, 235);

    colors.helpbar_bg = Color::RGB(36, 37, 38);
    colors.helpbar_fg = Color::RGB(176, 179, 184); // #B0B3B8
    colors.helpbar_key = Color::RGB(24, 119, 242); // 蓝

    colors.keyword = Color::RGB(242, 153, 74);  // 橙 #F2994A
    colors.string = Color::RGB(126, 214, 223);  // 青 #7ED6DF
    colors.comment = Color::RGB(96, 103, 112);  // #606770
    colors.number = Color::RGB(255, 195, 77);   // 黄 #FFC34D
    colors.function = Color::RGB(24, 119, 242); // Meta 蓝
    colors.type = Color::RGB(126, 214, 223);    // 青
    colors.operator_color = Color::RGB(228, 230, 235);

    colors.error = Color::RGB(244, 67, 54);    // 红
    colors.warning = Color::RGB(255, 195, 77); // 黄
    colors.info = Color::RGB(24, 119, 242);    // 蓝
    colors.success = Color::RGB(76, 175, 80);  // 绿 #4CAF50

    colors.dialog_bg = Color::RGB(36, 37, 38);
    colors.dialog_fg = Color::RGB(228, 230, 235);
    colors.dialog_title_bg = Color::RGB(24, 119, 242);
    colors.dialog_title_fg = Color::RGB(255, 255, 255);
    colors.dialog_border = Color::RGB(62, 65, 72); // #3E4148

    return colors;
}

ThemeColors Theme::IntelliJDark() {
    ThemeColors colors;
    // IntelliJ IDEA Dark: 深灰蓝基底 + 紫/绿/蓝/橙，JetBrains 经典 IDE 主题，适合长时间编程
    colors.background = Color::RGB(43, 43, 43);             // #2B2B2B
    colors.foreground = Color::RGB(187, 187, 187);          // #BBBBBB
    colors.current_line = Color::RGB(50, 50, 50);           // #323232
    colors.selection = Color::RGB(65, 65, 65);              // #414141
    colors.line_number = Color::RGB(128, 128, 128);         // #808080
    colors.line_number_current = Color::RGB(187, 187, 187); // #BBBBBB

    colors.statusbar_bg = Color::RGB(55, 55, 55);    // #373737
    colors.statusbar_fg = Color::RGB(187, 187, 187); // #BBBBBB

    colors.menubar_bg = Color::RGB(43, 43, 43);    // #2B2B2B
    colors.menubar_fg = Color::RGB(187, 187, 187); // #BBBBBB

    colors.helpbar_bg = Color::RGB(55, 55, 55);
    colors.helpbar_fg = Color::RGB(128, 128, 128); // #808080
    colors.helpbar_key = Color::RGB(78, 171, 255); // #4EABFF  IntelliJ 蓝

    // IntelliJ Dark 语法高亮：紫关键词/绿字符串/蓝函数/橙数字/青类型
    colors.keyword = Color::RGB(204, 120, 255);        // #CC78FF 紫
    colors.string = Color::RGB(106, 213, 136);         // #6AD588 绿
    colors.comment = Color::RGB(128, 128, 128);        // #808080 灰
    colors.number = Color::RGB(187, 181, 41);          // #BBB529 黄
    colors.function = Color::RGB(78, 171, 255);        // #4EABFF 蓝
    colors.type = Color::RGB(210, 149, 53);            // #D29535 橙
    colors.operator_color = Color::RGB(187, 187, 187); // #BBBBBB

    colors.error = Color::RGB(255, 100, 100);   // #FF6464 红
    colors.warning = Color::RGB(255, 191, 0);   // #FFBF00 黄
    colors.info = Color::RGB(78, 171, 255);     // #4EABFF 蓝
    colors.success = Color::RGB(106, 213, 136); // #6AD588 绿

    colors.dialog_bg = Color::RGB(50, 50, 50);          // #323232
    colors.dialog_fg = Color::RGB(187, 187, 187);       // #BBBBBB
    colors.dialog_title_bg = Color::RGB(43, 43, 43);    // #2B2B2B
    colors.dialog_title_fg = Color::RGB(187, 187, 187); // #BBBBBB
    colors.dialog_border = Color::RGB(77, 77, 77);      // #4D4D4D

    return colors;
}

ThemeColors Theme::DoomOne() {
    ThemeColors colors;
    // Doom One: Emacs Doom 默认主题，深灰蓝基底 + 紫红/绿/蓝/橙，经典 Emacs 风格
    colors.background = Color::RGB(40, 44, 52);             // #282C34
    colors.foreground = Color::RGB(171, 178, 191);          // #ABB2BF
    colors.current_line = Color::RGB(44, 48, 57);           // #2C3039
    colors.selection = Color::RGB(58, 63, 75);              // #3A3F4B
    colors.line_number = Color::RGB(92, 99, 112);           // #5C6370
    colors.line_number_current = Color::RGB(171, 178, 191); // #ABB2BF

    colors.statusbar_bg = Color::RGB(35, 39, 46);    // #23272E
    colors.statusbar_fg = Color::RGB(171, 178, 191); // #ABB2BF

    colors.menubar_bg = Color::RGB(40, 44, 52);    // #282C34
    colors.menubar_fg = Color::RGB(171, 178, 191); // #ABB2BF

    colors.helpbar_bg = Color::RGB(35, 39, 46);
    colors.helpbar_fg = Color::RGB(92, 99, 112);   // #5C6370
    colors.helpbar_key = Color::RGB(97, 175, 239); // #61AFEF 亮蓝

    // Doom One 语法高亮：紫红关键词/绿字符串/蓝函数/橙数字/青类型
    colors.keyword = Color::RGB(198, 120, 221);        // #C678DD 紫红
    colors.string = Color::RGB(152, 195, 121);         // #98C379 绿
    colors.comment = Color::RGB(92, 99, 112);          // #5C6370 灰
    colors.number = Color::RGB(209, 154, 102);         // #D19A66 橙
    colors.function = Color::RGB(97, 175, 239);        // #61AFEF 蓝
    colors.type = Color::RGB(229, 192, 123);           // #E5C07B 黄
    colors.operator_color = Color::RGB(171, 178, 191); // #ABB2BF

    colors.error = Color::RGB(224, 108, 117);   // #E06C75 红
    colors.warning = Color::RGB(229, 192, 123); // #E5C07B 黄
    colors.info = Color::RGB(97, 175, 239);     // #61AFEF 蓝
    colors.success = Color::RGB(152, 195, 121); // #98C379 绿

    colors.dialog_bg = Color::RGB(44, 48, 57);          // #2C3039
    colors.dialog_fg = Color::RGB(171, 178, 191);       // #ABB2BF
    colors.dialog_title_bg = Color::RGB(40, 44, 52);    // #282C34
    colors.dialog_title_fg = Color::RGB(171, 178, 191); // #ABB2BF
    colors.dialog_border = Color::RGB(76, 82, 97);      // #4C5261

    return colors;
}

ThemeColors Theme::Andromeda() {
    ThemeColors colors;
    // Andromeda: 深蓝紫基底 + 鲜艳语法色，流行 VSCode 主题，适合长时间编程
    colors.background = Color::RGB(27, 31, 43);             // #1B1F2B
    colors.foreground = Color::RGB(205, 214, 244);          // #CDD6F4
    colors.current_line = Color::RGB(35, 40, 55);           // #232837
    colors.selection = Color::RGB(45, 51, 70);              // #2D3346
    colors.line_number = Color::RGB(92, 102, 120);          // #5C6678
    colors.line_number_current = Color::RGB(205, 214, 244); // #CDD6F4

    colors.statusbar_bg = Color::RGB(22, 26, 36);    // #161A24
    colors.statusbar_fg = Color::RGB(205, 214, 244); // #CDD6F4

    colors.menubar_bg = Color::RGB(27, 31, 43);    // #1B1F2B
    colors.menubar_fg = Color::RGB(205, 214, 244); // #CDD6F4

    colors.helpbar_bg = Color::RGB(22, 26, 36);
    colors.helpbar_fg = Color::RGB(92, 102, 120);  // #5C6678
    colors.helpbar_key = Color::RGB(89, 151, 234); // #5997EA 亮蓝

    // Andromeda 语法高亮：紫红关键词/绿字符串/蓝函数/橙数字/青类型
    colors.keyword = Color::RGB(255, 105, 180);        // #FF69B4 紫红
    colors.string = Color::RGB(152, 195, 121);         // #98C379 绿
    colors.comment = Color::RGB(92, 102, 120);         // #5C6678 灰
    colors.number = Color::RGB(255, 171, 82);          // #FFAB52 橙
    colors.function = Color::RGB(89, 151, 234);        // #5997EA 蓝
    colors.type = Color::RGB(255, 198, 109);           // #FFC66D 黄
    colors.operator_color = Color::RGB(205, 214, 244); // #CDD6F4

    colors.error = Color::RGB(255, 85, 85);     // #FF5555 红
    colors.warning = Color::RGB(255, 198, 109); // #FFC66D 黄
    colors.info = Color::RGB(89, 151, 234);     // #5997EA 蓝
    colors.success = Color::RGB(152, 195, 121); // #98C379 绿

    colors.dialog_bg = Color::RGB(35, 40, 55);          // #232837
    colors.dialog_fg = Color::RGB(205, 214, 244);       // #CDD6F4
    colors.dialog_title_bg = Color::RGB(27, 31, 43);    // #1B1F2B
    colors.dialog_title_fg = Color::RGB(205, 214, 244); // #CDD6F4
    colors.dialog_border = Color::RGB(68, 78, 100);     // #444E64

    return colors;
}

ThemeColors Theme::DeepSpace() {
    ThemeColors colors;
    // Deep Space: 深空宇宙主题，深蓝紫基底配合星云色彩，适合长时间编程
    colors.background = Color::RGB(15, 18, 32);             // #0F1220 深空蓝黑
    colors.foreground = Color::RGB(210, 215, 235);          // #D2D7EB 淡星白
    colors.current_line = Color::RGB(22, 26, 45);           // #161A2D 深空蓝
    colors.selection = Color::RGB(35, 42, 70);              // #232A46 星云紫
    colors.line_number = Color::RGB(80, 90, 120);           // #505A78 暗星灰
    colors.line_number_current = Color::RGB(140, 180, 255); // #8CB4FF 亮星蓝

    colors.statusbar_bg = Color::RGB(20, 24, 42);    // #14182A 深空蓝
    colors.statusbar_fg = Color::RGB(210, 215, 235); // #D2D7EB 淡星白

    colors.menubar_bg = Color::RGB(15, 18, 32);    // #0F1220
    colors.menubar_fg = Color::RGB(210, 215, 235); // #D2D7EB

    colors.helpbar_bg = Color::RGB(20, 24, 42);
    colors.helpbar_fg = Color::RGB(100, 110, 145);  // #646E91 星云灰
    colors.helpbar_key = Color::RGB(180, 140, 255); // #B48CFF 星云紫

    // Deep Space 语法高亮：星云紫关键词/极光绿字符串/星蓝函数/超新星橙数字/银河青类型
    colors.keyword = Color::RGB(180, 140, 255);        // #B48CFF 星云紫
    colors.string = Color::RGB(130, 220, 180);         // #82DCB4 极光绿
    colors.comment = Color::RGB(90, 100, 130);         // #5A6482 暗星云
    colors.number = Color::RGB(255, 160, 100);         // #FFA064 超新星橙
    colors.function = Color::RGB(100, 180, 255);       // #64B4FF 星蓝
    colors.type = Color::RGB(100, 230, 230);           // #64E6E6 银河青
    colors.operator_color = Color::RGB(200, 160, 240); // #C8A0F0 淡紫

    colors.error = Color::RGB(255, 100, 120);   // #FF6478 红矮星
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 黄巨星
    colors.info = Color::RGB(100, 180, 255);    // #64B4FF 星蓝
    colors.success = Color::RGB(130, 220, 150); // #82DC96 极光绿

    colors.dialog_bg = Color::RGB(22, 26, 45);          // #161A2D
    colors.dialog_fg = Color::RGB(210, 215, 235);       // #D2D7EB
    colors.dialog_title_bg = Color::RGB(15, 18, 32);    // #0F1220
    colors.dialog_title_fg = Color::RGB(180, 140, 255); // #B48CFF 星云紫
    colors.dialog_border = Color::RGB(60, 70, 110);     // #3C466E 深空边框

    return colors;
}

ThemeColors Theme::Volcanic() {
    ThemeColors colors;
    // Volcanic: 火山岩浆主题，深黑红基底配合岩浆橙/红色调，热烈而神秘
    colors.background = Color::RGB(18, 12, 12);            // #120C0C 火山岩黑
    colors.foreground = Color::RGB(235, 220, 210);         // #EBDCD2 灰烬白
    colors.current_line = Color::RGB(35, 20, 18);          // #231412 熔岩暗红
    colors.selection = Color::RGB(60, 30, 25);             // #3C1E19 深熔岩
    colors.line_number = Color::RGB(120, 80, 70);          // #785046 火山灰
    colors.line_number_current = Color::RGB(255, 140, 80); // #FF8C50 岩浆橙

    colors.statusbar_bg = Color::RGB(45, 20, 18);    // #2D1412 深熔岩
    colors.statusbar_fg = Color::RGB(235, 220, 210); // #EBDCD2 灰烬白

    colors.menubar_bg = Color::RGB(18, 12, 12);    // #120C0C
    colors.menubar_fg = Color::RGB(235, 220, 210); // #EBDCD2

    colors.helpbar_bg = Color::RGB(45, 20, 18);
    colors.helpbar_fg = Color::RGB(140, 100, 90);  // #8C645A 暖灰
    colors.helpbar_key = Color::RGB(255, 100, 60); // #FF643C 烈焰橙

    // Volcanic 语法高亮：岩浆红关键词/硫磺黄字符串/火山灰注释/熔岩橙数字/烟灰蓝函数
    colors.keyword = Color::RGB(255, 80, 60);         // #FF503C 岩浆红
    colors.string = Color::RGB(255, 200, 80);         // #FFC850 硫磺黄
    colors.comment = Color::RGB(110, 80, 75);         // #6E504B 火山灰褐
    colors.number = Color::RGB(255, 140, 50);         // #FF8C32 熔岩橙
    colors.function = Color::RGB(180, 160, 140);      // #B4A08C 烟灰
    colors.type = Color::RGB(255, 160, 100);          // #FFA064 火焰橙
    colors.operator_color = Color::RGB(255, 120, 90); // #FF785A 热橙

    colors.error = Color::RGB(255, 60, 60);    // #FF3C3C 烈焰红
    colors.warning = Color::RGB(255, 180, 60); // #FFB43C 警告黄
    colors.info = Color::RGB(180, 160, 140);   // #B4A08C 烟灰
    colors.success = Color::RGB(200, 140, 80); // #C88C50 金橙

    colors.dialog_bg = Color::RGB(35, 20, 18);         // #231412
    colors.dialog_fg = Color::RGB(235, 220, 210);      // #EBDCD2
    colors.dialog_title_bg = Color::RGB(18, 12, 12);   // #120C0C
    colors.dialog_title_fg = Color::RGB(255, 100, 60); // #FF643C 烈焰橙
    colors.dialog_border = Color::RGB(80, 50, 45);     // #50322D 熔岩边框

    return colors;
}

ThemeColors Theme::Arctic() {
    ThemeColors colors;
    // Arctic: 北极冰雪主题，纯白冰蓝基底，清新冷冽，适合明亮环境
    colors.background = Color::RGB(250, 252, 255);         // #FAFCFF 极雪白
    colors.foreground = Color::RGB(50, 60, 75);            // #323C4B 冰川灰
    colors.current_line = Color::RGB(240, 245, 252);       // #F0F5FC 冰蓝
    colors.selection = Color::RGB(200, 225, 245);          // #C8E1F5 冰川蓝
    colors.line_number = Color::RGB(150, 170, 195);        // #96AAC3 冰灰
    colors.line_number_current = Color::RGB(30, 130, 200); // #1E82C8 极冰蓝

    colors.statusbar_bg = Color::RGB(230, 242, 252); // #E6F2FC 冰蓝
    colors.statusbar_fg = Color::RGB(50, 60, 75);    // #323C4B 冰川灰

    colors.menubar_bg = Color::RGB(250, 252, 255); // #FAFCFF
    colors.menubar_fg = Color::RGB(50, 60, 75);    // #323C4B

    colors.helpbar_bg = Color::RGB(230, 242, 252);
    colors.helpbar_fg = Color::RGB(130, 150, 175); // #8296AF 冰灰蓝
    colors.helpbar_key = Color::RGB(0, 140, 200);  // #008CC8 冰川蓝

    // Arctic 语法高亮：冰蓝关键词/森林绿字符串/冰灰注释/橙红数字/深蓝函数
    colors.keyword = Color::RGB(0, 120, 180);         // #0078B4 冰川蓝
    colors.string = Color::RGB(60, 140, 80);          // #3C8C50 森林绿
    colors.comment = Color::RGB(150, 170, 190);       // #96AABE 冰灰
    colors.number = Color::RGB(220, 100, 50);         // #DC6432 极光橙
    colors.function = Color::RGB(40, 90, 150);        // #285A96 深海蓝
    colors.type = Color::RGB(100, 160, 200);          // #64A0C8 天蓝
    colors.operator_color = Color::RGB(80, 120, 160); // #5078A0 钢蓝

    colors.error = Color::RGB(220, 60, 60);    // #DC3C3C 冰红
    colors.warning = Color::RGB(240, 160, 40); // #F0A028 暖橙
    colors.info = Color::RGB(0, 140, 200);     // #008CC8 冰川蓝
    colors.success = Color::RGB(60, 160, 100); // #3CA064 冰绿

    colors.dialog_bg = Color::RGB(245, 250, 255);       // #F5FAFF
    colors.dialog_fg = Color::RGB(50, 60, 75);          // #323C4B
    colors.dialog_title_bg = Color::RGB(230, 242, 252); // #E6F2FC
    colors.dialog_title_fg = Color::RGB(30, 130, 200);  // #1E82C8 极冰蓝
    colors.dialog_border = Color::RGB(180, 205, 230);   // #B4CDE6 冰边框

    return colors;
}

ThemeColors Theme::NeonTokyo() {
    ThemeColors colors;
    // Neon Tokyo: 霓虹东京主题，深紫黑基底配合霓虹粉/青/绿，赛博朋克都市风
    colors.background = Color::RGB(20, 15, 30);            // #140F1E 霓虹黑紫
    colors.foreground = Color::RGB(235, 230, 245);         // #EBE6F5 霓虹白
    colors.current_line = Color::RGB(35, 25, 50);          // #231932 深霓虹
    colors.selection = Color::RGB(60, 40, 85);             // #3C2855 霓虹紫
    colors.line_number = Color::RGB(100, 80, 130);         // #645082 暗霓虹
    colors.line_number_current = Color::RGB(255, 50, 180); // #FF32B4 霓虹粉

    colors.statusbar_bg = Color::RGB(40, 25, 60);    // #28193C 深霓虹紫
    colors.statusbar_fg = Color::RGB(235, 230, 245); // #EBE6F5 霓虹白

    colors.menubar_bg = Color::RGB(20, 15, 30);    // #140F1E
    colors.menubar_fg = Color::RGB(235, 230, 245); // #EBE6F5

    colors.helpbar_bg = Color::RGB(40, 25, 60);
    colors.helpbar_fg = Color::RGB(130, 110, 160); // #826EA0 霓虹灰
    colors.helpbar_key = Color::RGB(0, 255, 200);  // #00FFC8 霓虹青

    // Neon Tokyo 语法高亮：霓虹粉关键词/酸橙绿字符串/霓虹灰注释/霓虹黄数字/霓虹青函数
    colors.keyword = Color::RGB(255, 50, 180);         // #FF32B4 霓虹粉
    colors.string = Color::RGB(180, 255, 80);          // #B4FF50 酸橙
    colors.comment = Color::RGB(100, 90, 120);         // #645A78 暗霓虹
    colors.number = Color::RGB(255, 220, 60);          // #FFDC3C 霓虹黄
    colors.function = Color::RGB(0, 240, 220);         // #00F0DC 霓虹青
    colors.type = Color::RGB(180, 120, 255);           // #B478FF 霓虹紫
    colors.operator_color = Color::RGB(255, 100, 200); // #FF64C8 热粉

    colors.error = Color::RGB(255, 60, 100);    // #FF3C64 霓虹红
    colors.warning = Color::RGB(255, 180, 60);  // #FFB43C 霓虹橙
    colors.info = Color::RGB(0, 240, 220);      // #00F0DC 霓虹青
    colors.success = Color::RGB(100, 255, 150); // #64FF96 霓虹绿

    colors.dialog_bg = Color::RGB(35, 25, 50);         // #231932
    colors.dialog_fg = Color::RGB(235, 230, 245);      // #EBE6F5
    colors.dialog_title_bg = Color::RGB(20, 15, 30);   // #140F1E
    colors.dialog_title_fg = Color::RGB(255, 50, 180); // #FF32B4 霓虹粉
    colors.dialog_border = Color::RGB(80, 60, 110);    // #503C6E 霓虹边框

    return colors;
}

ThemeColors Theme::TraeDark() {
    ThemeColors colors;
    // Trae Dark: Trae 暗色主题，深灰黑基底配合蓝/紫/绿，现代简约
    colors.background = Color::RGB(30, 30, 30);           // #1E1E1E 深灰黑
    colors.foreground = Color::RGB(220, 220, 220);        // #DCDCDC 浅灰白
    colors.current_line = Color::RGB(45, 45, 45);         // #2D2D2D 深灰
    colors.selection = Color::RGB(60, 60, 70);            // #3C3C46 选中灰
    colors.line_number = Color::RGB(120, 120, 120);       // #787878 灰
    colors.line_number_current = Color::RGB(0, 120, 215); // #0078D7 Trae 蓝

    colors.statusbar_bg = Color::RGB(40, 40, 40);    // #282828 状态栏灰
    colors.statusbar_fg = Color::RGB(220, 220, 220); // #DCDCDC 浅灰白

    colors.menubar_bg = Color::RGB(30, 30, 30);    // #1E1E1E
    colors.menubar_fg = Color::RGB(220, 220, 220); // #DCDCDC

    colors.helpbar_bg = Color::RGB(40, 40, 40);
    colors.helpbar_fg = Color::RGB(140, 140, 140); // #8C8C8C 灰
    colors.helpbar_key = Color::RGB(0, 150, 200);  // #0096C8 青

    // Trae Dark 语法高亮：蓝关键词/绿字符串/灰注释/橙数字/紫函数
    colors.keyword = Color::RGB(200, 100, 255);        // #C864FF 紫
    colors.string = Color::RGB(100, 200, 100);         // #64C864 绿
    colors.comment = Color::RGB(120, 120, 120);        // #787878 灰
    colors.number = Color::RGB(255, 150, 50);          // #FF9632 橙
    colors.function = Color::RGB(0, 120, 215);         // #0078D7 Trae 蓝
    colors.type = Color::RGB(80, 180, 220);            // #50B4DC 青
    colors.operator_color = Color::RGB(220, 220, 220); // #DCDCDC

    colors.error = Color::RGB(255, 80, 80);     // #FF5050 红
    colors.warning = Color::RGB(255, 180, 50);  // #FFB432 橙
    colors.info = Color::RGB(0, 120, 215);      // #0078D7 Trae 蓝
    colors.success = Color::RGB(100, 200, 100); // #64C864 绿

    colors.dialog_bg = Color::RGB(45, 45, 45);        // #2D2D2D
    colors.dialog_fg = Color::RGB(220, 220, 220);     // #DCDCDC
    colors.dialog_title_bg = Color::RGB(30, 30, 30);  // #1E1E1E
    colors.dialog_title_fg = Color::RGB(0, 120, 215); // #0078D7 Trae 蓝
    colors.dialog_border = Color::RGB(80, 80, 90);    // #50505A 边框灰

    return colors;
}

ThemeColors Theme::TraeDeepBlue() {
    ThemeColors colors;
    // Trae Deep Blue: Trae 深蓝主题，深蓝灰基底配合蓝/青/紫，科技感
    colors.background = Color::RGB(25, 35, 50);             // #192332 深蓝灰
    colors.foreground = Color::RGB(210, 220, 240);          // #D2DCF0 浅蓝白
    colors.current_line = Color::RGB(40, 55, 75);           // #28374B 深蓝
    colors.selection = Color::RGB(55, 75, 100);             // #374B64 选中蓝
    colors.line_number = Color::RGB(100, 120, 150);         // #647896 蓝灰
    colors.line_number_current = Color::RGB(100, 200, 255); // #64C8FF 亮蓝

    colors.statusbar_bg = Color::RGB(35, 50, 70);    // #233246 深蓝
    colors.statusbar_fg = Color::RGB(210, 220, 240); // #D2DCF0 浅蓝白

    colors.menubar_bg = Color::RGB(25, 35, 50);    // #192332
    colors.menubar_fg = Color::RGB(210, 220, 240); // #D2DCF0

    colors.helpbar_bg = Color::RGB(35, 50, 70);
    colors.helpbar_fg = Color::RGB(120, 140, 170); // #788CAA 蓝灰
    colors.helpbar_key = Color::RGB(80, 200, 255); // #50C8FF 亮青

    // Trae Deep Blue 语法高亮：紫关键词/青字符串/蓝灰注释/橙数字/蓝函数
    colors.keyword = Color::RGB(180, 140, 255);        // #B48CFF 紫
    colors.string = Color::RGB(80, 220, 200);          // #50DCC8 青
    colors.comment = Color::RGB(110, 130, 160);        // #6E82A0 蓝灰
    colors.number = Color::RGB(255, 180, 80);          // #FFB450 橙
    colors.function = Color::RGB(100, 180, 255);       // #64B4FF 蓝
    colors.type = Color::RGB(120, 200, 255);           // #78C8FF 亮蓝
    colors.operator_color = Color::RGB(210, 220, 240); // #D2DCF0

    colors.error = Color::RGB(255, 100, 100);  // #FF6464 红
    colors.warning = Color::RGB(255, 200, 80); // #FFC850 橙
    colors.info = Color::RGB(100, 180, 255);   // #64B4FF 蓝
    colors.success = Color::RGB(80, 220, 200); // #50DCC8 青

    colors.dialog_bg = Color::RGB(40, 55, 75);          // #28374B
    colors.dialog_fg = Color::RGB(210, 220, 240);       // #D2DCF0
    colors.dialog_title_bg = Color::RGB(25, 35, 50);    // #192332
    colors.dialog_title_fg = Color::RGB(100, 200, 255); // #64C8FF 亮蓝
    colors.dialog_border = Color::RGB(70, 90, 120);     // #465A78 蓝边框

    return colors;
}

ThemeColors Theme::Midnight() {
    ThemeColors colors;
    // Midnight: 午夜蓝主题，深邃神秘的蓝紫色调
    colors.background = Color::RGB(20, 25, 40);             // #141928 深夜蓝
    colors.foreground = Color::RGB(200, 205, 220);          // #C8CDE4 浅蓝白
    colors.current_line = Color::RGB(35, 45, 70);           // #232D46 深蓝
    colors.selection = Color::RGB(50, 65, 95);              // #32415F 选中蓝紫
    colors.line_number = Color::RGB(90, 100, 130);          // #5A6482 蓝灰
    colors.line_number_current = Color::RGB(180, 190, 230); // #B4BEE6 亮蓝白

    colors.statusbar_bg = Color::RGB(30, 38, 55);    // #1E2637 深蓝
    colors.statusbar_fg = Color::RGB(200, 205, 220); // #C8CDE4 浅蓝白

    colors.menubar_bg = Color::RGB(20, 25, 40);    // #141928
    colors.menubar_fg = Color::RGB(200, 205, 220); // #C8CDE4

    colors.helpbar_bg = Color::RGB(30, 38, 55);
    colors.helpbar_fg = Color::RGB(130, 145, 180);  // #8291B4 蓝紫灰
    colors.helpbar_key = Color::RGB(180, 160, 255); // #B4A0FF 亮紫

    // Midnight 语法高亮：紫关键词/青字符串/蓝灰注释/橙粉数字/蓝函数
    colors.keyword = Color::RGB(180, 140, 255);        // #B48CFF 紫
    colors.string = Color::RGB(100, 220, 200);         // #64DCC8 青绿
    colors.comment = Color::RGB(90, 100, 130);         // #5A6482 蓝灰
    colors.number = Color::RGB(255, 140, 180);         // #FF8CB4 粉橙
    colors.function = Color::RGB(120, 180, 255);       // #78B4FF 蓝
    colors.type = Color::RGB(160, 200, 255);           // #A0C8FF 浅蓝
    colors.operator_color = Color::RGB(200, 205, 220); // #C8CDE4

    colors.error = Color::RGB(255, 100, 130);   // #FF6482 红粉
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 橙黄
    colors.info = Color::RGB(120, 180, 255);    // #78B4FF 蓝
    colors.success = Color::RGB(100, 220, 200); // #64DCC8 青绿

    colors.dialog_bg = Color::RGB(35, 45, 70);          // #232D46
    colors.dialog_fg = Color::RGB(200, 205, 220);       // #C8CDE4
    colors.dialog_title_bg = Color::RGB(20, 25, 40);    // #141928
    colors.dialog_title_fg = Color::RGB(180, 160, 255); // #B4A0FF 亮紫
    colors.dialog_border = Color::RGB(70, 85, 115);     // #465573 蓝紫边框

    return colors;
}

ThemeColors Theme::FrancisBacon() {
    ThemeColors colors;
    // Francis Bacon: 弗朗西斯·培根主题，深红黑基底配合血色/暗金色调，灵感来自培根的黑暗绘画风格
    colors.background = Color::RGB(35, 25, 28);             // #23191C 深红褐色
    colors.foreground = Color::RGB(225, 210, 200);          // #E1D2C8 暖肉色
    colors.current_line = Color::RGB(50, 35, 38);           // #322326 红棕
    colors.selection = Color::RGB(70, 50, 53);              // #463235 深红棕
    colors.line_number = Color::RGB(130, 100, 90);          // #82645A 暗红棕
    colors.line_number_current = Color::RGB(230, 190, 170); // #E6BEAA 亮肉色

    colors.statusbar_bg = Color::RGB(45, 30, 33);    // #2D1E21 深红褐
    colors.statusbar_fg = Color::RGB(225, 210, 200); // #E1D2C8

    colors.menubar_bg = Color::RGB(35, 25, 28);    // #23191C
    colors.menubar_fg = Color::RGB(225, 210, 200); // #E1D2C8

    colors.helpbar_bg = Color::RGB(45, 30, 33);
    colors.helpbar_fg = Color::RGB(160, 130, 120); // #A08278 暗棕
    colors.helpbar_key = Color::RGB(210, 170, 90); // #D2AA5A 暗金

    // Bacon 语法高亮：血红关键词/暗金字符串/深棕注释/橙色数字/金棕函数
    colors.keyword = Color::RGB(190, 70, 60);        // #BE463C 血红
    colors.string = Color::RGB(210, 170, 90);        // #D2AA5A 暗金
    colors.comment = Color::RGB(100, 80, 75);        // #64504B 深棕
    colors.number = Color::RGB(230, 150, 100);       // #E69664 橙红
    colors.function = Color::RGB(190, 150, 80);      // #BE9650 金棕
    colors.type = Color::RGB(210, 130, 100);         // #D28264 橙红
    colors.operator_color = Color::RGB(190, 70, 60); // #BE463C 血红

    colors.error = Color::RGB(230, 60, 50);     // #E63C32 鲜红
    colors.warning = Color::RGB(230, 170, 70);  // #E6AA46 橙黄
    colors.info = Color::RGB(190, 150, 80);     // #BE9650 金棕
    colors.success = Color::RGB(150, 190, 110); // #96BE6E 橄榄绿

    colors.dialog_bg = Color::RGB(50, 35, 38);          // #322326
    colors.dialog_fg = Color::RGB(225, 210, 200);       // #E1D2C8
    colors.dialog_title_bg = Color::RGB(70, 45, 48);    // #462D30 深红
    colors.dialog_title_fg = Color::RGB(230, 190, 110); // #E6BE6E 金色
    colors.dialog_border = Color::RGB(130, 90, 80);     // #825A50 红棕边框

    return colors;
}

ThemeColors Theme::Moray() {
    ThemeColors colors;
    // Moray: 莫莱（海鳗）主题，深海鳗鱼色，深蓝绿基底配合荧光绿/橙色，灵感来自深海生物
    colors.background = Color::RGB(10, 40, 45);             // #0A282D 深海蓝绿
    colors.foreground = Color::RGB(210, 240, 230);          // #D2F0E6 浅海绿
    colors.current_line = Color::RGB(20, 55, 60);           // #14373C 深海绿
    colors.selection = Color::RGB(30, 75, 80);              // #1E4B50 选中绿
    colors.line_number = Color::RGB(90, 140, 130);          // #5A8C82 海绿
    colors.line_number_current = Color::RGB(190, 250, 230); // #BEFAE6 亮海绿

    colors.statusbar_bg = Color::RGB(15, 50, 55);    // #0F3237 深蓝绿
    colors.statusbar_fg = Color::RGB(210, 240, 230); // #D2F0E6

    colors.menubar_bg = Color::RGB(10, 40, 45);    // #0A282D
    colors.menubar_fg = Color::RGB(210, 240, 230); // #D2F0E6

    colors.helpbar_bg = Color::RGB(15, 50, 55);
    colors.helpbar_fg = Color::RGB(130, 190, 170); // #82BEAA 中海绿
    colors.helpbar_key = Color::RGB(255, 190, 90); // #FFBE5A 荧光橙

    // Moray 语法高亮：荧光绿关键词/橙字符串/深绿注释/亮橙数字/青函数
    colors.keyword = Color::RGB(80, 255, 170);        // #50FFAA 荧光绿
    colors.string = Color::RGB(255, 170, 90);         // #FFAA5A 橙
    colors.comment = Color::RGB(50, 120, 110);        // #32786E 深绿
    colors.number = Color::RGB(255, 210, 110);        // #FFD26E 亮橙
    colors.function = Color::RGB(70, 230, 210);       // #46E6D2 青绿
    colors.type = Color::RGB(90, 210, 230);           // #5AD2E6 青
    colors.operator_color = Color::RGB(80, 255, 170); // #50FFAA 荧光绿

    colors.error = Color::RGB(255, 90, 100);   // #FF5A64 红
    colors.warning = Color::RGB(255, 190, 70); // #FFBE46 橙
    colors.info = Color::RGB(70, 230, 210);    // #46E6D2 青绿
    colors.success = Color::RGB(80, 255, 170); // #50FFAA 荧光绿

    colors.dialog_bg = Color::RGB(20, 55, 60);         // #14373C
    colors.dialog_fg = Color::RGB(210, 240, 230);      // #D2F0E6
    colors.dialog_title_bg = Color::RGB(30, 75, 80);   // #1E4B50 深绿
    colors.dialog_title_fg = Color::RGB(80, 255, 170); // #50FFAA 荧光绿
    colors.dialog_border = Color::RGB(90, 140, 130);   // #5A8C82 海绿边框

    return colors;
}

ThemeColors Theme::VanGogh() {
    ThemeColors colors;
    // Van Gogh: 梵高主题，星空蓝黄基底配合漩涡状色彩，灵感来自《星夜》等名作
    colors.background = Color::RGB(15, 20, 40);             // #0F1428 星空深蓝
    colors.foreground = Color::RGB(255, 240, 190);          // #FFF0BE 星芒黄
    colors.current_line = Color::RGB(30, 40, 70);           // #1E2846 深蓝
    colors.selection = Color::RGB(45, 60, 95);              // #2D3C5F 选中蓝
    colors.line_number = Color::RGB(95, 115, 155);          // #5F739B 星夜蓝
    colors.line_number_current = Color::RGB(255, 230, 150); // #FFE696 亮星黄

    colors.statusbar_bg = Color::RGB(25, 35, 65);    // #192341 深蓝
    colors.statusbar_fg = Color::RGB(255, 240, 190); // #FFF0BE

    colors.menubar_bg = Color::RGB(15, 20, 40);    // #0F1428
    colors.menubar_fg = Color::RGB(255, 240, 190); // #FFF0BE

    colors.helpbar_bg = Color::RGB(25, 35, 65);
    colors.helpbar_fg = Color::RGB(175, 185, 215); // #AFB9D7 星夜灰蓝
    colors.helpbar_key = Color::RGB(255, 210, 90); // #FFD25A 向日葵黄

    // Van Gogh 语法高亮：向日葵黄关键词/橙红字符串/深蓝灰注释/金黄数字/漩涡蓝函数
    colors.keyword = Color::RGB(255, 230, 110);        // #FFE66E 向日葵黄
    colors.string = Color::RGB(255, 150, 90);          // #FF965A 橙红
    colors.comment = Color::RGB(65, 80, 115);          // #415073 深蓝灰
    colors.number = Color::RGB(255, 210, 70);          // #FFD246 金黄
    colors.function = Color::RGB(70, 175, 220);        // #46AFDC 漩涡蓝
    colors.type = Color::RGB(90, 195, 240);            // #5AC3F0 天蓝
    colors.operator_color = Color::RGB(255, 190, 110); // #FFBE6E 橙黄

    colors.error = Color::RGB(255, 110, 90);    // #FF6E5A 朱红
    colors.warning = Color::RGB(255, 210, 90);  // #FFD25A 橙黄
    colors.info = Color::RGB(70, 175, 220);     // #46AFDC 漩涡蓝
    colors.success = Color::RGB(130, 220, 150); // #82DC96 橄榄绿

    colors.dialog_bg = Color::RGB(30, 40, 70);          // #1E2846
    colors.dialog_fg = Color::RGB(255, 240, 190);       // #FFF0BE
    colors.dialog_title_bg = Color::RGB(45, 60, 95);    // #2D3C5F 深蓝
    colors.dialog_title_fg = Color::RGB(255, 230, 110); // #FFE66E 向日葵黄
    colors.dialog_border = Color::RGB(95, 115, 155);    // #5F739B 星夜蓝边框

    return colors;
}

ThemeColors Theme::Minecraft() {
    ThemeColors colors;
    // Minecraft: 我的世界主题，经典像素游戏配色
    // 灵感来源：草地绿、泥土棕、石头灰、钻石蓝、黄金黄
    colors.background = Color::RGB(28, 32, 34);     // #1C2022 深灰黑（基岩色）
    colors.foreground = Color::RGB(220, 220, 220);  // #DCDCDC 浅灰白
    colors.current_line = Color::RGB(42, 48, 51);   // #2A3033 深灰（选中行）
    colors.selection = Color::RGB(56, 64, 68);      // #384044 灰黑（选择区）
    colors.line_number = Color::RGB(102, 115, 119); // #667377 中灰（行号）
    colors.line_number_current = Color::RGB(85, 255, 85); // #55FF55 荧光绿（当前行号 - Creeper 绿）

    colors.statusbar_bg = Color::RGB(36, 42, 45);    // #242A2D 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(220, 220, 220); // #DCDCDC 浅灰白

    colors.menubar_bg = Color::RGB(28, 32, 34);    // #1C2022 深灰黑
    colors.menubar_fg = Color::RGB(220, 220, 220); // #DCDCDC 浅灰白

    colors.helpbar_bg = Color::RGB(36, 42, 45);
    colors.helpbar_fg = Color::RGB(170, 170, 170); // #AAAAAA 浅灰
    colors.helpbar_key = Color::RGB(85, 255, 85);  // #55FF55 Creeper 绿

    // Minecraft 语法高亮：绿关键词/棕字符串/灰注释/黄金数字/蓝函数/青类型
    colors.keyword = Color::RGB(85, 255, 85);          // #55FF55 Creeper 绿
    colors.string = Color::RGB(181, 127, 85);          // #B57F55 泥土棕
    colors.comment = Color::RGB(119, 119, 119);        // #777777 石头灰
    colors.number = Color::RGB(255, 255, 85);          // #FFFF55 黄金黄
    colors.function = Color::RGB(85, 170, 255);        // #55AAFF 钻石蓝
    colors.type = Color::RGB(85, 255, 255);            // #55FFFF 青金石蓝
    colors.operator_color = Color::RGB(220, 220, 220); // #DCDCDC 浅灰白

    colors.error = Color::RGB(255, 85, 85);    // #FF5555 红石红
    colors.warning = Color::RGB(255, 200, 85); // #FFC855 南瓜橙
    colors.info = Color::RGB(85, 170, 255);    // #55AAFF 钻石蓝
    colors.success = Color::RGB(85, 255, 85);  // #55FF55 Creeper 绿

    colors.dialog_bg = Color::RGB(42, 48, 51);          // #2A3033 深灰（对话框）
    colors.dialog_fg = Color::RGB(220, 220, 220);       // #DCDCDC 浅灰白
    colors.dialog_title_bg = Color::RGB(85, 170, 255);  // #55AAFF 钻石蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(119, 119, 119);   // #777777 石头灰边框

    return colors;
}

ThemeColors Theme::EVAUnit01() {
    ThemeColors colors;
    // EVA Unit-01: 初号机主题，紫/绿配色 - 碇真嗣座驾
    // 灵感来源：初号机紫色装甲 + 荧光绿条纹 + 暴走模式
    colors.background = Color::RGB(20, 20, 30);           // #14141E 深紫黑（宇宙深渊）
    colors.foreground = Color::RGB(200, 200, 210);        // #C8C8D2 浅灰紫
    colors.current_line = Color::RGB(35, 35, 50);         // #232332 深紫（选中行）
    colors.selection = Color::RGB(50, 50, 70);            // #323246 紫灰（选择区）
    colors.line_number = Color::RGB(100, 100, 120);       // #646478 中紫灰（行号）
    colors.line_number_current = Color::RGB(124, 255, 0); // #7CFF00 荧光绿（初号机绿）

    colors.statusbar_bg = Color::RGB(28, 28, 40);    // #1C1C28 深紫（状态栏）
    colors.statusbar_fg = Color::RGB(200, 200, 210); // #C8C8D2 浅灰紫

    colors.menubar_bg = Color::RGB(20, 20, 30);    // #14141E 深紫黑
    colors.menubar_fg = Color::RGB(200, 200, 210); // #C8C8D2 浅灰紫

    colors.helpbar_bg = Color::RGB(28, 28, 40);
    colors.helpbar_fg = Color::RGB(160, 160, 180); // #A0A0B4 浅紫灰
    colors.helpbar_key = Color::RGB(124, 255, 0);  // #7CFF00 初号机绿

    // 初号机配色：紫关键词/绿字符串/灰注释/橙数字/蓝函数/青类型
    colors.keyword = Color::RGB(140, 80, 200);         // #8C50C8 EVA 紫（初号机）
    colors.string = Color::RGB(124, 255, 0);           // #7CFF00 初号机绿
    colors.comment = Color::RGB(100, 100, 120);        // #646478 灰紫
    colors.number = Color::RGB(255, 140, 80);          // #FF8C50 警告橙
    colors.function = Color::RGB(80, 180, 255);        // #50B4FF 科技蓝
    colors.type = Color::RGB(80, 220, 200);            // #50DCC8 青蓝
    colors.operator_color = Color::RGB(200, 200, 210); // #C8C8D2 浅灰紫

    colors.error = Color::RGB(255, 80, 100);   // #FF5064 使徒红（暴走警报）
    colors.warning = Color::RGB(255, 180, 80); // #FFB450 橙黄（警戒）
    colors.info = Color::RGB(80, 180, 255);    // #50B4FF 蓝
    colors.success = Color::RGB(124, 255, 0);  // #7CFF00 初号机绿（同步成功）

    colors.dialog_bg = Color::RGB(35, 35, 50);          // #232332 深紫
    colors.dialog_fg = Color::RGB(200, 200, 210);       // #C8C8D2 浅灰紫
    colors.dialog_title_bg = Color::RGB(140, 80, 200);  // #8C50C8 EVA 紫
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(100, 100, 120);   // #646478 紫灰边框

    return colors;
}

ThemeColors Theme::EVAUnit02() {
    ThemeColors colors;
    // EVA Unit-02: 贰号机主题，红/橙配色 - 明日香座驾
    // 灵感来源：贰号机红色装甲 + 橙色条纹 + 野兽模式
    colors.background = Color::RGB(35, 15, 15);            // #230F0F 深红棕
    colors.foreground = Color::RGB(230, 220, 210);         // #E6DCD2 暖白
    colors.current_line = Color::RGB(55, 25, 25);          // #371919 深红（选中行）
    colors.selection = Color::RGB(75, 35, 35);             // #4B2323 红棕（选择区）
    colors.line_number = Color::RGB(140, 100, 90);         // #8C645A 红棕（行号）
    colors.line_number_current = Color::RGB(255, 200, 50); // #FFC832 金黄

    colors.statusbar_bg = Color::RGB(45, 20, 20);    // #2D1414 深红（状态栏）
    colors.statusbar_fg = Color::RGB(230, 220, 210); // #E6DCD2 暖白

    colors.menubar_bg = Color::RGB(35, 15, 15);    // #230F0F 深红棕
    colors.menubar_fg = Color::RGB(230, 220, 210); // #E6DCD2 暖白

    colors.helpbar_bg = Color::RGB(45, 20, 20);
    colors.helpbar_fg = Color::RGB(180, 140, 130); // #B48C82 浅红棕
    colors.helpbar_key = Color::RGB(255, 100, 80); // #FF6450 贰号机红

    // 贰号机配色：红关键词/橙字符串/棕注释/黄数字
    colors.keyword = Color::RGB(220, 60, 60);          // #DC3C3C 贰号机红
    colors.string = Color::RGB(255, 160, 60);          // #FFA03C 贰号机橙
    colors.comment = Color::RGB(140, 100, 90);         // #8C645A 红棕
    colors.number = Color::RGB(255, 220, 80);          // #FFDC50 亮黄
    colors.function = Color::RGB(255, 120, 80);        // #FF7850 珊瑚红
    colors.type = Color::RGB(255, 180, 100);           // #FFB464 浅橙
    colors.operator_color = Color::RGB(230, 220, 210); // #E6DCD2 暖白

    colors.error = Color::RGB(255, 50, 50);    // #FF3232 危险红
    colors.warning = Color::RGB(255, 180, 60); // #FFB43C 警告橙
    colors.info = Color::RGB(255, 140, 100);   // #FF8C64 信息珊瑚
    colors.success = Color::RGB(255, 200, 80); // #FFC850 成功黄

    colors.dialog_bg = Color::RGB(55, 25, 25);          // #371919 深红
    colors.dialog_fg = Color::RGB(230, 220, 210);       // #E6DCD2 暖白
    colors.dialog_title_bg = Color::RGB(200, 60, 60);   // #C83C3C 贰号机红
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(140, 100, 90);    // #8C645A 红棕边框

    return colors;
}

ThemeColors Theme::EVAUnit00() {
    ThemeColors colors;
    // EVA Unit-00: 零号机主题，黄/蓝配色 - 绫波丽座驾
    // 灵感来源：零号机黄色装甲 + 蓝色条纹 + 自爆模式
    colors.background = Color::RGB(30, 30, 25);            // #1E1E19 深黄棕
    colors.foreground = Color::RGB(220, 220, 210);         // #DCDCD2 米白
    colors.current_line = Color::RGB(50, 50, 40);          // #323228 深黄（选中行）
    colors.selection = Color::RGB(70, 70, 55);             // #464637 黄棕（选择区）
    colors.line_number = Color::RGB(130, 130, 110);        // #82826E 黄灰（行号）
    colors.line_number_current = Color::RGB(80, 180, 255); // #50B4FF 零号机蓝

    colors.statusbar_bg = Color::RGB(40, 40, 32);    // #282820 深黄（状态栏）
    colors.statusbar_fg = Color::RGB(220, 220, 210); // #DCDCD2 米白

    colors.menubar_bg = Color::RGB(30, 30, 25);    // #1E1E19 深黄棕
    colors.menubar_fg = Color::RGB(220, 220, 210); // #DCDCD2 米白

    colors.helpbar_bg = Color::RGB(40, 40, 32);
    colors.helpbar_fg = Color::RGB(160, 160, 140); // #A0A08C 浅黄灰
    colors.helpbar_key = Color::RGB(255, 220, 80); // #FFDC50 零号机黄

    // 零号机配色：黄关键词/蓝字符串/灰注释/橙数字
    colors.keyword = Color::RGB(255, 200, 60);         // #FFC83C 零号机黄
    colors.string = Color::RGB(80, 180, 255);          // #50B4FF 零号机蓝
    colors.comment = Color::RGB(130, 130, 110);        // #82826E 黄灰
    colors.number = Color::RGB(255, 160, 80);          // #FFA050 测试橙
    colors.function = Color::RGB(100, 200, 255);       // #64C8FF 天蓝
    colors.type = Color::RGB(255, 220, 120);           // #FFDC78 浅黄
    colors.operator_color = Color::RGB(220, 220, 210); // #DCDCD2 米白

    colors.error = Color::RGB(255, 80, 80);    // #FF5050 自爆红
    colors.warning = Color::RGB(255, 180, 80); // #FFB450 测试橙
    colors.info = Color::RGB(80, 180, 255);    // #50B4FF 零号机蓝
    colors.success = Color::RGB(255, 220, 80); // #FFDC50 零号机黄

    colors.dialog_bg = Color::RGB(50, 50, 40);         // #323228 深黄
    colors.dialog_fg = Color::RGB(220, 220, 210);      // #DCDCD2 米白
    colors.dialog_title_bg = Color::RGB(255, 200, 60); // #FFC83C 零号机黄
    colors.dialog_title_fg = Color::RGB(50, 50, 40);   // #323228 深黄文字
    colors.dialog_border = Color::RGB(130, 130, 110);  // #82826E 黄灰边框

    return colors;
}

ThemeColors Theme::EVAMark06() {
    ThemeColors colors;
    // EVA Mark.06: 六号机主题，蓝/白配色 - 渚薰座驾
    // 灵感来源：Mark.06 蓝色装甲 + 白色条纹 + 卡西乌斯之枪
    colors.background = Color::RGB(15, 25, 40);             // #0F1928 深海蓝
    colors.foreground = Color::RGB(220, 230, 240);          // #DCE6F0 冰白
    colors.current_line = Color::RGB(25, 40, 60);           // #19283C 深蓝（选中行）
    colors.selection = Color::RGB(35, 55, 80);              // #233750 蓝灰（选择区）
    colors.line_number = Color::RGB(100, 120, 150);         // #647896 蓝灰（行号）
    colors.line_number_current = Color::RGB(100, 220, 255); // #64DCFF 六号机青

    colors.statusbar_bg = Color::RGB(20, 35, 55);    // #142337 深海蓝（状态栏）
    colors.statusbar_fg = Color::RGB(220, 230, 240); // #DCE6F0 冰白

    colors.menubar_bg = Color::RGB(15, 25, 40);    // #0F1928 深海蓝
    colors.menubar_fg = Color::RGB(220, 230, 240); // #DCE6F0 冰白

    colors.helpbar_bg = Color::RGB(20, 35, 55);
    colors.helpbar_fg = Color::RGB(140, 160, 190); // #8CA0BE 浅蓝灰
    colors.helpbar_key = Color::RGB(80, 180, 255); // #50B4FF 六号机蓝

    // 六号机配色：蓝关键词/青字符串/灰注释/白数字
    colors.keyword = Color::RGB(80, 160, 240);         // #50A0F0 Mark.06蓝
    colors.string = Color::RGB(100, 220, 255);         // #64DCFF 青蓝
    colors.comment = Color::RGB(100, 120, 150);        // #647896 蓝灰
    colors.number = Color::RGB(240, 240, 255);         // #F0F0FF 纯白
    colors.function = Color::RGB(120, 200, 255);       // #78C8FF 天蓝
    colors.type = Color::RGB(150, 210, 255);           // #96D2FF 浅蓝
    colors.operator_color = Color::RGB(220, 230, 240); // #DCE6F0 冰白

    colors.error = Color::RGB(255, 100, 120);   // #FF6478 使徒红
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 警戒黄
    colors.info = Color::RGB(100, 220, 255);    // #64DCFF 六号机青
    colors.success = Color::RGB(80, 180, 255);  // #50B4FF 六号机蓝

    colors.dialog_bg = Color::RGB(25, 40, 60);          // #19283C 深蓝
    colors.dialog_fg = Color::RGB(220, 230, 240);       // #DCE6F0 冰白
    colors.dialog_title_bg = Color::RGB(80, 160, 240);  // #50A0F0 Mark.06蓝
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(100, 120, 150);   // #647896 蓝灰边框

    return colors;
}

ThemeColors Theme::EVATerminal() {
    ThemeColors colors;
    // EVA Terminal: NERV 终端机风格，绿黑配色 - MAGI 系统界面
    // 灵感来源：NERV 终端界面 + MAGI 系统 + 老式 CRT 显示器
    colors.background = Color::RGB(10, 15, 10);           // #0A0F0A 深绿黑
    colors.foreground = Color::RGB(180, 255, 180);        // #B4FFB4 荧光绿
    colors.current_line = Color::RGB(20, 30, 20);         // #141E14 深绿（选中行）
    colors.selection = Color::RGB(30, 45, 30);            // #1E2D1E 绿灰（选择区）
    colors.line_number = Color::RGB(60, 90, 60);          // #3C5A3C 中绿灰（行号）
    colors.line_number_current = Color::RGB(0, 255, 100); // #00FF64 NERV绿

    colors.statusbar_bg = Color::RGB(15, 25, 15);    // #0F190F 深绿（状态栏）
    colors.statusbar_fg = Color::RGB(180, 255, 180); // #B4FFB4 荧光绿

    colors.menubar_bg = Color::RGB(10, 15, 10);    // #0A0F0A 深绿黑
    colors.menubar_fg = Color::RGB(180, 255, 180); // #B4FFB4 荧光绿

    colors.helpbar_bg = Color::RGB(15, 25, 15);
    colors.helpbar_fg = Color::RGB(100, 150, 100); // #649664 浅绿灰
    colors.helpbar_key = Color::RGB(0, 255, 100);  // #00FF64 NERV绿

    // NERV 终端配色：绿关键词/亮绿字符串/暗绿注释/黄数字
    colors.keyword = Color::RGB(0, 255, 100);          // #00FF64 NERV绿
    colors.string = Color::RGB(150, 255, 150);         // #96FF96 亮绿
    colors.comment = Color::RGB(60, 90, 60);           // #3C5A3C 暗绿
    colors.number = Color::RGB(255, 255, 100);         // #FFFF64 警告黄
    colors.function = Color::RGB(100, 255, 200);       // #64FFC8 青绿
    colors.type = Color::RGB(180, 255, 120);           // #B4FF78 黄绿
    colors.operator_color = Color::RGB(180, 255, 180); // #B4FFB4 荧光绿

    colors.error = Color::RGB(255, 80, 80);    // #FF5050 警报红
    colors.warning = Color::RGB(255, 200, 80); // #FFC850 警告黄
    colors.info = Color::RGB(100, 255, 200);   // #64FFC8 青绿
    colors.success = Color::RGB(0, 255, 100);  // #00FF64 NERV绿

    colors.dialog_bg = Color::RGB(20, 30, 20);        // #141E14 深绿
    colors.dialog_fg = Color::RGB(180, 255, 180);     // #B4FFB4 荧光绿
    colors.dialog_title_bg = Color::RGB(0, 100, 40);  // #006428 NERV深绿
    colors.dialog_title_fg = Color::RGB(0, 255, 100); // #00FF64 NERV绿
    colors.dialog_border = Color::RGB(60, 90, 60);    // #3C5A3C 绿灰边框

    return colors;
}

ThemeColors Theme::IronMan() {
    ThemeColors colors;
    // Iron Man: 钢铁侠主题，红金配色 + 反应堆蓝光
    // 灵感来源：钢铁侠战甲红/金、方舟反应堆蓝、贾维斯界面
    colors.background = Color::RGB(15, 15, 20);           // #0F0F14 深黑（战甲内舱）
    colors.foreground = Color::RGB(220, 220, 230);        // #DCDCE6 银白（金属质感）
    colors.current_line = Color::RGB(30, 30, 40);         // #1E1E28 深灰（选中行）
    colors.selection = Color::RGB(45, 45, 60);            // #2D2D3C 灰黑（选择区）
    colors.line_number = Color::RGB(120, 120, 130);       // #787882 中灰（行号）
    colors.line_number_current = Color::RGB(255, 215, 0); // #FFD700 黄金（当前行号）

    colors.statusbar_bg = Color::RGB(25, 25, 35);    // #191923 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(220, 220, 230); // #DCDCE6 银白

    colors.menubar_bg = Color::RGB(15, 15, 20);    // #0F0F14 深黑
    colors.menubar_fg = Color::RGB(220, 220, 230); // #DCDCE6 银白

    colors.helpbar_bg = Color::RGB(25, 25, 35);
    colors.helpbar_fg = Color::RGB(180, 180, 190); // #B4B4BE 浅灰
    colors.helpbar_key = Color::RGB(255, 215, 0);  // #FFD700 黄金

    // Iron Man 语法高亮：红关键词/金字符串/灰注释/蓝数字/青函数
    colors.keyword = Color::RGB(220, 60, 70);          // #DC3C46 钢铁红（战甲主色）
    colors.string = Color::RGB(255, 215, 0);           // #FFD700 黄金金（战甲点缀）
    colors.comment = Color::RGB(100, 100, 110);        // #64646E 深灰（低调注释）
    colors.number = Color::RGB(0, 180, 255);           // #00B4FF 反应堆蓝（方舟能量）
    colors.function = Color::RGB(0, 220, 200);         // #00DCC8 青蓝（贾维斯界面）
    colors.type = Color::RGB(255, 180, 50);            // #FFB432 橙金（能量光束）
    colors.operator_color = Color::RGB(220, 220, 230); // #DCDCE6 银白

    colors.error = Color::RGB(255, 50, 60);    // #FF323C 危急红（战甲警报）
    colors.warning = Color::RGB(255, 180, 50); // #FFB432 橙黄（能量警告）
    colors.info = Color::RGB(0, 180, 255);     // #00B4FF 反应堆蓝（系统信息）
    colors.success = Color::RGB(0, 220, 200);  // #00DCC8 青蓝（系统正常）

    colors.dialog_bg = Color::RGB(30, 30, 40);        // #1E1E28 深灰（对话框）
    colors.dialog_fg = Color::RGB(220, 220, 230);     // #DCDCE6 银白
    colors.dialog_title_bg = Color::RGB(220, 60, 70); // #DC3C46 钢铁红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 215, 0); // #FFD700 黄金（标题文字）
    colors.dialog_border = Color::RGB(100, 100, 110); // #64646E 深灰边框

    return colors;
}

ThemeColors Theme::SpiderMan() {
    ThemeColors colors;
    // Spider-Man: 蜘蛛侠主题，红蓝配色 + 蛛网元素
    // 灵感来源：蜘蛛侠战衣红/蓝、蛛网白、蜘蛛标志黑
    colors.background = Color::RGB(10, 15, 30);             // #0A0F1E 深蓝黑（夜幕）
    colors.foreground = Color::RGB(220, 220, 230);          // #DCDCE6 白（蛛丝）
    colors.current_line = Color::RGB(25, 35, 60);           // #19233C 深蓝（选中行）
    colors.selection = Color::RGB(40, 55, 90);              // #28375A 蓝灰（选择区）
    colors.line_number = Color::RGB(100, 120, 150);         // #647896 中蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 255, 255); // #FFFFFF 纯白（蛛丝高光）

    colors.statusbar_bg = Color::RGB(20, 30, 55);    // #141E37 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(220, 220, 230); // #DCDCE6 白

    colors.menubar_bg = Color::RGB(10, 15, 30);    // #0A0F1E 深蓝黑
    colors.menubar_fg = Color::RGB(220, 220, 230); // #DCDCE6 白

    colors.helpbar_bg = Color::RGB(20, 30, 55);
    colors.helpbar_fg = Color::RGB(160, 180, 210); // #A0B4D2 浅蓝灰
    colors.helpbar_key = Color::RGB(255, 50, 50);  // #FF3232 蜘蛛红

    // Spider-Man 语法高亮：红关键词/蓝字符串/灰注释/白数字/青函数
    colors.keyword = Color::RGB(255, 50, 50);          // #FF3232 蜘蛛红（战衣主色）
    colors.string = Color::RGB(50, 120, 220);          // #3278DC 蜘蛛蓝（战衣配色）
    colors.comment = Color::RGB(90, 110, 140);         // #5A6E8C 蓝灰（低调注释）
    colors.number = Color::RGB(255, 255, 255);         // #FFFFFF 纯白（蛛丝）
    colors.function = Color::RGB(0, 200, 180);         // #00C8B4 青绿（蜘蛛感应）
    colors.type = Color::RGB(100, 160, 255);           // #64A0FF 浅蓝（夜空）
    colors.operator_color = Color::RGB(220, 220, 230); // #DCDCE6 白

    colors.error = Color::RGB(255, 60, 70);    // #FF3C46 危险红（蜘蛛警报）
    colors.warning = Color::RGB(255, 180, 50); // #FFB432 橙黄（警戒）
    colors.info = Color::RGB(50, 120, 220);    // #3278DC 蜘蛛蓝（信息）
    colors.success = Color::RGB(0, 200, 180);  // #00C8B4 青绿（感应正常）

    colors.dialog_bg = Color::RGB(25, 35, 60);          // #19233C 深蓝（对话框）
    colors.dialog_fg = Color::RGB(220, 220, 230);       // #DCDCE6 白
    colors.dialog_title_bg = Color::RGB(255, 50, 50);   // #FF3232 蜘蛛红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(50, 80, 130);     // #325082 深蓝边框

    return colors;
}

ThemeColors Theme::CaptainAmerica() {
    ThemeColors colors;
    // Captain America: 美国队长主题，红白蓝配色 + 星条旗元素
    // 灵感来源：美队战衣红/白/蓝、盾牌星徽、振银金属光泽
    colors.background = Color::RGB(15, 20, 40);            // #0F1428 深蓝（夜空）
    colors.foreground = Color::RGB(230, 230, 240);         // #E6E6F0 纯白（星星）
    colors.current_line = Color::RGB(30, 40, 70);          // #1E2846 深蓝（选中行）
    colors.selection = Color::RGB(45, 60, 100);            // #2D3C64 蓝灰（选择区）
    colors.line_number = Color::RGB(110, 130, 170);        // #6E82AA 中蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 200, 50); // #FFC832 金色（星徽）

    colors.statusbar_bg = Color::RGB(25, 35, 65);    // #192341 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(230, 230, 240); // #E6E6F0 纯白

    colors.menubar_bg = Color::RGB(15, 20, 40);    // #0F1428 深蓝
    colors.menubar_fg = Color::RGB(230, 230, 240); // #E6E6F0 纯白

    colors.helpbar_bg = Color::RGB(25, 35, 65);
    colors.helpbar_fg = Color::RGB(170, 185, 215); // #AAB9D7 浅蓝白
    colors.helpbar_key = Color::RGB(220, 60, 70);  // #DC3C46 美国红

    // Captain America 语法高亮：红关键词/蓝字符串/白数字/金函数
    colors.keyword = Color::RGB(220, 60, 70);          // #DC3C46 美国红（星条旗红）
    colors.string = Color::RGB(60, 100, 200);          // #3C64C8 美国蓝（星条旗蓝）
    colors.comment = Color::RGB(100, 120, 150);        // #647896 蓝灰（低调注释）
    colors.number = Color::RGB(255, 255, 255);         // #FFFFFF 纯白（星星）
    colors.function = Color::RGB(255, 200, 50);        // #FFC832 金色（星徽）
    colors.type = Color::RGB(100, 160, 230);           // #64A0E6 浅蓝（振银光泽）
    colors.operator_color = Color::RGB(230, 230, 240); // #E6E6F0 纯白

    colors.error = Color::RGB(255, 50, 60);    // #FF323C 危急红（战斗警报）
    colors.warning = Color::RGB(255, 180, 50); // #FFB432 橙黄（警戒）
    colors.info = Color::RGB(60, 100, 200);    // #3C64C8 美国蓝（信息）
    colors.success = Color::RGB(255, 200, 50); // #FFC832 金色（胜利）

    colors.dialog_bg = Color::RGB(30, 40, 70);          // #1E2846 深蓝（对话框）
    colors.dialog_fg = Color::RGB(230, 230, 240);       // #E6E6F0 纯白
    colors.dialog_title_bg = Color::RGB(60, 100, 200);  // #3C64C8 美国蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(220, 60, 70);     // #DC3C46 美国红边框

    return colors;
}

ThemeColors Theme::Hulk() {
    ThemeColors colors;
    // Hulk: 绿巨人主题，绿色皮肤 + 愤怒力量 + 紫色裤子
    // 灵感来源：浩克绿皮肤、伽马射线绿、愤怒红眼、紫裤子
    colors.background = Color::RGB(10, 20, 15);             // #0A140F 深绿黑（愤怒前夕）
    colors.foreground = Color::RGB(200, 230, 210);          // #C8E6D2 浅绿白（伽马光芒）
    colors.current_line = Color::RGB(20, 40, 30);           // #14281E 深绿（选中行）
    colors.selection = Color::RGB(30, 60, 45);              // #1E3C2D 绿灰（选择区）
    colors.line_number = Color::RGB(80, 130, 100);          // #508264 中绿灰（行号）
    colors.line_number_current = Color::RGB(100, 255, 100); // #64FF64 荧光绿（伽马射线）✨

    colors.statusbar_bg = Color::RGB(15, 35, 25);    // #0F2319 深绿（状态栏）
    colors.statusbar_fg = Color::RGB(200, 230, 210); // #C8E6D2 浅绿白

    colors.menubar_bg = Color::RGB(10, 20, 15);    // #0A140F 深绿黑
    colors.menubar_fg = Color::RGB(200, 230, 210); // #C8E6D2 浅绿白

    colors.helpbar_bg = Color::RGB(15, 35, 25);
    colors.helpbar_fg = Color::RGB(140, 190, 165);  // #8CBFA5 浅绿灰
    colors.helpbar_key = Color::RGB(100, 255, 100); // #64FF64 伽马绿

    // Hulk 语法高亮：绿关键词/紫字符串/灰注释/红数字/青函数
    colors.keyword = Color::RGB(50, 255, 100);         // #32FF64 浩克绿（皮肤主色）
    colors.string = Color::RGB(180, 100, 200);         // #B464C8 紫裤子（经典配色）
    colors.comment = Color::RGB(70, 110, 90);          // #466E5A 深绿灰（低调注释）
    colors.number = Color::RGB(255, 80, 100);          // #FF5064 愤怒红（绿巨人怒火）
    colors.function = Color::RGB(50, 220, 180);        // #32DCB4 青绿（伽马能量）
    colors.type = Color::RGB(100, 255, 150);           // #64FF96 亮绿（力量爆发）
    colors.operator_color = Color::RGB(200, 230, 210); // #C8E6D2 浅绿白

    colors.error = Color::RGB(255, 60, 80);     // #FF3C50 暴怒红（无法控制）
    colors.warning = Color::RGB(255, 180, 50);  // #FFB432 橙黄（警戒）
    colors.info = Color::RGB(50, 220, 180);     // #32DCB4 青绿（伽马信息）
    colors.success = Color::RGB(100, 255, 100); // #64FF64 伽马绿（力量正常）

    colors.dialog_bg = Color::RGB(20, 40, 30);         // #14281E 深绿（对话框）
    colors.dialog_fg = Color::RGB(200, 230, 210);      // #C8E6D2 浅绿白
    colors.dialog_title_bg = Color::RGB(50, 255, 100); // #32FF64 浩克绿（标题栏）
    colors.dialog_title_fg = Color::RGB(10, 20, 15);   // #0A140F 深绿黑（标题文字）
    colors.dialog_border = Color::RGB(70, 110, 90);    // #466E5A 深绿灰边框

    return colors;
}

ThemeColors Theme::Superman() {
    ThemeColors colors;
    // Superman: 超人主题，红蓝配色 + 黄金 S 标志
    // 灵感来源：超人战衣红/蓝、S 盾徽黄金、氪星科技蓝
    colors.background = Color::RGB(10, 15, 40);           // #0A0F28 深蓝黑（大都会夜空）
    colors.foreground = Color::RGB(220, 225, 240);        // #DCE1F0 白（云朵）
    colors.current_line = Color::RGB(20, 30, 65);         // #141E41 深蓝（选中行）
    colors.selection = Color::RGB(30, 45, 95);            // #1E2D5F 蓝灰（选择区）
    colors.line_number = Color::RGB(90, 110, 160);        // #5A6EA0 中蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 215, 0); // #FFD700 黄金（S 标志）✨

    colors.statusbar_bg = Color::RGB(15, 25, 60);    // #0F193C 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(220, 225, 240); // #DCE1F0 白

    colors.menubar_bg = Color::RGB(10, 15, 40);    // #0A0F28 深蓝黑
    colors.menubar_fg = Color::RGB(220, 225, 240); // #DCE1F0 白

    colors.helpbar_bg = Color::RGB(15, 25, 60);
    colors.helpbar_fg = Color::RGB(150, 165, 200); // #96A5C8 浅蓝灰
    colors.helpbar_key = Color::RGB(255, 50, 50);  // #FF3232 超人红

    // Superman 语法高亮：红关键词/蓝字符串/金数字/青函数
    colors.keyword = Color::RGB(255, 50, 50);          // #FF3232 超人红（披风主色）
    colors.string = Color::RGB(50, 100, 220);          // #3264DC 超人蓝（战衣配色）
    colors.comment = Color::RGB(80, 100, 140);         // #50648C 蓝灰（低调注释）
    colors.number = Color::RGB(255, 215, 0);           // #FFD700 黄金（S 盾徽）
    colors.function = Color::RGB(0, 200, 255);         // #00C8FF 青蓝（热视线）
    colors.type = Color::RGB(100, 160, 255);           // #64A0FF 浅蓝（氪星科技）
    colors.operator_color = Color::RGB(220, 225, 240); // #DCE1F0 白

    colors.error = Color::RGB(255, 40, 50);    // #FF2832 氪石红（致命危险）
    colors.warning = Color::RGB(255, 180, 50); // #FFB432 橙黄（警戒）
    colors.info = Color::RGB(50, 100, 220);    // #3264DC 超人蓝（信息）
    colors.success = Color::RGB(0, 200, 255);  // #00C8FF 青蓝（正义胜利）

    colors.dialog_bg = Color::RGB(20, 30, 65);        // #141E41 深蓝（对话框）
    colors.dialog_fg = Color::RGB(220, 225, 240);     // #DCE1F0 白
    colors.dialog_title_bg = Color::RGB(255, 50, 50); // #FF3232 超人红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 215, 0); // #FFD700 黄金（标题文字）
    colors.dialog_border = Color::RGB(50, 80, 140);   // #32508C 深蓝边框

    return colors;
}

ThemeColors Theme::Godfather() {
    ThemeColors colors;
    // The Godfather: 教父主题，黑金配色 + 意大利黑手党风格
    // 灵感来源：黑色西装、金色徽章、暗红葡萄酒、复古棕
    colors.background = Color::RGB(15, 15, 15);            // #0F0F0F 纯黑（黑色西装）
    colors.foreground = Color::RGB(220, 210, 190);         // #DCD2BE 米白（复古纸张）
    colors.current_line = Color::RGB(30, 30, 30);          // #1E1E1E 深灰（选中行）
    colors.selection = Color::RGB(45, 45, 45);             // #2D2D2D 灰黑（选择区）
    colors.line_number = Color::RGB(120, 110, 100);        // #786E64 棕灰（行号）
    colors.line_number_current = Color::RGB(212, 175, 55); // #D4AF37 黄金（权力徽章）✨

    colors.statusbar_bg = Color::RGB(25, 25, 25);    // #191919 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(220, 210, 190); // #DCD2BE 米白

    colors.menubar_bg = Color::RGB(15, 15, 15);    // #0F0F0F 纯黑
    colors.menubar_fg = Color::RGB(220, 210, 190); // #DCD2BE 米白

    colors.helpbar_bg = Color::RGB(25, 25, 25);
    colors.helpbar_fg = Color::RGB(170, 160, 145); // #AAA091 浅棕灰
    colors.helpbar_key = Color::RGB(212, 175, 55); // #D4AF37 黄金

    // Godfather 语法高亮：金关键词/红字符串/棕注释/白数字
    colors.keyword = Color::RGB(212, 175, 55);         // #D4AF37 黄金（权力与荣耀）
    colors.string = Color::RGB(180, 60, 70);           // #B43C46 暗红（葡萄酒/鲜血）
    colors.comment = Color::RGB(100, 90, 80);          // #645A50 深棕（低语密谋）
    colors.number = Color::RGB(240, 230, 220);         // #F0E6DC 米白（金钱数字）
    colors.function = Color::RGB(200, 170, 120);       // #C8AA78 古铜（家族徽章）
    colors.type = Color::RGB(190, 160, 130);           // #BEA082 棕褐（皮革质感）
    colors.operator_color = Color::RGB(220, 210, 190); // #DCD2BE 米白

    colors.error = Color::RGB(220, 50, 60);    // #DC323C 血红色（背叛代价）
    colors.warning = Color::RGB(212, 175, 55); // #D4AF37 黄金（警告）
    colors.info = Color::RGB(170, 160, 145);   // #AAA091 棕灰（信息）
    colors.success = Color::RGB(212, 175, 55); // #D4AF37 黄金（家族胜利）

    colors.dialog_bg = Color::RGB(30, 30, 30);         // #1E1E1E 深灰（对话框）
    colors.dialog_fg = Color::RGB(220, 210, 190);      // #DCD2BE 米白
    colors.dialog_title_bg = Color::RGB(212, 175, 55); // #D4AF37 黄金（标题栏 - 权力象征）
    colors.dialog_title_fg = Color::RGB(15, 15, 15);   // #0F0F0F 纯黑（标题文字）
    colors.dialog_border = Color::RGB(100, 90, 80);    // #645A50 深棕边框

    return colors;
}

ThemeColors Theme::RoboCop() {
    ThemeColors colors;
    // RoboCop: 机械战警主题，银色金属 + HUD 界面蓝 + 警示红
    // 灵感来源：ED-209 银色装甲、战术 HUD 蓝屏、红色扫描光、底特律警局
    colors.background = Color::RGB(20, 25, 30);           // #14191E 深灰蓝（金属内舱）
    colors.foreground = Color::RGB(200, 210, 220);        // #C8D2DC 银白（金属光泽）
    colors.current_line = Color::RGB(35, 45, 55);         // #232D37 深灰蓝（选中行）
    colors.selection = Color::RGB(50, 65, 80);            // #324150 蓝灰（选择区）
    colors.line_number = Color::RGB(110, 130, 150);       // #6E8296 中灰蓝（行号）
    colors.line_number_current = Color::RGB(0, 255, 255); // #00FFFF 青色（HUD 高亮）✨

    colors.statusbar_bg = Color::RGB(30, 40, 50);    // #1E2832 深灰蓝（状态栏）
    colors.statusbar_fg = Color::RGB(200, 210, 220); // #C8D2DC 银白

    colors.menubar_bg = Color::RGB(20, 25, 30);    // #14191E 深灰蓝
    colors.menubar_fg = Color::RGB(200, 210, 220); // #C8D2DC 银白

    colors.helpbar_bg = Color::RGB(30, 40, 50);
    colors.helpbar_fg = Color::RGB(150, 170, 190); // #96AABE 浅蓝灰
    colors.helpbar_key = Color::RGB(0, 255, 255);  // #00FFFF 青色（HUD）

    // RoboCop 语法高亮：银关键词/蓝字符串/灰注释/红数字/青函数
    colors.keyword = Color::RGB(190, 200, 210);        // #BEC8D2 银色（金属主色）
    colors.string = Color::RGB(50, 150, 220);          // #3296DC HUD 蓝（战术界面）
    colors.comment = Color::RGB(90, 110, 130);         // #5A6E82 灰蓝（低调注释）
    colors.number = Color::RGB(255, 80, 100);          // #FF5064 警示红（扫描光）
    colors.function = Color::RGB(0, 220, 200);         // #00DCC8 青绿（系统功能）
    colors.type = Color::RGB(100, 200, 255);           // #64C8FF 浅蓝（数据流）
    colors.operator_color = Color::RGB(200, 210, 220); // #C8D2DC 银白

    colors.error = Color::RGB(255, 60, 80);    // #FF3C50 危急红（系统故障）
    colors.warning = Color::RGB(255, 180, 50); // #FFB432 橙黄（警戒）
    colors.info = Color::RGB(0, 200, 255);     // #00C8FF 青蓝（HUD 信息）
    colors.success = Color::RGB(0, 255, 200);  // #00FFC8 青绿（系统正常）

    colors.dialog_bg = Color::RGB(35, 45, 55);          // #232D37 深灰蓝（对话框）
    colors.dialog_fg = Color::RGB(200, 210, 220);       // #C8D2DC 银白
    colors.dialog_title_bg = Color::RGB(0, 180, 255);   // #00B4FF HUD 蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(90, 110, 130);    // #5A6E82 灰蓝边框

    return colors;
}

ThemeColors Theme::Robot() {
    ThemeColors colors;
    // Robot: 机器人主题，金属银 + 科技蓝 + 电子绿
    // 灵感来源：未来机器人、AI 界面、量子计算、赛博科技
    colors.background = Color::RGB(15, 20, 25);           // #0F1419 深空黑（宇宙背景）
    colors.foreground = Color::RGB(220, 230, 240);        // #DCE6F0 亮银白（金属外壳）
    colors.current_line = Color::RGB(30, 40, 50);         // #1E2832 深空蓝（选中行）
    colors.selection = Color::RGB(45, 60, 75);            // #2D3C4B 科技蓝（选择区）
    colors.line_number = Color::RGB(100, 120, 140);       // #64788C 中灰蓝（行号）
    colors.line_number_current = Color::RGB(0, 255, 200); // #00FFC8 电子绿（AI 高亮）✨

    colors.statusbar_bg = Color::RGB(25, 35, 45);    // #19232D 深空蓝（状态栏）
    colors.statusbar_fg = Color::RGB(220, 230, 240); // #DCE6F0 亮银白

    colors.menubar_bg = Color::RGB(15, 20, 25);    // #0F1419 深空黑
    colors.menubar_fg = Color::RGB(220, 230, 240); // #DCE6F0 亮银白

    colors.helpbar_bg = Color::RGB(25, 35, 45);
    colors.helpbar_fg = Color::RGB(140, 160, 180); // #8CA0B4 科技灰
    colors.helpbar_key = Color::RGB(0, 255, 200);  // #00FFC8 电子绿

    // Robot 语法高亮：银关键词/青字符串/灰注释/橙数字/蓝函数
    colors.keyword = Color::RGB(200, 210, 220);        // #C8D2DC 亮银（金属主色）
    colors.string = Color::RGB(60, 200, 255);          // #3CC8FF 青蓝（数据流）
    colors.comment = Color::RGB(80, 100, 120);         // #506478 深灰（低调注释）
    colors.number = Color::RGB(255, 150, 80);          // #FF9650 橙色（能量指示）
    colors.function = Color::RGB(100, 180, 255);       // #64B4FF 天蓝（系统功能）
    colors.type = Color::RGB(120, 220, 255);           // #78DCFF 浅青（数据类型）
    colors.operator_color = Color::RGB(210, 220, 230); // #D2DCE6 亮银

    colors.error = Color::RGB(255, 80, 100);   // #FF5064 系统红（错误警报）
    colors.warning = Color::RGB(255, 200, 80); // #FFC850 警戒黄（警告）
    colors.info = Color::RGB(80, 200, 255);    // #50C8FF 青蓝（系统信息）
    colors.success = Color::RGB(0, 255, 180);  // #00FFB4 电子绿（运行正常）

    colors.dialog_bg = Color::RGB(30, 40, 50);          // #1E2832 深空蓝（对话框）
    colors.dialog_fg = Color::RGB(220, 230, 240);       // #DCE6F0 亮银白
    colors.dialog_title_bg = Color::RGB(0, 180, 255);   // #00B4FF 科技蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(80, 100, 120);    // #506478 深灰边框

    return colors;
}

ThemeColors Theme::Icy() {
    ThemeColors colors;
    // Icy: 冰蓝主题，灰色与浅蓝色配色，清新冷冽
    // 灵感来源：冬日霜雪、冰川蓝灰、冷冽空气、冰雪世界
    colors.background = Color::RGB(30, 35, 40);             // #1E2328 深灰（冬日夜空）
    colors.foreground = Color::RGB(210, 220, 230);          // #D2DCE6 浅灰白（霜雪）
    colors.current_line = Color::RGB(45, 52, 60);           // #2D343C 中灰（选中行）
    colors.selection = Color::RGB(60, 72, 85);              // #3C4855 蓝灰（选择区）
    colors.line_number = Color::RGB(120, 135, 150);         // #788796 中灰蓝（行号）
    colors.line_number_current = Color::RGB(180, 210, 235); // #B4D2EB 浅蓝（霜花高亮）✨

    colors.statusbar_bg = Color::RGB(40, 48, 56);    // #283038 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(210, 220, 230); // #D2DCE6 浅灰白

    colors.menubar_bg = Color::RGB(30, 35, 40);    // #1E2328 深灰
    colors.menubar_fg = Color::RGB(210, 220, 230); // #D2DCE6 浅灰白

    colors.helpbar_bg = Color::RGB(40, 48, 56);
    colors.helpbar_fg = Color::RGB(160, 175, 190);  // #A0AFBE 浅蓝灰
    colors.helpbar_key = Color::RGB(150, 190, 220); // #96BEDC 浅蓝

    // Icy 语法高亮：灰关键词/浅蓝字符串/深灰注释/蓝灰数字
    colors.keyword = Color::RGB(180, 190, 200);        // #B4BEC8 浅灰（霜雪主色）
    colors.string = Color::RGB(160, 200, 230);         // #A0C8E6 浅蓝（冰川蓝）
    colors.comment = Color::RGB(100, 115, 130);        // #647382 深灰蓝（低调注释）
    colors.number = Color::RGB(170, 195, 220);         // #AAC3DC 蓝灰（冰晶）
    colors.function = Color::RGB(140, 185, 220);       // #8CB9DC 天蓝（晴空）
    colors.type = Color::RGB(165, 205, 235);           // #A5CDEB 浅蓝（冰层）
    colors.operator_color = Color::RGB(200, 210, 220); // #C8D2DC 灰白

    colors.error = Color::RGB(220, 100, 120);   // #DC6478 冷红（冰霜警告）
    colors.warning = Color::RGB(230, 180, 100); // #E6B464 橙黄（警戒）
    colors.info = Color::RGB(140, 185, 220);    // #8CB9DC 天蓝（信息）
    colors.success = Color::RGB(130, 200, 180); // #82C8B4 青绿（正常）

    colors.dialog_bg = Color::RGB(45, 52, 60);          // #2D343C 中灰（对话框）
    colors.dialog_fg = Color::RGB(210, 220, 230);       // #D2DCE6 浅灰白
    colors.dialog_title_bg = Color::RGB(100, 130, 160); // #6482A0 灰蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(230, 240, 245); // #E6F0F5 极浅蓝白
    colors.dialog_border = Color::RGB(100, 115, 130);   // #647382 深灰蓝边框

    return colors;
}

ThemeColors Theme::PureBlue() {
    ThemeColors colors;
    // Pure Blue: 纯净蓝白主题，纯白 + 天蓝极简配色
    // 灵感来源：晴朗天空、蓝天白云、极简主义、清新纯净
    colors.background = Color::RGB(255, 255, 255);         // #FFFFFF 纯白（极简背景）
    colors.foreground = Color::RGB(40, 60, 90);            // #283C5A 深蓝灰（文字颜色）✨
    colors.current_line = Color::RGB(240, 248, 255);       // #F0F8FF 爱丽丝蓝（选中行）
    colors.selection = Color::RGB(220, 235, 250);          // #DCEBFA 浅天蓝（选择区）
    colors.line_number = Color::RGB(140, 160, 185);        // #8CA0B9 灰蓝（行号）
    colors.line_number_current = Color::RGB(60, 120, 190); // #3C78BE 天蓝（高亮）✨

    colors.statusbar_bg = Color::RGB(230, 242, 252); // #E6F2FC 极浅蓝（状态栏）
    colors.statusbar_fg = Color::RGB(40, 60, 90);    // #283C5A 深蓝灰

    colors.menubar_bg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.menubar_fg = Color::RGB(40, 60, 90);    // #283C5A 深蓝灰

    colors.helpbar_bg = Color::RGB(230, 242, 252);
    colors.helpbar_fg = Color::RGB(120, 145, 175); // #7891AF 中蓝灰
    colors.helpbar_key = Color::RGB(70, 130, 200); // #4682C8 天蓝

    // Pure Blue 语法高亮：纯蓝白配色，极简清新
    colors.keyword = Color::RGB(60, 110, 180);        // #3C6EB4 天蓝（关键词）💙
    colors.string = Color::RGB(80, 140, 200);         // #508CC8 浅天蓝（字符串）
    colors.comment = Color::RGB(150, 170, 195);       // #96AAC3 灰蓝（注释）
    colors.number = Color::RGB(50, 100, 160);         // #3264A0 深蓝（数字）
    colors.function = Color::RGB(70, 120, 190);       // #4678BE 晴空蓝（函数）
    colors.type = Color::RGB(90, 150, 210);           // #5A96D2 浅蓝（类型）
    colors.operator_color = Color::RGB(60, 110, 180); // #3C6EB4 天蓝

    colors.error = Color::RGB(200, 80, 100);   // #C85064 浅红（错误）
    colors.warning = Color::RGB(230, 160, 80); // #E6A050 琥珀（警告）
    colors.info = Color::RGB(70, 130, 200);    // #4682C8 天蓝（信息）
    colors.success = Color::RGB(90, 170, 150); // #5AAAA6 青绿（成功）

    colors.dialog_bg = Color::RGB(250, 252, 255);       // #FAFCFF 极浅蓝白（对话框）
    colors.dialog_fg = Color::RGB(40, 60, 90);          // #283C5A 深蓝灰
    colors.dialog_title_bg = Color::RGB(210, 230, 248); // #D2E6F8 浅天蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(50, 90, 140);   // #325A8C 中蓝
    colors.dialog_border = Color::RGB(180, 205, 230);   // #B4CDE6 浅蓝边框

    return colors;
}

ThemeColors Theme::Silver() {
    ThemeColors colors;
    // Silver: 银色主题，高端金属质感，银白 + 深灰 + 亮银点缀
    // 灵感来源：铂金金属、银器光泽、高端科技产品、液态金属
    colors.background = Color::RGB(25, 28, 32);             // #191C20 深炭灰（金属基底）
    colors.foreground = Color::RGB(220, 225, 230);          // #DCE1E6 银白（金属主色）✨
    colors.current_line = Color::RGB(40, 45, 50);           // #282D32 深灰（选中行）
    colors.selection = Color::RGB(55, 62, 70);              // #373E46 银灰（选择区）
    colors.line_number = Color::RGB(130, 140, 150);         // #828C96 中银灰（行号）
    colors.line_number_current = Color::RGB(240, 245, 250); // #F0F5FA 亮银（高亮）✨

    colors.statusbar_bg = Color::RGB(35, 40, 45);    // #23282D 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(220, 225, 230); // #DCE1E6 银白

    colors.menubar_bg = Color::RGB(25, 28, 32);    // #191C20 深炭灰
    colors.menubar_fg = Color::RGB(220, 225, 230); // #DCE1E6 银白

    colors.helpbar_bg = Color::RGB(35, 40, 45);
    colors.helpbar_fg = Color::RGB(180, 190, 200);  // #B4BEC8 浅银灰
    colors.helpbar_key = Color::RGB(230, 235, 240); // #E6EBF0 亮银

    // Silver 语法高亮：银白关键词/浅灰字符串/深灰注释/亮银数字
    colors.keyword = Color::RGB(210, 215, 220);        // #D2D7DC 银白（金属关键词）
    colors.string = Color::RGB(190, 200, 210);         // #BEC8D2 浅银灰（字符串）
    colors.comment = Color::RGB(110, 120, 130);        // #6E7882 深银灰（注释）
    colors.number = Color::RGB(240, 242, 245);         // #F0F2F5 亮银（数字）
    colors.function = Color::RGB(200, 210, 220);       // #C8D2DC 银灰（函数）
    colors.type = Color::RGB(215, 225, 235);           // #D7E1EB 浅银蓝（类型）
    colors.operator_color = Color::RGB(220, 225, 230); // #DCE1E6 银白

    colors.error = Color::RGB(230, 90, 100);    // #E65A64 金属红（错误）
    colors.warning = Color::RGB(240, 180, 80);  // #F0B450 金属橙（警告）
    colors.info = Color::RGB(180, 200, 220);    // #B4C8DC 银蓝（信息）
    colors.success = Color::RGB(160, 210, 190); // #A0D2BE 银绿（成功）

    colors.dialog_bg = Color::RGB(40, 45, 50);          // #282D32 深灰（对话框）
    colors.dialog_fg = Color::RGB(220, 225, 230);       // #DCE1E6 银白
    colors.dialog_title_bg = Color::RGB(70, 80, 90);    // #46505A 银灰（标题栏）
    colors.dialog_title_fg = Color::RGB(245, 248, 252); // #F5F8FC 极亮银
    colors.dialog_border = Color::RGB(110, 120, 130);   // #6E7882 深银灰边框

    return colors;
}

ThemeColors Theme::Egypt() {
    ThemeColors colors;
    // Egypt: 古埃及主题，沙漠金 + 尼罗河蓝 + 金字塔金 + 纸莎草棕
    // 灵感来源：金字塔、法老宝藏、尼罗河、沙漠落日、象形文字
    colors.background = Color::RGB(35, 28, 20);             // #231C14 深棕（沙漠夜空）
    colors.foreground = Color::RGB(235, 220, 180);          // #EBDCB4 沙金（沙漠之色）✨
    colors.current_line = Color::RGB(50, 40, 30);           // #32281E 深棕（选中行）
    colors.selection = Color::RGB(65, 52, 38);              // #413426 棕褐（选择区）
    colors.line_number = Color::RGB(180, 160, 120);         // #B4A078 沙褐（行号）
    colors.line_number_current = Color::RGB(255, 215, 100); // #FFD764 亮金（高亮）✨

    colors.statusbar_bg = Color::RGB(45, 35, 25);    // #2D2319 深棕（状态栏）
    colors.statusbar_fg = Color::RGB(235, 220, 180); // #EBDCB4 沙金

    colors.menubar_bg = Color::RGB(35, 28, 20);    // #231C14 深棕
    colors.menubar_fg = Color::RGB(235, 220, 180); // #EBDCB4 沙金

    colors.helpbar_bg = Color::RGB(45, 35, 25);
    colors.helpbar_fg = Color::RGB(200, 180, 150); // #C8B496 浅棕
    colors.helpbar_key = Color::RGB(255, 200, 80); // #FFC850 金色

    // Egypt 语法高亮：金关键词/青金石字符串/棕注释/宝石数字
    colors.keyword = Color::RGB(255, 200, 80);         // #FFC850 金色（法老权杖）
    colors.string = Color::RGB(80, 180, 200);          // #50B4C8 青金石（尼罗河蓝）
    colors.comment = Color::RGB(150, 130, 100);        // #968264 纸莎草棕（古老注释）
    colors.number = Color::RGB(255, 180, 100);         // #FFB464 琥珀金（数字）
    colors.function = Color::RGB(100, 200, 180);       // #64C8B4 绿松石（功能）
    colors.type = Color::RGB(220, 160, 100);           // #DCA064 黄褐（类型）
    colors.operator_color = Color::RGB(235, 220, 180); // #EBDCB4 沙金

    colors.error = Color::RGB(220, 80, 70);     // #DC5046 赤陶红（错误）
    colors.warning = Color::RGB(240, 160, 60);  // #F0A03C 沙漠橙（警告）
    colors.info = Color::RGB(100, 180, 220);    // #64B4DC 尼罗河蓝（信息）
    colors.success = Color::RGB(120, 200, 160); // #78C8A0 绿洲绿（成功）

    colors.dialog_bg = Color::RGB(50, 40, 30);          // #32281E 深棕（对话框）
    colors.dialog_fg = Color::RGB(235, 220, 180);       // #EBDCB4 沙金
    colors.dialog_title_bg = Color::RGB(180, 140, 60);  // #B48C3C 金字塔金（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 245, 220); // #FFF5DC 象牙白
    colors.dialog_border = Color::RGB(150, 130, 100);   // #968264 棕褐边框

    return colors;
}

ThemeColors Theme::Oasis() {
    ThemeColors colors;
    // Oasis: 绿洲主题，翡翠绿 + 金色 + 纯白 + 深蓝，中东绿洲风格
    // 灵感来源：沙漠绿洲、清真寺穹顶、伊斯兰几何图案、椰枣树、生命之水
    colors.background = Color::RGB(20, 25, 30);            // #14191E 深夜空（沙漠夜晚）
    colors.foreground = Color::RGB(240, 245, 240);         // #F0F5F0 月光白（绿洲月光）✨
    colors.current_line = Color::RGB(30, 45, 35);          // #1E2D23 翡翠暗绿（选中行）
    colors.selection = Color::RGB(40, 60, 45);             // #283C2D 深翡翠（选择区）
    colors.line_number = Color::RGB(120, 150, 130);        // #789682 灰绿（行号）
    colors.line_number_current = Color::RGB(255, 215, 80); // #FFD750 金色（高亮）✨

    colors.statusbar_bg = Color::RGB(25, 40, 30);    // #19281E 深翡翠（状态栏）
    colors.statusbar_fg = Color::RGB(240, 245, 240); // #F0F5F0 月光白

    colors.menubar_bg = Color::RGB(20, 25, 30);    // #14191E 深夜空
    colors.menubar_fg = Color::RGB(240, 245, 240); // #F0F5F0 月光白

    colors.helpbar_bg = Color::RGB(25, 40, 30);
    colors.helpbar_fg = Color::RGB(160, 185, 170); // #A0B9AA 浅灰绿
    colors.helpbar_key = Color::RGB(255, 200, 60); // #FFC83C 金色

    // Oasis 语法高亮：翡翠绿关键词/金色字符串/灰绿注释/蓝宝石数字
    colors.keyword = Color::RGB(80, 180, 120);         // #50B478 翡翠绿（生命之色）💚
    colors.string = Color::RGB(255, 210, 80);          // #FFD250 金色（沙漠黄金）✨
    colors.comment = Color::RGB(100, 130, 115);        // #648273 灰绿（低调注释）
    colors.number = Color::RGB(60, 140, 200);          // #3C8CC8 蓝宝石（数字）💎
    colors.function = Color::RGB(100, 200, 150);       // #64C896 浅翡翠（函数）
    colors.type = Color::RGB(255, 190, 70);            // #FFBE46 琥珀金（类型）
    colors.operator_color = Color::RGB(220, 235, 225); // #DCEBE1 浅绿白

    colors.error = Color::RGB(220, 80, 90);     // #DC505A 深红（错误）
    colors.warning = Color::RGB(255, 180, 60);  // #FFB43C 橙金（警告）
    colors.info = Color::RGB(80, 160, 200);     // #50A0C8 天蓝（信息）
    colors.success = Color::RGB(100, 200, 140); // #64C88C 翡翠绿（成功）

    colors.dialog_bg = Color::RGB(30, 45, 35);          // #1E2D23 深翡翠（对话框）
    colors.dialog_fg = Color::RGB(240, 245, 240);       // #F0F5F0 月光白
    colors.dialog_title_bg = Color::RGB(60, 140, 100);  // #3C8C64 翡翠绿（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(80, 120, 100);    // #507864 深绿边框

    return colors;
}

ThemeColors Theme::Aladdin() {
    ThemeColors colors;
    // Aladdin: 阿拉丁主题，神灯金 + 魔法紫 + 宝石蓝 + 深空黑，一千零一夜奇幻风格
    // 灵感来源：阿拉丁神灯、魔法飞毯、神秘洞穴、宝石金币、星空夜景
    colors.background = Color::RGB(15, 10, 25);            // #0F0A19 深空黑（神秘夜空）
    colors.foreground = Color::RGB(245, 240, 250);         // #F5F0FA 星芒白（夜空星辰）✨
    colors.current_line = Color::RGB(35, 25, 50);          // #231932 暗紫（选中行）
    colors.selection = Color::RGB(50, 35, 70);             // #322346 深紫（选择区）
    colors.line_number = Color::RGB(140, 120, 160);        // #8C78A0 灰紫（行号）
    colors.line_number_current = Color::RGB(255, 220, 60); // #FFDC3C 神灯金（高亮）✨

    colors.statusbar_bg = Color::RGB(30, 20, 45);    // #1E142D 深紫（状态栏）
    colors.statusbar_fg = Color::RGB(245, 240, 250); // #F5F0FA 星芒白

    colors.menubar_bg = Color::RGB(15, 10, 25);    // #0F0A19 深空黑
    colors.menubar_fg = Color::RGB(245, 240, 250); // #F5F0FA 星芒白

    colors.helpbar_bg = Color::RGB(30, 20, 45);
    colors.helpbar_fg = Color::RGB(170, 150, 190); // #AA96BE 浅紫
    colors.helpbar_key = Color::RGB(255, 200, 50); // #FFC832 金色

    // Aladdin 语法高亮：神灯金关键词/魔法紫字符串/宝石蓝数字
    colors.keyword = Color::RGB(255, 210, 60);         // #FFD23C 神灯金（魔法之光）💫
    colors.string = Color::RGB(180, 120, 220);         // #B478DC 魔法紫（神秘咒语）🔮
    colors.comment = Color::RGB(110, 100, 130);        // #6E6482 暗紫（低调注释）
    colors.number = Color::RGB(60, 180, 220);          // #3CB4DC 宝石蓝（数字）💎
    colors.function = Color::RGB(200, 140, 230);       // #C88CE6 紫水晶（函数）
    colors.type = Color::RGB(255, 190, 70);            // #FFBE46 琥珀金（类型）
    colors.operator_color = Color::RGB(230, 220, 240); // #E6DCF0 浅紫白

    colors.error = Color::RGB(230, 80, 100);    // #E65064 深红（错误）
    colors.warning = Color::RGB(255, 170, 50);  // #FFAA32 橙金（警告）
    colors.info = Color::RGB(80, 170, 220);     // #50AADC 天空蓝（信息）
    colors.success = Color::RGB(120, 200, 160); // #78C8A0 翡翠绿（成功）

    colors.dialog_bg = Color::RGB(35, 25, 50);          // #231932 暗紫（对话框）
    colors.dialog_fg = Color::RGB(245, 240, 250);       // #F5F0FA 星芒白
    colors.dialog_title_bg = Color::RGB(80, 50, 120);   // #503278 深紫（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(90, 70, 120);     // #5A4678 紫灰边框

    return colors;
}

ThemeColors Theme::Denmark() {
    ThemeColors colors;
    // Denmark: 丹麦主题，丹麦国旗红 + 纯白 + 北欧简约风 + 金色点缀
    // 灵感来源：丹麦国旗、安徒生童话、哥本哈根新港、北欧设计、乐高积木
    colors.background = Color::RGB(25, 30, 35);            // #191E23 深灰（北欧冬夜）
    colors.foreground = Color::RGB(240, 242, 245);         // #F0F2F5 纯白（北欧简约）✨
    colors.current_line = Color::RGB(40, 48, 56);          // #283038 深灰（选中行）
    colors.selection = Color::RGB(55, 68, 80);             // #374450 蓝灰（选择区）
    colors.line_number = Color::RGB(140, 155, 170);        // #8C9BAA 浅灰（行号）
    colors.line_number_current = Color::RGB(255, 200, 80); // #FFC850 金色（高亮）✨

    colors.statusbar_bg = Color::RGB(35, 43, 51);    // #232B33 深灰（状态栏）
    colors.statusbar_fg = Color::RGB(240, 242, 245); // #F0F2F5 纯白

    colors.menubar_bg = Color::RGB(25, 30, 35);    // #191E23 深灰
    colors.menubar_fg = Color::RGB(240, 242, 245); // #F0F2F5 纯白

    colors.helpbar_bg = Color::RGB(35, 43, 51);
    colors.helpbar_fg = Color::RGB(170, 185, 200); // #AAB9C8 浅灰
    colors.helpbar_key = Color::RGB(255, 190, 70); // #FFBE46 金色

    // Denmark 语法高亮：红关键词/白字符串/灰注释/金数字
    colors.keyword = Color::RGB(220, 60, 60);          // #DC3C3C 丹麦红（关键词）
    colors.string = Color::RGB(235, 238, 242);         // #EBEEF2 纯白（字符串）
    colors.comment = Color::RGB(120, 135, 150);        // #788796 中灰（注释）
    colors.number = Color::RGB(255, 190, 60);          // #FFBE3C 金黄（数字）
    colors.function = Color::RGB(100, 170, 220);       // #64AADC 北欧蓝（函数）
    colors.type = Color::RGB(240, 180, 70);            // #F0B446 琥珀金（类型）
    colors.operator_color = Color::RGB(240, 242, 245); // #F0F2F5 纯白

    colors.error = Color::RGB(230, 70, 70);     // #E64646 鲜红（错误）
    colors.warning = Color::RGB(255, 170, 60);  // #FFAA3C 橙黄（警告）
    colors.info = Color::RGB(90, 160, 210);     // #5AA0D2 北欧蓝（信息）
    colors.success = Color::RGB(110, 190, 150); // #6EBE96 北欧绿（成功）

    colors.dialog_bg = Color::RGB(40, 48, 56);          // #283038 深灰（对话框）
    colors.dialog_fg = Color::RGB(240, 242, 245);       // #F0F2F5 纯白
    colors.dialog_title_bg = Color::RGB(200, 50, 50);   // #C83232 丹麦红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(120, 135, 150);   // #788796 中灰边框

    return colors;
}

ThemeColors Theme::France() {
    ThemeColors colors;
    // France: 法兰西主题，法国国旗蓝 + 白 + 红 + 薰衣草紫 + 香槟金
    // 灵感来源：法国国旗、普罗旺斯薰衣草、香槟、埃菲尔铁塔、卢浮宫、法式优雅
    colors.background = Color::RGB(20, 25, 35);     // #141923 深海军蓝（法国深蓝）
    colors.foreground = Color::RGB(245, 245, 250);  // #F5F5FA 珍珠白（法式优雅）✨
    colors.current_line = Color::RGB(35, 45, 60);   // #232D3C 深蓝灰（选中行）
    colors.selection = Color::RGB(50, 65, 85);      // #324155 靛蓝（选择区）
    colors.line_number = Color::RGB(130, 145, 165); // #8291A5 灰蓝（行号）
    colors.line_number_current = Color::RGB(255, 215, 120); // #FFD778 香槟金（高亮）✨

    colors.statusbar_bg = Color::RGB(30, 40, 55);    // #1E2837 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(245, 245, 250); // #F5F5FA 珍珠白

    colors.menubar_bg = Color::RGB(20, 25, 35);    // #141923 深海军蓝
    colors.menubar_fg = Color::RGB(245, 245, 250); // #F5F5FA 珍珠白

    colors.helpbar_bg = Color::RGB(30, 40, 55);
    colors.helpbar_fg = Color::RGB(165, 180, 200); // #A5B4C8 浅蓝灰
    colors.helpbar_key = Color::RGB(255, 90, 100); // #FF5A64 法国红

    // France 语法高亮：蓝关键词/白字符串/灰注释/红数字/薰衣草紫函数
    colors.keyword = Color::RGB(80, 120, 200);         // #5078C8 法国蓝（关键词）🇫🇷
    colors.string = Color::RGB(240, 242, 248);         // #F0F2F8 珍珠白（字符串）
    colors.comment = Color::RGB(110, 125, 145);        // #6E7D91 灰蓝（注释）
    colors.number = Color::RGB(230, 80, 90);           // #E6505A 法国红（数字）
    colors.function = Color::RGB(180, 140, 220);       // #B48CDC 薰衣草紫（函数）🌿
    colors.type = Color::RGB(255, 200, 100);           // #FFC864 香槟金（类型）🥂
    colors.operator_color = Color::RGB(245, 245, 250); // #F5F5FA 珍珠白

    colors.error = Color::RGB(240, 70, 80);     // #F04650 鲜红（错误）
    colors.warning = Color::RGB(255, 180, 70);  // #FFB446 琥珀金（警告）
    colors.info = Color::RGB(90, 140, 210);     // #5A8CD2 法国蓝（信息）
    colors.success = Color::RGB(130, 200, 160); // #82C8A0 薄荷绿（成功）

    colors.dialog_bg = Color::RGB(35, 45, 60);          // #232D3C 深蓝灰（对话框）
    colors.dialog_fg = Color::RGB(245, 245, 250);       // #F5F5FA 珍珠白
    colors.dialog_title_bg = Color::RGB(200, 70, 80);   // #C84650 法国红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(100, 120, 150);   // #647896 蓝灰边框

    return colors;
}

ThemeColors Theme::Heaven() {
    ThemeColors colors;
    // Heaven: 天堂主题，天蓝 + 白云 + 阳光金 + 天使白
    // 灵感来源：天堂、云朵、阳光、天使、神圣光芒
    colors.background = Color::RGB(25, 35, 55);     // #192337 深邃天蓝（天堂背景）
    colors.foreground = Color::RGB(245, 248, 255);  // #F5F8FF 天使白（纯净文字）✨
    colors.current_line = Color::RGB(40, 55, 85);   // #283755 柔和天蓝（选中行）
    colors.selection = Color::RGB(55, 75, 115);     // #374B73 云朵蓝（选择区）
    colors.line_number = Color::RGB(130, 150, 185); // #8296B9 淡蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 220, 120); // #FFDC78 阳光金（高亮）✨

    colors.statusbar_bg = Color::RGB(35, 50, 80);    // #233250 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(245, 248, 255); // #F5F8FF 天使白

    colors.menubar_bg = Color::RGB(25, 35, 55);    // #192337 深邃天蓝
    colors.menubar_fg = Color::RGB(245, 248, 255); // #F5F8FF 天使白

    colors.helpbar_bg = Color::RGB(35, 50, 80);
    colors.helpbar_fg = Color::RGB(160, 180, 210);  // #A0B4D2 云朵灰
    colors.helpbar_key = Color::RGB(120, 200, 255); // #78C8FF 天空蓝

    // Heaven 语法高亮：白金关键词/天蓝字符串/灰蓝注释/阳光金数字
    colors.keyword = Color::RGB(255, 240, 200);        // #FFF0C8 阳光白金（关键词）
    colors.string = Color::RGB(150, 210, 255);         // #96D2FF 天蓝（字符串）
    colors.comment = Color::RGB(120, 140, 170);        // #788CAA 云朵灰（注释）
    colors.number = Color::RGB(255, 200, 100);         // #FFC864 阳光金（数字）
    colors.function = Color::RGB(140, 230, 200);       // #8CE6C8 薄荷绿（函数）
    colors.type = Color::RGB(180, 160, 255);           // #B4A0FF 薰衣草紫（类型）
    colors.operator_color = Color::RGB(255, 240, 200); // #FFF0C8 阳光白金

    colors.error = Color::RGB(255, 120, 140);   // #FF788C 玫瑰红（错误）
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 阳光金（警告）
    colors.info = Color::RGB(120, 200, 255);    // #78C8FF 天空蓝（信息）
    colors.success = Color::RGB(140, 230, 180); // #8CE6B4 天使绿（成功）

    colors.dialog_bg = Color::RGB(40, 55, 85);          // #283755 柔和天蓝（对话框）
    colors.dialog_fg = Color::RGB(245, 248, 255);       // #F5F8FF 天使白
    colors.dialog_title_bg = Color::RGB(70, 110, 160);  // #466EA0 天堂蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(120, 150, 190);   // #7896BE 云朵蓝边框

    return colors;
}

ThemeColors Theme::Hell() {
    ThemeColors colors;
    // Hell: 地狱主题，熔岩红 + 硫磺黄 + 炭黑 + 恶魔橙
    // 灵感来源：地狱之火、熔岩、硫磺、恶魔、暗黑深渊
    colors.background = Color::RGB(15, 10, 20);    // #0F0A14 深渊黑（地狱深渊）
    colors.foreground = Color::RGB(220, 210, 200); // #DCD2C8 灰烬白（文字颜色）🔥
    colors.current_line = Color::RGB(40, 15, 20);  // #280F14 熔岩暗红（选中行）
    colors.selection = Color::RGB(65, 25, 30);     // #41191E 深红（选择区）
    colors.line_number = Color::RGB(100, 60, 50);  // #643C32 炭灰（行号）
    colors.line_number_current = Color::RGB(255, 100, 50); // #FF6432 熔岩橙（高亮）🔥

    colors.statusbar_bg = Color::RGB(30, 10, 15);    // #1E0A0F 深红黑（状态栏）
    colors.statusbar_fg = Color::RGB(220, 210, 200); // #DCD2C8 灰烬白

    colors.menubar_bg = Color::RGB(15, 10, 20);    // #0F0A14 深渊黑
    colors.menubar_fg = Color::RGB(220, 210, 200); // #DCD2C8 灰烬白

    colors.helpbar_bg = Color::RGB(30, 10, 15);
    colors.helpbar_fg = Color::RGB(160, 100, 80);  // #A06450 焦土灰
    colors.helpbar_key = Color::RGB(255, 120, 60); // #FF783C 熔岩橙

    // Hell 语法高亮：熔岩红关键词/硫磺黄字符串/炭灰注释/恶魔橙数字
    colors.keyword = Color::RGB(255, 80, 60);         // #FF503C 熔岩红（关键词）🔥
    colors.string = Color::RGB(255, 200, 80);         // #FFC850 硫磺黄（字符串）
    colors.comment = Color::RGB(80, 60, 55);          // #503C37 炭黑（注释）
    colors.number = Color::RGB(255, 140, 60);         // #FF8C3C 恶魔橙（数字）
    colors.function = Color::RGB(255, 100, 80);       // #FF6450 火焰红（函数）
    colors.type = Color::RGB(200, 80, 120);           // #C85078 暗紫红（类型）
    colors.operator_color = Color::RGB(255, 120, 80); // #FF7850 熔岩橙

    colors.error = Color::RGB(255, 60, 80);     // #FF3C50 血红色（错误）
    colors.warning = Color::RGB(255, 180, 60);  // #FFB43C 硫磺黄（警告）
    colors.info = Color::RGB(255, 140, 100);    // #FF8C64 火焰橙（信息）
    colors.success = Color::RGB(180, 220, 100); // #B4DC64 毒液绿（成功）

    colors.dialog_bg = Color::RGB(40, 15, 20);          // #280F14 熔岩暗红（对话框）
    colors.dialog_fg = Color::RGB(220, 210, 200);       // #DCD2C8 灰烬白
    colors.dialog_title_bg = Color::RGB(100, 30, 40);   // #641E28 地狱红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 200, 150); // #FFC896 火焰金
    colors.dialog_border = Color::RGB(150, 50, 60);     // #96323C 熔岩边框

    return colors;
}

ThemeColors Theme::Antarctica() {
    ThemeColors colors;
    // Antarctica: 南极主题，冰雪白 + 冰川蓝 + 极光绿 + 企鹅黑
    // 灵感来源：南极冰川、极光、企鹅、科考站、极夜星空
    colors.background = Color::RGB(15, 20, 30);     // #0F141E 极夜蓝（南极夜空）
    colors.foreground = Color::RGB(245, 250, 255);  // #F5FAFF 冰雪白（纯净冰雪）✨
    colors.current_line = Color::RGB(30, 40, 55);   // #1E2837 深蓝（选中行）
    colors.selection = Color::RGB(45, 60, 80);      // #2D3C50 冰蓝（选择区）
    colors.line_number = Color::RGB(150, 170, 195); // #96AAC3 冰灰（行号）
    colors.line_number_current = Color::RGB(120, 255, 180); // #78FFB4 极光绿（高亮）✨

    colors.statusbar_bg = Color::RGB(25, 35, 50);    // #192332 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(245, 250, 255); // #F5FAFF 冰雪白

    colors.menubar_bg = Color::RGB(15, 20, 30);    // #0F141E 极夜蓝
    colors.menubar_fg = Color::RGB(245, 250, 255); // #F5FAFF 冰雪白

    colors.helpbar_bg = Color::RGB(25, 35, 50);
    colors.helpbar_fg = Color::RGB(180, 200, 220);  // #B4C8DC 浅蓝
    colors.helpbar_key = Color::RGB(100, 240, 160); // #64F0A0 极光绿

    // Antarctica 语法高亮：白关键词/冰蓝字符串/灰注释/极光绿数字
    colors.keyword = Color::RGB(230, 240, 250);        // #E6F0FA 冰雪白（关键词）
    colors.string = Color::RGB(160, 200, 240);         // #A0C8F0 冰川蓝（字符串）
    colors.comment = Color::RGB(130, 150, 170);        // #8296AA 冰灰（注释）
    colors.number = Color::RGB(140, 250, 200);         // #8CFAC8 冰晶绿（数字）
    colors.function = Color::RGB(100, 230, 170);       // #64E6AA 极光绿（函数）
    colors.type = Color::RGB(150, 220, 255);           // #96DCFF 天空蓝（类型）
    colors.operator_color = Color::RGB(245, 250, 255); // #F5FAFF 冰雪白

    colors.error = Color::RGB(240, 100, 120);   // #F06478 珊瑚红（错误）
    colors.warning = Color::RGB(255, 200, 100); // #FFC864 企鹅黄（警告）
    colors.info = Color::RGB(120, 200, 255);    // #78C8FF 冰川蓝（信息）
    colors.success = Color::RGB(80, 240, 160);  // #50F0A0 极光绿（成功）

    colors.dialog_bg = Color::RGB(30, 40, 55);          // #1E2837 深蓝（对话框）
    colors.dialog_fg = Color::RGB(245, 250, 255);       // #F5FAFF 冰雪白
    colors.dialog_title_bg = Color::RGB(60, 100, 150);  // #3C6496 冰川蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(100, 130, 160);   // #6482A0 冰蓝边框

    return colors;
}

ThemeColors Theme::DraculaSoft() {
    ThemeColors colors;
    // Dracula Soft: 德古拉柔和版，降低对比度更护眼
    // 灵感来源：经典 Dracula 配色但降低饱和度，适合长时间编码
    colors.background = Color::RGB(45, 47, 60);     // #2D2F3C 柔和紫灰（比原版稍浅）
    colors.foreground = Color::RGB(230, 230, 225);  // #E6E6E1 柔光白（降低刺眼感）✨
    colors.current_line = Color::RGB(57, 59, 75);   // #393B4B 柔和选中（更低调）
    colors.selection = Color::RGB(73, 76, 95);      // #494C5F 柔和选择区
    colors.line_number = Color::RGB(103, 119, 169); // #6777A9 柔和蓝灰（行号）
    colors.line_number_current = Color::RGB(230, 230, 225); // #E6E6E1 柔光白（高亮）✨

    colors.statusbar_bg = Color::RGB(73, 76, 95);    // #494C5F 柔和紫灰（状态栏）
    colors.statusbar_fg = Color::RGB(230, 230, 225); // #E6E6E1 柔光白

    colors.menubar_bg = Color::RGB(38, 40, 52);    // #262834 柔和深紫灰
    colors.menubar_fg = Color::RGB(230, 230, 225); // #E6E6E1 柔光白

    colors.helpbar_bg = Color::RGB(73, 76, 95);
    colors.helpbar_fg = Color::RGB(174, 135, 234); // #AE87EA 柔和紫
    colors.helpbar_key = Color::RGB(92, 235, 135); // #5CEB87 柔和绿

    // Dracula Soft 语法高亮：降低饱和度的经典色
    colors.keyword = Color::RGB(240, 110, 185);        // #F06EB9 柔和粉（降低刺眼）
    colors.string = Color::RGB(226, 235, 130);         // #E2EB82 柔和黄
    colors.comment = Color::RGB(103, 119, 169);        // #6777A9 柔和蓝灰
    colors.number = Color::RGB(174, 135, 234);         // #AE87EA 柔和紫
    colors.function = Color::RGB(92, 235, 135);        // #5CEB87 柔和绿
    colors.type = Color::RGB(139, 218, 238);           // #8BDAEE 柔和青
    colors.operator_color = Color::RGB(240, 110, 185); // 柔和粉

    colors.error = Color::RGB(240, 95, 95);     // #F05F5F 柔和红
    colors.warning = Color::RGB(255, 170, 110); // #FFAA6E 柔和橙
    colors.info = Color::RGB(139, 218, 238);    // #8BDAEE 柔和青
    colors.success = Color::RGB(92, 235, 135);  // #5CEB87 柔和绿

    colors.dialog_bg = Color::RGB(57, 59, 75);          // #393B4B 柔和紫灰（对话框）
    colors.dialog_fg = Color::RGB(230, 230, 225);       // #E6E6E1 柔光白
    colors.dialog_title_bg = Color::RGB(73, 76, 95);    // #494C5F 柔和紫灰（标题栏）
    colors.dialog_title_fg = Color::RGB(230, 230, 225); // #E6E6E1 柔光白
    colors.dialog_border = Color::RGB(103, 119, 169);   // #6777A9 柔和蓝灰边框

    return colors;
}

ThemeColors Theme::DraculaLight() {
    ThemeColors colors;
    // Dracula Light: 德古拉浅色版，明亮办公环境
    // 灵感来源：经典 Dracula 配色的明亮版本，适合白天办公使用
    colors.background = Color::RGB(245, 245, 250);         // #F5F5FA 亮白紫（明亮背景）✨
    colors.foreground = Color::RGB(55, 57, 70);            // #373946 深紫灰（深色文字）
    colors.current_line = Color::RGB(230, 230, 240);       // #E6E6F0 浅紫白（选中行）
    colors.selection = Color::RGB(215, 215, 230);          // #D7D7E6 浅紫（选择区）
    colors.line_number = Color::RGB(135, 140, 165);        // #878CA5 中紫灰（行号）
    colors.line_number_current = Color::RGB(88, 101, 242); // #5865F2 亮紫（高亮）✨

    colors.statusbar_bg = Color::RGB(200, 200, 215); // #C8C8D7 浅紫灰（状态栏）
    colors.statusbar_fg = Color::RGB(55, 57, 70);    // #373946 深紫灰

    colors.menubar_bg = Color::RGB(235, 235, 245); // #EBEBF5 极浅紫
    colors.menubar_fg = Color::RGB(55, 57, 70);    // #373946 深紫灰

    colors.helpbar_bg = Color::RGB(200, 200, 215);
    colors.helpbar_fg = Color::RGB(100, 90, 130);  // #645A82 中紫
    colors.helpbar_key = Color::RGB(50, 180, 100); // #32B464 鲜绿

    // Dracula Light 语法高亮：明亮背景下的深色高饱和色
    colors.keyword = Color::RGB(200, 60, 150);        // #C83C96 深粉（关键词）
    colors.string = Color::RGB(180, 180, 60);         // #B4B43C 深黄（字符串）
    colors.comment = Color::RGB(110, 120, 150);       // #6E7896 蓝灰（注释）
    colors.number = Color::RGB(120, 80, 200);         // #7850C8 深紫（数字）
    colors.function = Color::RGB(40, 180, 100);       // #28B464 鲜绿（函数）
    colors.type = Color::RGB(60, 160, 200);           // #3CA0C8 深青（类型）
    colors.operator_color = Color::RGB(200, 60, 150); // 深粉

    colors.error = Color::RGB(220, 60, 70);    // #DC3C46 深红（错误）
    colors.warning = Color::RGB(220, 140, 60); // #DC8C3C 深橙（警告）
    colors.info = Color::RGB(60, 160, 200);    // #3CA0C8 深青（信息）
    colors.success = Color::RGB(40, 180, 100); // #28B464 鲜绿（成功）

    colors.dialog_bg = Color::RGB(230, 230, 240);       // #E6E6F0 浅紫白（对话框）
    colors.dialog_fg = Color::RGB(55, 57, 70);          // #373946 深紫灰
    colors.dialog_title_bg = Color::RGB(200, 200, 215); // #C8C8D7 浅紫灰（标题栏）
    colors.dialog_title_fg = Color::RGB(55, 57, 70);    // #373946 深紫灰
    colors.dialog_border = Color::RGB(160, 165, 190);   // #A0A5BE 浅紫边框

    return colors;
}

ThemeColors Theme::DraculaNeon() {
    ThemeColors colors;
    // Dracula Neon: 德古拉霓虹版，增强霓虹灯效果
    // 灵感来源：赛博朋克霓虹灯、夜店灯光、电子招牌、荧光效果
    colors.background = Color::RGB(35, 37, 50);     // #232532 深紫黑（霓虹背景）
    colors.foreground = Color::RGB(255, 255, 240);  // #FFFFF0 荧光白（高亮文字）✨
    colors.current_line = Color::RGB(50, 52, 68);   // #323444 霓虹紫（选中行）
    colors.selection = Color::RGB(65, 68, 88);      // #414458 霓虹选择区
    colors.line_number = Color::RGB(110, 126, 176); // #6E7EB0 霓虹蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 100, 200); // #FF64C8 霓虹粉（高亮）✨

    colors.statusbar_bg = Color::RGB(65, 68, 88);    // #414458 深紫（状态栏）
    colors.statusbar_fg = Color::RGB(255, 255, 240); // #FFFFF0 荧光白

    colors.menubar_bg = Color::RGB(30, 32, 44);    // #1E202C 深紫黑
    colors.menubar_fg = Color::RGB(255, 255, 240); // #FFFFF0 荧光白

    colors.helpbar_bg = Color::RGB(65, 68, 88);
    colors.helpbar_fg = Color::RGB(200, 160, 255);  // #C8A0FF 霓虹紫
    colors.helpbar_key = Color::RGB(100, 255, 150); // #64FF96 霓虹绿

    // Dracula Neon 语法高亮：高饱和霓虹色
    colors.keyword = Color::RGB(255, 80, 180);        // #FF50B4 霓虹粉（关键词）
    colors.string = Color::RGB(255, 255, 100);        // #FFFF64 霓虹黄（字符串）
    colors.comment = Color::RGB(110, 126, 176);       // #6E7EB0 霓虹蓝灰（注释）
    colors.number = Color::RGB(200, 120, 255);        // #C878FF 霓虹紫（数字）
    colors.function = Color::RGB(100, 255, 150);      // #64FF96 霓虹绿（函数）
    colors.type = Color::RGB(100, 220, 255);          // #64DCFF 霓虹青（类型）
    colors.operator_color = Color::RGB(255, 80, 180); // 霓虹粉

    colors.error = Color::RGB(255, 80, 100);    // #FF5064 霓虹红（错误）
    colors.warning = Color::RGB(255, 180, 80);  // #FFB450 霓虹橙（警告）
    colors.info = Color::RGB(100, 220, 255);    // #64DCFF 霓虹青（信息）
    colors.success = Color::RGB(100, 255, 150); // #64FF96 霓虹绿（成功）

    colors.dialog_bg = Color::RGB(50, 52, 68);          // #323444 霓虹紫（对话框）
    colors.dialog_fg = Color::RGB(255, 255, 240);       // #FFFFF0 荧光白
    colors.dialog_title_bg = Color::RGB(255, 80, 180);  // #FF50B4 霓虹粉（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(200, 120, 255);   // #C878FF 霓虹紫边框

    return colors;
}

ThemeColors Theme::DraculaPastel() {
    ThemeColors colors;
    // Dracula Pastel: 德古拉粉彩版，柔和马卡龙色系
    // 灵感来源：马卡龙甜点、粉彩画笔、柔和彩虹、棉花糖
    colors.background = Color::RGB(48, 50, 66);     // #303242 柔和紫灰（马卡龙底）
    colors.foreground = Color::RGB(235, 230, 240);  // #EBE6F0 粉彩白（柔和文字）✨
    colors.current_line = Color::RGB(62, 64, 82);   // #3E4052 粉彩紫（选中行）
    colors.selection = Color::RGB(78, 80, 102);     // #4E5066 粉彩选择区
    colors.line_number = Color::RGB(145, 155, 185); // #919BB9 粉彩蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 180, 200); // #FFB4C8 粉彩粉（高亮）✨

    colors.statusbar_bg = Color::RGB(78, 80, 102);   // #4E5066 粉彩紫（状态栏）
    colors.statusbar_fg = Color::RGB(235, 230, 240); // #EBE6F0 粉彩白

    colors.menubar_bg = Color::RGB(43, 45, 58);    // #2B2D3A 深粉彩紫
    colors.menubar_fg = Color::RGB(235, 230, 240); // #EBE6F0 粉彩白

    colors.helpbar_bg = Color::RGB(78, 80, 102);
    colors.helpbar_fg = Color::RGB(200, 180, 230);  // #C8B4E6 粉彩紫
    colors.helpbar_key = Color::RGB(180, 230, 200); // #B4E6C8 粉彩绿

    // Dracula Pastel 语法高亮：马卡龙粉彩色系
    colors.keyword = Color::RGB(255, 160, 190);        // #FFA0BE 粉彩粉（关键词）
    colors.string = Color::RGB(255, 240, 160);         // #FFF0A0 粉彩黄（字符串）
    colors.comment = Color::RGB(145, 155, 185);        // #919BB9 粉彩蓝灰（注释）
    colors.number = Color::RGB(200, 170, 230);         // #C8AAE6 粉彩紫（数字）
    colors.function = Color::RGB(180, 230, 200);       // #B4E6C8 粉彩绿（函数）
    colors.type = Color::RGB(160, 210, 235);           // #A0D2EB 粉彩青（类型）
    colors.operator_color = Color::RGB(255, 160, 190); // 粉彩粉

    colors.error = Color::RGB(255, 150, 160);   // #FF96A0 粉彩红（错误）
    colors.warning = Color::RGB(255, 200, 140); // #FFC88C 粉彩橙（警告）
    colors.info = Color::RGB(160, 210, 235);    // #A0D2EB 粉彩青（信息）
    colors.success = Color::RGB(180, 230, 200); // #B4E6C8 粉彩绿（成功）

    colors.dialog_bg = Color::RGB(62, 64, 82);          // #3E4052 粉彩紫（对话框）
    colors.dialog_fg = Color::RGB(235, 230, 240);       // #EBE6F0 粉彩白
    colors.dialog_title_bg = Color::RGB(255, 160, 190); // #FFA0BE 粉彩粉（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(200, 170, 230);   // #C8AAE6 粉彩紫边框

    return colors;
}

ThemeColors Theme::DraculaDeep() {
    ThemeColors colors;
    // Dracula Deep: 德古拉深邃版，更深沉的暗夜风格
    // 灵感来源：深夜吸血鬼城堡、漆黑夜幕、深邃星空、神秘深渊
    colors.background = Color::RGB(25, 27, 38);    // #191B26 深漆黑（暗夜深渊）✨
    colors.foreground = Color::RGB(250, 250, 245); // #FAFAF5 月白（冷峻文字）
    colors.current_line = Color::RGB(38, 40, 56);  // #262838 深邃紫（选中行）
    colors.selection = Color::RGB(52, 55, 75);     // #34374B 深邃选择区
    colors.line_number = Color::RGB(85, 100, 145); // #556491 深渊蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 200, 100); // #FFC864 深金（高亮）✨

    colors.statusbar_bg = Color::RGB(52, 55, 75);    // #34374B 深邃紫（状态栏）
    colors.statusbar_fg = Color::RGB(250, 250, 245); // #FAFAF5 月白

    colors.menubar_bg = Color::RGB(20, 22, 32);    // #141620 极深紫黑
    colors.menubar_fg = Color::RGB(250, 250, 245); // #FAFAF5 月白

    colors.helpbar_bg = Color::RGB(52, 55, 75);
    colors.helpbar_fg = Color::RGB(165, 130, 220); // #A582DC 深紫（帮助栏）
    colors.helpbar_key = Color::RGB(70, 220, 110); // #46DC6E 深绿（按键）

    // Dracula Deep 语法高亮：更深沉的高饱和色
    colors.keyword = Color::RGB(255, 100, 180);        // #FF64B4 深粉（关键词）
    colors.string = Color::RGB(245, 255, 120);         // #F5FF78 深黄（字符串）
    colors.comment = Color::RGB(85, 100, 145);         // #556491 深渊蓝灰（注释）
    colors.number = Color::RGB(175, 130, 235);         // #AF82EB 深紫（数字）
    colors.function = Color::RGB(70, 220, 110);        // #46DC6E 深绿（函数）
    colors.type = Color::RGB(120, 215, 235);           // #78D7EB 深青（类型）
    colors.operator_color = Color::RGB(255, 100, 180); // 深粉

    colors.error = Color::RGB(255, 70, 85);    // #FF4655 深红（错误）
    colors.warning = Color::RGB(255, 170, 90); // #FFAA5A 深橙（警告）
    colors.info = Color::RGB(120, 215, 235);   // #78D7EB 深青（信息）
    colors.success = Color::RGB(70, 220, 110); // #46DC6E 深绿（成功）

    colors.dialog_bg = Color::RGB(38, 40, 56);          // #262838 深邃紫（对话框）
    colors.dialog_fg = Color::RGB(250, 250, 245);       // #FAFAF5 月白
    colors.dialog_title_bg = Color::RGB(52, 55, 75);    // #34374B 深邃紫（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(85, 100, 145);    // #556491 深渊蓝灰边框

    return colors;
}

ThemeColors Theme::GreeceClassic() {
    ThemeColors colors;
    // Greece Classic: 希腊经典版，经典蓝白配色
    // 灵感来源：圣托里尼蓝顶教堂、希腊国旗、爱琴海传统建筑
    colors.background = Color::RGB(18, 32, 55);     // #122037 经典深蓝（爱琴海夜空）
    colors.foreground = Color::RGB(250, 252, 255);  // #FAFCFF 纯白（圣托里尼白墙）✨
    colors.current_line = Color::RGB(32, 48, 72);   // #203048 经典蓝（选中行）
    colors.selection = Color::RGB(48, 68, 98);      // #304462 海蓝（选择区）
    colors.line_number = Color::RGB(135, 160, 190); // #87A0BE 浅蓝灰（行号）
    colors.line_number_current = Color::RGB(255, 215, 70); // #FFD746 阳光金（高亮）✨

    colors.statusbar_bg = Color::RGB(28, 42, 65);    // #1C2A41 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(250, 252, 255); // #FAFCFF 纯白

    colors.menubar_bg = Color::RGB(18, 32, 55);    // #122037 经典深蓝
    colors.menubar_fg = Color::RGB(250, 252, 255); // #FAFCFF 纯白

    colors.helpbar_bg = Color::RGB(28, 42, 65);
    colors.helpbar_fg = Color::RGB(175, 195, 215); // #AFC3D7 浅蓝
    colors.helpbar_key = Color::RGB(255, 205, 60); // #FFCD3C 阳光黄

    // Greece Classic 语法高亮：经典蓝白配色
    colors.keyword = Color::RGB(90, 150, 210);         // #5A96D2 经典希腊蓝（关键词）
    colors.string = Color::RGB(235, 240, 245);         // #EBF0F5 纯白（字符串）
    colors.comment = Color::RGB(125, 150, 175);        // #7D96AF 浅蓝灰（注释）
    colors.number = Color::RGB(255, 195, 50);          // #FFC332 金黄（数字）
    colors.function = Color::RGB(110, 170, 220);       // #6EAADC 天蓝（函数）
    colors.type = Color::RGB(255, 185, 60);            // #FFB93C 阳光黄（类型）
    colors.operator_color = Color::RGB(250, 252, 255); // #FAFCFF 纯白

    colors.error = Color::RGB(215, 65, 75);     // #D7414B 深红（错误）
    colors.warning = Color::RGB(255, 175, 45);  // #FFAF2D 橙黄（警告）
    colors.info = Color::RGB(85, 160, 220);     // #55A0DC 爱琴海蓝（信息）
    colors.success = Color::RGB(115, 180, 135); // #73B487 橄榄绿（成功）

    colors.dialog_bg = Color::RGB(32, 48, 72);          // #203048 经典蓝（对话框）
    colors.dialog_fg = Color::RGB(250, 252, 255);       // #FAFCFF 纯白
    colors.dialog_title_bg = Color::RGB(75, 135, 195);  // #4B87C3 希腊蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(95, 145, 195);    // #5F91C3 浅蓝边框

    return colors;
}

ThemeColors Theme::GreeceSunset() {
    ThemeColors colors;
    // Greece Sunset: 希腊日落版，爱琴海日落暖色调
    // 灵感来源：圣托里尼日落、爱琴海晚霞、金色余晖、橙色天空
    colors.background = Color::RGB(45, 35, 50);     // #2D2332 深紫红（日落后的天空）
    colors.foreground = Color::RGB(250, 245, 235);  // #FAF5EB 暖白（余晖）✨
    colors.current_line = Color::RGB(60, 48, 62);   // #3C303E 暖紫（选中行）
    colors.selection = Color::RGB(78, 62, 75);      // #4E3E4B 紫红（选择区）
    colors.line_number = Color::RGB(170, 155, 165); // #AA9BA5 暖灰（行号）
    colors.line_number_current = Color::RGB(255, 180, 100); // #FFB464 日落橙（高亮）✨

    colors.statusbar_bg = Color::RGB(55, 45, 58);    // #372D3A 深紫（状态栏）
    colors.statusbar_fg = Color::RGB(250, 245, 235); // #FAF5EB 暖白

    colors.menubar_bg = Color::RGB(40, 32, 45);    // #28202D 深紫红
    colors.menubar_fg = Color::RGB(250, 245, 235); // #FAF5EB 暖白

    colors.helpbar_bg = Color::RGB(55, 45, 58);
    colors.helpbar_fg = Color::RGB(210, 190, 180); // #D2BEB4 暖灰
    colors.helpbar_key = Color::RGB(255, 170, 90); // #FFAA5A 日落橙

    // Greece Sunset 语法高亮：暖色调日落配色
    colors.keyword = Color::RGB(255, 140, 120);        // #FF8C78 日落红（关键词）
    colors.string = Color::RGB(255, 220, 160);         // #FFDCA0 暖橙（字符串）
    colors.comment = Color::RGB(160, 145, 155);        // #A0919B 暖灰（注释）
    colors.number = Color::RGB(255, 190, 100);         // #FFBE64 金黄（数字）
    colors.function = Color::RGB(255, 160, 130);       // #FFA082 珊瑚红（函数）
    colors.type = Color::RGB(255, 200, 120);           // #FFC878 暖黄（类型）
    colors.operator_color = Color::RGB(250, 245, 235); // #FAF5EB 暖白

    colors.error = Color::RGB(255, 100, 90);    // #FF645A 鲜红（错误）
    colors.warning = Color::RGB(255, 180, 80);  // #FFB450 橙黄（警告）
    colors.info = Color::RGB(255, 170, 140);    // #FFAA8C 珊瑚粉（信息）
    colors.success = Color::RGB(180, 200, 140); // #B4C88C 橄榄绿（成功）

    colors.dialog_bg = Color::RGB(60, 48, 62);          // #3C303E 暖紫（对话框）
    colors.dialog_fg = Color::RGB(250, 245, 235);       // #FAF5EB 暖白
    colors.dialog_title_bg = Color::RGB(255, 140, 120); // #FF8C78 日落红（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(200, 160, 150);   // #C8A096 暖粉边框

    return colors;
}

ThemeColors Theme::GreeceOlive() {
    ThemeColors colors;
    // Greece Olive: 希腊橄榄版，橄榄绿 + 地中海蓝
    // 灵感来源：希腊橄榄树、地中海植被、绿色田园、自然生态
    colors.background = Color::RGB(25, 40, 35);     // #192823 深橄榄绿（橄榄树林）
    colors.foreground = Color::RGB(240, 245, 238);  // #F0F5EE 橄榄白（橄榄花）✨
    colors.current_line = Color::RGB(38, 55, 48);   // #263730 橄榄绿（选中行）
    colors.selection = Color::RGB(52, 70, 60);      // #34463C 深绿（选择区）
    colors.line_number = Color::RGB(145, 165, 155); // #91A59B 橄榄灰（行号）
    colors.line_number_current = Color::RGB(200, 230, 180); // #C8E6B4 嫩绿（高亮）✨

    colors.statusbar_bg = Color::RGB(35, 52, 45);    // #23342D 深橄榄（状态栏）
    colors.statusbar_fg = Color::RGB(240, 245, 238); // #F0F5EE 橄榄白

    colors.menubar_bg = Color::RGB(25, 40, 35);    // #192823 深橄榄绿
    colors.menubar_fg = Color::RGB(240, 245, 238); // #F0F5EE 橄榄白

    colors.helpbar_bg = Color::RGB(35, 52, 45);
    colors.helpbar_fg = Color::RGB(185, 205, 190);  // #B9CDBE 浅橄榄
    colors.helpbar_key = Color::RGB(180, 220, 160); // #B4DCA0 嫩绿

    // Greece Olive 语法高亮：橄榄绿配色
    colors.keyword = Color::RGB(140, 190, 160);        // #8CBEA0 橄榄绿（关键词）
    colors.string = Color::RGB(200, 220, 190);         // #C8DCBE 浅绿（字符串）
    colors.comment = Color::RGB(135, 155, 145);        // #879B91 橄榄灰（注释）
    colors.number = Color::RGB(220, 200, 140);         // #DCC88C 橄榄黄（数字）
    colors.function = Color::RGB(160, 210, 180);       // #A0D2B4 浅绿（函数）
    colors.type = Color::RGB(210, 190, 130);           // #D2BE82 橄榄金（类型）
    colors.operator_color = Color::RGB(240, 245, 238); // #F0F5EE 橄榄白

    colors.error = Color::RGB(220, 100, 110);   // #DC646E 深红（错误）
    colors.warning = Color::RGB(240, 180, 100); // #F0B464 橙黄（警告）
    colors.info = Color::RGB(130, 180, 200);    // #82B4C8 地中海蓝（信息）
    colors.success = Color::RGB(150, 200, 150); // #96C896 嫩绿（成功）

    colors.dialog_bg = Color::RGB(38, 55, 48);          // #263730 橄榄绿（对话框）
    colors.dialog_fg = Color::RGB(240, 245, 238);       // #F0F5EE 橄榄白
    colors.dialog_title_bg = Color::RGB(100, 150, 120); // #649678 橄榄绿（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(120, 150, 135);   // #789687 橄榄边框

    return colors;
}

ThemeColors Theme::GreeceMyth() {
    ThemeColors colors;
    // Greece Myth: 希腊神话版，紫金神话配色
    // 灵感来源：希腊众神、奥林匹斯山、紫色神袍、金色神光、神话传说
    colors.background = Color::RGB(30, 25, 45);            // #1E192D 深紫（神秘夜空）
    colors.foreground = Color::RGB(245, 240, 250);         // #F5F0FA 神光白（神圣光芒）✨
    colors.current_line = Color::RGB(45, 38, 62);          // #2D263E 深紫（选中行）
    colors.selection = Color::RGB(62, 52, 82);             // #3E3452 紫红（选择区）
    colors.line_number = Color::RGB(155, 145, 175);        // #9B91AF 紫灰（行号）
    colors.line_number_current = Color::RGB(255, 210, 80); // #FFD250 神光金（高亮）✨

    colors.statusbar_bg = Color::RGB(42, 35, 58);    // #2A233A 深紫（状态栏）
    colors.statusbar_fg = Color::RGB(245, 240, 250); // #F5F0FA 神光白

    colors.menubar_bg = Color::RGB(28, 23, 42);    // #1C172A 深紫
    colors.menubar_fg = Color::RGB(245, 240, 250); // #F5F0FA 神光白

    colors.helpbar_bg = Color::RGB(42, 35, 58);
    colors.helpbar_fg = Color::RGB(195, 180, 210); // #C3B4D2 浅紫
    colors.helpbar_key = Color::RGB(255, 200, 70); // #FFC846 神光金

    // Greece Myth 语法高亮：紫金神话配色
    colors.keyword = Color::RGB(180, 140, 220);        // #B48CDC 紫水晶（关键词）
    colors.string = Color::RGB(220, 200, 235);         // #DCC8EB 浅紫（字符串）
    colors.comment = Color::RGB(145, 135, 165);        // #9187A5 紫灰（注释）
    colors.number = Color::RGB(255, 195, 70);          // #FFC346 黄金（数字）
    colors.function = Color::RGB(200, 170, 235);       // #C8AAEB 紫罗兰（函数）
    colors.type = Color::RGB(255, 185, 60);            // #FFB93C 金（类型）
    colors.operator_color = Color::RGB(245, 240, 250); // #F5F0FA 神光白

    colors.error = Color::RGB(230, 90, 100);    // #E65A64 深红（错误）
    colors.warning = Color::RGB(255, 185, 60);  // #FFB93C 金黄（警告）
    colors.info = Color::RGB(170, 150, 220);    // #AA96DC 紫水晶（信息）
    colors.success = Color::RGB(160, 200, 140); // #A0C88C 橄榄绿（成功）

    colors.dialog_bg = Color::RGB(45, 38, 62);          // #2D263E 深紫（对话框）
    colors.dialog_fg = Color::RGB(245, 240, 250);       // #F5F0FA 神光白
    colors.dialog_title_bg = Color::RGB(140, 110, 190); // #8C6EBE 紫水晶（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(130, 120, 160);   // #8278A0 紫灰边框

    return colors;
}

ThemeColors Theme::GreeceAegean() {
    ThemeColors colors;
    // Greece Aegean: 希腊爱琴海版，深邃海洋蓝
    // 灵感来源：爱琴海深海、海洋生物、海底世界、深邃蓝调
    colors.background = Color::RGB(15, 30, 50);     // #0F1E32 深海蓝（爱琴海深渊）
    colors.foreground = Color::RGB(235, 245, 250);  // #EBF5FA 海沫白（海浪泡沫）✨
    colors.current_line = Color::RGB(28, 45, 68);   // #1C2D44 深蓝（选中行）
    colors.selection = Color::RGB(42, 62, 88);      // #2A3E58 海蓝（选择区）
    colors.line_number = Color::RGB(125, 150, 175); // #7D96AF 海灰（行号）
    colors.line_number_current = Color::RGB(100, 220, 200); // #64DCC8 海蓝绿（高亮）✨

    colors.statusbar_bg = Color::RGB(25, 42, 62);    // #192A3E 深蓝（状态栏）
    colors.statusbar_fg = Color::RGB(235, 245, 250); // #EBF5FA 海沫白

    colors.menubar_bg = Color::RGB(15, 30, 50);    // #0F1E32 深海蓝
    colors.menubar_fg = Color::RGB(235, 245, 250); // #EBF5FA 海沫白

    colors.helpbar_bg = Color::RGB(25, 42, 62);
    colors.helpbar_fg = Color::RGB(165, 190, 210); // #A5BED2 浅海蓝
    colors.helpbar_key = Color::RGB(90, 210, 190); // #5AD2BE 海蓝绿

    // Greece Aegean 语法高亮：深邃海洋配色
    colors.keyword = Color::RGB(80, 140, 200);         // #508CC8 海洋蓝（关键词）
    colors.string = Color::RGB(180, 210, 230);         // #B4D2E6 浅蓝（字符串）
    colors.comment = Color::RGB(115, 140, 165);        // #738CA5 海灰（注释）
    colors.number = Color::RGB(255, 200, 90);          // #FFC85A 海底金（数字）
    colors.function = Color::RGB(90, 180, 210);        // #5AB4D2 海蓝（函数）
    colors.type = Color::RGB(255, 180, 80);            // #FFB450 珊瑚金（类型）
    colors.operator_color = Color::RGB(235, 245, 250); // #EBF5FA 海沫白

    colors.error = Color::RGB(230, 80, 90);     // #E6505A 珊瑚红（错误）
    colors.warning = Color::RGB(255, 170, 70);  // #FFAA46 橙黄（警告）
    colors.info = Color::RGB(80, 160, 210);     // #50A0D2 海洋蓝（信息）
    colors.success = Color::RGB(100, 200, 180); // #64C8B4 海蓝绿（成功）

    colors.dialog_bg = Color::RGB(28, 45, 68);          // #1C2D44 深蓝（对话框）
    colors.dialog_fg = Color::RGB(235, 245, 250);       // #EBF5FA 海沫白
    colors.dialog_title_bg = Color::RGB(65, 120, 175);  // #4178AF 海洋蓝（标题栏）
    colors.dialog_title_fg = Color::RGB(255, 255, 255); // #FFFFFF 纯白
    colors.dialog_border = Color::RGB(85, 135, 175);    // #5587AF 浅蓝边框

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
    } else if (name == "monokai-dark") {
        colors_ = MonokaiDark();
    } else if (name == "monokai-light") {
        colors_ = MonokaiLight();
    } else if (name == "monokai-neon") {
        colors_ = MonokaiNeon();
    } else if (name == "monokai-pastel") {
        colors_ = MonokaiPastel();
    } else if (name == "orange") {
        colors_ = Orange();
    } else if (name == "solarized-dark") {
        colors_ = SolarizedDark();
    } else if (name == "solarized-light") {
        colors_ = SolarizedLight();
    } else if (name == "onedark") {
        colors_ = OneDark();
    } else if (name == "nord") {
        colors_ = Nord();
    } else if (name == "nord-frost") {
        colors_ = NordFrost();
    } else if (name == "nord-aurora") {
        colors_ = NordAurora();
    } else if (name == "nord-deep") {
        colors_ = NordDeep();
    } else if (name == "nord-light") {
        colors_ = NordLight();
    } else if (name == "nord-storm") {
        colors_ = NordStorm();
    } else if (name == "nord-polar") {
        colors_ = NordPolar();
    } else if (name == "nord-midnight") {
        colors_ = NordMidnight();
    } else if (name == "nord-glacier") {
        colors_ = NordGlacier();
    } else if (name == "purple-dark") {
        colors_ = PurpleDark();
    } else if (name == "gruvbox") {
        colors_ = Gruvbox();
    } else if (name == "tokyo-night") {
        colors_ = TokyoNight();
    } else if (name == "catppuccin") {
        colors_ = Catppuccin();
    } else if (name == "catppuccin-latte") {
        colors_ = CatppuccinLatte();
    } else if (name == "catppuccin-frappe") {
        colors_ = CatppuccinFrappe();
    } else if (name == "catppuccin-macchiato") {
        colors_ = CatppuccinMacchiato();
    } else if (name == "doraemon") {
        colors_ = Doraemon();
    } else if (name == "material") {
        colors_ = Material();
    } else if (name == "ayu") {
        colors_ = Ayu();
    } else if (name == "github") {
        colors_ = GitHub();
    } else if (name == "github-dark") {
        colors_ = GitHubDark();
    } else if (name == "github-dark-dimmed") {
        colors_ = GitHubDarkDimmed();
    } else if (name == "github-dark-high-contrast") {
        colors_ = GitHubDarkHighContrast();
    } else if (name == "github-light-high-contrast") {
        colors_ = GitHubLightHighContrast();
    } else if (name == "github-colorblind") {
        colors_ = GitHubColorblind();
    } else if (name == "github-tritanopia") {
        colors_ = GitHubTritanopia();
    } else if (name == "github-soft") {
        colors_ = GitHubSoft();
    } else if (name == "github-midnight") {
        colors_ = GitHubMidnight();
    } else if (name == "markdown-dark") {
        colors_ = MarkdownDark();
    } else if (name == "vscode-dark") {
        colors_ = VSCodeDark();
    } else if (name == "vscode-light") {
        colors_ = VSCodeLight();
    } else if (name == "vscode-light-modern") {
        colors_ = VSCodeLightModern();
    } else if (name == "vscode-dark-modern") {
        colors_ = VSCodeDarkModern();
    } else if (name == "vscode-monokai") {
        colors_ = VSCodeMonokai();
    } else if (name == "vscode-dark-plus") {
        colors_ = VSCodeDarkPlus();
    } else if (name == "dark-plus-moon-light") {
        colors_ = DarkPlusMoonLight();
    } else if (name == "night-owl") {
        colors_ = NightOwl();
    } else if (name == "palenight") {
        colors_ = Palenight();
    } else if (name == "oceanic-next") {
        colors_ = OceanicNext();
    } else if (name == "kanagawa") {
        colors_ = Kanagawa();
    } else if (name == "kanagawa-light") {
        colors_ = KanagawaLight();
    } else if (name == "kanagawa-sakura") {
        colors_ = KanagawaSakura();
    } else if (name == "kanagawa-midnight") {
        colors_ = KanagawaMidnight();
    } else if (name == "kanagawa-fuji") {
        colors_ = KanagawaFuji();
    } else if (name == "kanagawa-wave") {
        colors_ = KanagawaWave();
    } else if (name == "kanagawa-oni") {
        colors_ = KanagawaOni();
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
    } else if (name == "minions") {
        colors_ = Minions();
    } else if (name == "batman") {
        colors_ = Batman();
    } else if (name == "spongebob") {
        colors_ = SpongeBob();
    } else if (name == "modus-vivendi") {
        colors_ = ModusVivendi();
    } else if (name == "modus-operandi") {
        colors_ = ModusOperandi();
    } else if (name == "horizon") {
        colors_ = Horizon();
    } else if (name == "oxocarbon") {
        colors_ = Oxocarbon();
    } else if (name == "poimandres") {
        colors_ = Poimandres();
    } else if (name == "terafox") {
        colors_ = Terafox();
    } else if (name == "galaxy") {
        colors_ = Galaxy();
    } else if (name == "lightning") {
        colors_ = Lightning();
    } else if (name == "storm") {
        colors_ = Storm();
    } else if (name == "mellow") {
        colors_ = Mellow();
    } else if (name == "fleet") {
        colors_ = Fleet();
    } else if (name == "luna") {
        colors_ = Luna();
    } else if (name == "retro") {
        colors_ = Retro();
    } else if (name == "sunset") {
        colors_ = Sunset();
    } else if (name == "forest") {
        colors_ = Forest();
    } else if (name == "ocean") {
        colors_ = Ocean();
    } else if (name == "tango-dark") {
        colors_ = TangoDark();
    } else if (name == "synthwave") {
        colors_ = Synthwave();
    } else if (name == "retro-future") {
        colors_ = RetroFuture();
    } else if (name == "decay") {
        colors_ = Decay();
    } else if (name == "rider-dark") {
        colors_ = RiderDark();
    } else if (name == "parchment-dark") {
        colors_ = ParchmentDark();
    } else if (name == "crimson") {
        colors_ = Crimson();
    } else if (name == "frost") {
        colors_ = Frost();
    } else if (name == "lavender") {
        colors_ = Lavender();
    } else if (name == "matcha") {
        colors_ = Matcha();
    } else if (name == "aurora") {
        colors_ = Aurora();
    } else if (name == "amber") {
        colors_ = Amber();
    } else if (name == "mint") {
        colors_ = Mint();
    } else if (name == "obsidian") {
        colors_ = Obsidian();
    } else if (name == "coffee") {
        colors_ = Coffee();
    } else if (name == "ink") {
        colors_ = Ink();
    } else if (name == "sakura") {
        colors_ = Sakura();
    } else if (name == "sakura-dark") {
        colors_ = SakuraDark();
    } else if (name == "monochrome") {
        colors_ = Monochrome();
    } else if (name == "neon-noir") {
        colors_ = NeonNoir();
    } else if (name == "warm-sepia") {
        colors_ = WarmSepia();
    } else if (name == "colorful") {
        colors_ = Colorful();
    } else if (name == "microsoft") {
        colors_ = Microsoft();
    } else if (name == "google") {
        colors_ = Google();
    } else if (name == "meta") {
        colors_ = Meta();
    } else if (name == "intellij-dark") {
        colors_ = IntelliJDark();
    } else if (name == "doom-one") {
        colors_ = DoomOne();
    } else if (name == "vscode-light") {
        colors_ = VSCodeLight();
    } else if (name == "andromeda") {
        colors_ = Andromeda();
    } else if (name == "deep-space") {
        colors_ = DeepSpace();
    } else if (name == "volcanic") {
        colors_ = Volcanic();
    } else if (name == "arctic") {
        colors_ = Arctic();
    } else if (name == "neon-tokyo") {
        colors_ = NeonTokyo();
    } else if (name == "trae-dark") {
        colors_ = TraeDark();
    } else if (name == "trae-deep-blue") {
        colors_ = TraeDeepBlue();
    } else if (name == "midnight") {
        colors_ = Midnight();
    } else if (name == "francis-bacon") {
        colors_ = FrancisBacon();
    } else if (name == "moray") {
        colors_ = Moray();
    } else if (name == "van-gogh") {
        colors_ = VanGogh();
    } else if (name == "minecraft") {
        colors_ = Minecraft();
    } else if (name == "eva-unit01") {
        colors_ = EVAUnit01();
    } else if (name == "eva-unit02") {
        colors_ = EVAUnit02();
    } else if (name == "eva-unit00") {
        colors_ = EVAUnit00();
    } else if (name == "eva-mark06") {
        colors_ = EVAMark06();
    } else if (name == "eva-terminal") {
        colors_ = EVATerminal();
    } else if (name == "iron-man") {
        colors_ = IronMan();
    } else if (name == "spider-man") {
        colors_ = SpiderMan();
    } else if (name == "captain-america") {
        colors_ = CaptainAmerica();
    } else if (name == "hulk") {
        colors_ = Hulk();
    } else if (name == "superman") {
        colors_ = Superman();
    } else if (name == "godfather") {
        colors_ = Godfather();
    } else if (name == "robocop") {
        colors_ = RoboCop();
    } else if (name == "robot") {
        colors_ = Robot();
    } else if (name == "icy") {
        colors_ = Icy();
    } else if (name == "pure-blue") {
        colors_ = PureBlue();
    } else if (name == "silver") {
        colors_ = Silver();
    } else if (name == "egypt") {
        colors_ = Egypt();
    } else if (name == "oasis") {
        colors_ = Oasis();
    } else if (name == "aladdin") {
        colors_ = Aladdin();
    } else if (name == "denmark") {
        colors_ = Denmark();
    } else if (name == "france") {
        colors_ = France();
    } else if (name == "antarctica") {
        colors_ = Antarctica();
    } else if (name == "heaven") {
        colors_ = Heaven();
    } else if (name == "hell") {
        colors_ = Hell();
    } else if (name == "dracula-soft") {
        colors_ = DraculaSoft();
    } else if (name == "dracula-light") {
        colors_ = DraculaLight();
    } else if (name == "dracula-neon") {
        colors_ = DraculaNeon();
    } else if (name == "dracula-pastel") {
        colors_ = DraculaPastel();
    } else if (name == "dracula-deep") {
        colors_ = DraculaDeep();
    } else if (name == "greece-classic") {
        colors_ = GreeceClassic();
    } else if (name == "greece-sunset") {
        colors_ = GreeceSunset();
    } else if (name == "greece-olive") {
        colors_ = GreeceOlive();
    } else if (name == "greece-myth") {
        colors_ = GreeceMyth();
    } else if (name == "greece-aegean") {
        colors_ = GreeceAegean();
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
    const std::string& name, const std::vector<int>& background, const std::vector<int>& foreground,
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

    // 将通过配置加载的主题注册为自定义主题，但不立即切换当前主题
    return loadCustomTheme(name, colors);
}

ftxui::Color Theme::rgbToColor(const std::vector<int>& rgb) {
    if (rgb.size() >= 3) {
        return Color::RGB(rgb[0], rgb[1], rgb[2]);
    }
    return Color::Default;
}

std::vector<std::string> Theme::getAvailableThemes() {
    return {"monokai",
            "monokai-dark",
            "monokai-light",
            "monokai-neon",
            "monokai-pastel",
            "orange",
            "dracula-soft",
            "dracula-light",
            "dracula-neon",
            "dracula-pastel",
            "dracula-deep",
            "greece-classic",
            "greece-sunset",
            "greece-olive",
            "greece-myth",
            "greece-aegean",
            "solarized-dark",
            "solarized-light",
            "onedark",
            "nord",
            "nord-frost",
            "nord-aurora",
            "nord-deep",
            "nord-light",
            "nord-storm",
            "nord-polar",
            "nord-midnight",
            "nord-glacier",
            "purple-dark",
            "gruvbox",
            "tokyo-night",
            "catppuccin",
            "catppuccin-latte",
            "catppuccin-frappe",
            "catppuccin-macchiato",
            "doraemon",
            "material",
            "ayu",
            "github",
            "github-dark",
            "github-dark-dimmed",
            "github-dark-high-contrast",
            "github-light-high-contrast",
            "github-colorblind",
            "github-tritanopia",
            "github-soft",
            "github-midnight",
            "markdown-dark",
            "vscode-dark",
            "vscode-light",
            "vscode-light-modern",
            "vscode-dark-modern",
            "vscode-monokai",
            "vscode-dark-plus",
            "dark-plus-moon-light",
            "night-owl",
            "palenight",
            "oceanic-next",
            "kanagawa",
            "kanagawa-light",
            "kanagawa-sakura",
            "kanagawa-midnight",
            "kanagawa-fuji",
            "kanagawa-wave",
            "kanagawa-oni",
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
            "hatsune-miku",
            "minions",
            "batman",
            "spongebob",
            "modus-vivendi",
            "modus-operandi",
            "horizon",
            "oxocarbon",
            "poimandres",
            "terafox",
            "galaxy",
            "lightning",
            "storm",
            "mellow",
            "fleet",
            "luna",
            "retro",
            "sunset",
            "forest",
            "ocean",
            "tango-dark",
            "synthwave",
            "retro-future",
            "decay",
            "rider-dark",
            "parchment-dark",
            "crimson",
            "frost",
            "lavender",
            "matcha",
            "aurora",
            "amber",
            "mint",
            "obsidian",
            "coffee",
            "ink",
            "sakura",
            "sakura-dark",
            "monochrome",
            "neon-noir",
            "warm-sepia",
            "colorful",
            "microsoft",
            "google",
            "meta",
            "intellij-dark",
            "doom-one",
            "vscode-light",
            "andromeda",
            "deep-space",
            "volcanic",
            "arctic",
            "neon-tokyo",
            "trae-dark",
            "trae-deep-blue",
            "midnight",
            "francis-bacon",
            "moray",
            "van-gogh",
            "minecraft",
            "eva-unit01",
            "eva-unit02",
            "eva-unit00",
            "eva-mark06",
            "eva-terminal",
            "iron-man",
            "spider-man",
            "captain-america",
            "hulk",
            "superman",
            "godfather",
            "robocop",
            "robot",
            "icy",
            "pure-blue",
            "silver",
            "egypt",
            "oasis",
            "aladdin",
            "denmark",
            "france",
            "antarctica",
            "heaven",
            "hell"};
}

std::vector<std::string> Theme::getCustomThemeNames() const {
    std::vector<std::string> names;
    for (const auto& [name, _] : custom_themes_) {
        names.push_back(name);
    }
    return names;
}

std::vector<Color> Theme::getGradientColors() const {
    using ColorMember = Color ThemeColors::*;
    static const std::map<std::string, std::array<ColorMember, 6>> theme_gradients = {
        {"monokai",
         {&ThemeColors::success, &ThemeColors::type, &ThemeColors::keyword, &ThemeColors::type,
          &ThemeColors::success, &ThemeColors::function}},
        {"dracula",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::type, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"solarized-dark",
         {&ThemeColors::function, &ThemeColors::type, &ThemeColors::keyword, &ThemeColors::type,
          &ThemeColors::function, &ThemeColors::string}},
        {"solarized-light",
         {&ThemeColors::function, &ThemeColors::type, &ThemeColors::keyword, &ThemeColors::type,
          &ThemeColors::function, &ThemeColors::string}},
        {"onedark",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::string,
          &ThemeColors::function, &ThemeColors::keyword, &ThemeColors::type}},
        {"nord",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::function, &ThemeColors::keyword, &ThemeColors::success}},
        {"gruvbox",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"tokyo-night",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::function, &ThemeColors::keyword, &ThemeColors::success}},
        {"catppuccin",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"catppuccin-latte",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"catppuccin-frappe",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"catppuccin-macchiato",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"doraemon",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::string,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"material",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"ayu",
         {&ThemeColors::keyword, &ThemeColors::string, &ThemeColors::function, &ThemeColors::string,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"github-dark",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::string,
          &ThemeColors::function, &ThemeColors::keyword, &ThemeColors::success}},
        {"github",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::string,
          &ThemeColors::function, &ThemeColors::keyword, &ThemeColors::success}},
        {"vscode-dark",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"dark-plus-moon-light",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"night-owl",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-light",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-sakura",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-midnight",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-fuji",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-wave",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"kanagawa-oni",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"rose-pine",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"everforest",
         {&ThemeColors::function, &ThemeColors::type, &ThemeColors::keyword, &ThemeColors::type,
          &ThemeColors::function, &ThemeColors::success}},
        {"horizon",
         {&ThemeColors::keyword, &ThemeColors::number, &ThemeColors::function, &ThemeColors::number,
          &ThemeColors::keyword, &ThemeColors::success}},
        {"cyberpunk",
         {&ThemeColors::keyword, &ThemeColors::function, &ThemeColors::type, &ThemeColors::function,
          &ThemeColors::keyword, &ThemeColors::success}},
    };

    auto it = theme_gradients.find(current_theme_);

    if (it != theme_gradients.end()) {
        return {
            colors_.*(it->second[0]), colors_.*(it->second[1]), colors_.*(it->second[2]),
            colors_.*(it->second[3]), colors_.*(it->second[4]), colors_.*(it->second[5]),
        };
    }

    return {colors_.success, colors_.type,    colors_.keyword,
            colors_.type,    colors_.success, colors_.function};
}

} // namespace ui
} // namespace pnana
