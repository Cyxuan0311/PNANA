#include "input/event_parser.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace input {

Modifiers EventParser::parseModifiers(const ftxui::Event& event) const {
    Modifiers mods;
    std::string input = event.input();

    // 转换为小写以便比较（不区分大小写）
    std::transform(input.begin(), input.end(), input.begin(), ::tolower);

    mods.ctrl = (input.find("ctrl") != std::string::npos);
    mods.alt = (input.find("alt") != std::string::npos);
    mods.shift = (input.find("shift") != std::string::npos);
    mods.meta = (input.find("meta") != std::string::npos);

    return mods;
}

bool EventParser::isCtrlShift(const ftxui::Event& event) const {
    auto mods = parseModifiers(event);
    return mods.ctrl && mods.shift;
}

bool EventParser::isAlt(const ftxui::Event& event) const {
    return parseModifiers(event).alt;
}

bool EventParser::isCtrl(const ftxui::Event& event) const {
    return parseModifiers(event).ctrl;
}

bool EventParser::isShift(const ftxui::Event& event) const {
    return parseModifiers(event).shift;
}

std::string EventParser::parseCtrlKey(const ftxui::Event& event) const {
    // Ctrl 组合键（A-Z）- 使用 FTXUI 预定义事件
    if (event == ftxui::Event::CtrlA)
        return "ctrl_a";
    if (event == ftxui::Event::CtrlB)
        return "ctrl_b";
    if (event == ftxui::Event::CtrlC)
        return "ctrl_c";
    if (event == ftxui::Event::CtrlD)
        return "ctrl_d";
    if (event == ftxui::Event::CtrlE)
        return "ctrl_e";
    if (event == ftxui::Event::CtrlF)
        return "ctrl_f";
    if (event == ftxui::Event::CtrlG)
        return "ctrl_g";
    if (event == ftxui::Event::CtrlH)
        return "ctrl_h";
    // 注意：Ctrl+I 在终端中通常是 Tab 键的别名（ASCII 9）
    // 但我们应该让 parseSpecialKey() 优先处理 Tab，所以这里不处理 Ctrl+I
    // 如果确实需要 Ctrl+I 作为快捷键，可以在这里添加，但会与 Tab 冲突
    // if (event == ftxui::Event::CtrlI) return "ctrl_i";  // 注释掉，避免与 Tab 冲突

    // 注意：Ctrl+J 在终端中通常是 Enter 键的别名（ASCII 10）
    // 但我们应该让 parseSpecialKey() 优先处理 Enter，所以这里不处理 Ctrl+J
    // if (event == ftxui::Event::CtrlJ) return "ctrl_j";  // 注释掉，避免与 Enter 冲突
    if (event == ftxui::Event::CtrlK) {
        if (isCtrlShift(event))
            return "ctrl_shift_k";
        return "ctrl_k";
    }
    if (event == ftxui::Event::CtrlL) {
        if (isCtrlShift(event))
            return "ctrl_shift_l";
        return "ctrl_l";
    }
    if (event == ftxui::Event::CtrlM)
        return "ctrl_m"; // Enter 的别名
    if (event == ftxui::Event::CtrlN)
        return "ctrl_n";
    if (event == ftxui::Event::CtrlO)
        return "ctrl_o";
    if (event == ftxui::Event::CtrlP)
        return "ctrl_p";
    if (event == ftxui::Event::CtrlQ)
        return "ctrl_q";
    if (event == ftxui::Event::CtrlR)
        return "ctrl_r";
    if (event == ftxui::Event::CtrlS)
        return "ctrl_s";
    if (event == ftxui::Event::CtrlT)
        return "ctrl_t";
    if (event == ftxui::Event::CtrlU)
        return "ctrl_u";
    if (event == ftxui::Event::CtrlV)
        return "ctrl_v";
    if (event == ftxui::Event::CtrlW)
        return "ctrl_w";
    if (event == ftxui::Event::CtrlX)
        return "ctrl_x";
    if (event == ftxui::Event::CtrlY)
        return "ctrl_y";
    if (event == ftxui::Event::CtrlZ) {
        if (isCtrlShift(event))
            return "ctrl_shift_z";
        return "ctrl_z";
    }

    // Ctrl+数字键
    if (event.is_character()) {
        std::string ch = event.character();
        auto mods = parseModifiers(event);
        if (mods.ctrl && ch.length() == 1) {
            char c = ch[0];
            if (c >= '0' && c <= '9') {
                return "ctrl_" + std::string(1, c);
            }
        }
    }

    return "";
}

std::string EventParser::parseFunctionKey(const ftxui::Event& event) const {
    if (event == ftxui::Event::F1)
        return "f1";
    if (event == ftxui::Event::F2)
        return "f2";
    if (event == ftxui::Event::F3) {
        if (isShift(event))
            return "shift_f3";
        return "f3";
    }
    if (event == ftxui::Event::F4)
        return "f4";
    if (event == ftxui::Event::F5)
        return "f5";
    if (event == ftxui::Event::F6)
        return "f6";
    if (event == ftxui::Event::F7)
        return "f7";
    if (event == ftxui::Event::F8)
        return "f8";
    if (event == ftxui::Event::F9)
        return "f9";
    if (event == ftxui::Event::F10)
        return "f10";
    if (event == ftxui::Event::F11)
        return "f11";
    if (event == ftxui::Event::F12)
        return "f12";
    return "";
}

std::string EventParser::parseNavigationKey(const ftxui::Event& event) const {
    auto mods = parseModifiers(event);

    if (event == ftxui::Event::Home) {
        if (mods.ctrl)
            return "ctrl_home";
        return "home";
    }
    if (event == ftxui::Event::End) {
        if (mods.ctrl)
            return "ctrl_end";
        return "end";
    }
    if (event == ftxui::Event::PageUp) {
        if (mods.ctrl)
            return "ctrl_pageup";
        return "pageup";
    }
    if (event == ftxui::Event::PageDown) {
        if (mods.ctrl)
            return "ctrl_pagedown";
        return "pagedown";
    }
    return "";
}

std::string EventParser::parseArrowKey(const ftxui::Event& event) const {
    auto mods = parseModifiers(event);

    if (event == ftxui::Event::ArrowUp) {
        if (mods.ctrl)
            return "ctrl_up";
        if (isAlt(event)) {
            if (mods.shift)
                return "alt_shift_arrow_up";
            return "alt_arrow_up";
        }
        return "arrow_up";
    }
    if (event == ftxui::Event::ArrowDown) {
        if (mods.ctrl)
            return "ctrl_down";
        if (isAlt(event)) {
            if (mods.shift)
                return "alt_shift_arrow_down";
            return "alt_arrow_down";
        }
        return "arrow_down";
    }
    if (event == ftxui::Event::ArrowLeft) {
        if (mods.ctrl)
            return "ctrl_left";
        if (isAlt(event)) {
            if (mods.shift)
                return "alt_shift_arrow_left";
            return "alt_arrow_left";
        }
        return "arrow_left";
    }
    if (event == ftxui::Event::ArrowRight) {
        if (mods.ctrl)
            return "ctrl_right";
        if (isAlt(event)) {
            if (mods.shift)
                return "alt_shift_arrow_right";
            return "alt_arrow_right";
        }
        return "arrow_right";
    }

    // Shift+方向键（用于选择）
    if (event == ftxui::Event::ArrowUpCtrl)
        return "shift_arrow_up";
    if (event == ftxui::Event::ArrowDownCtrl)
        return "shift_arrow_down";
    if (event == ftxui::Event::ArrowLeftCtrl)
        return "shift_arrow_left";
    if (event == ftxui::Event::ArrowRightCtrl)
        return "shift_arrow_right";

    return "";
}

std::string EventParser::parseSpecialKey(const ftxui::Event& event) const {
    if (event == ftxui::Event::Escape)
        return "escape";
    if (event == ftxui::Event::Return)
        return "return";

    if (event == ftxui::Event::Backspace) {
        if (isCtrl(event))
            return "ctrl_backspace";
        return "backspace";
    }
    if (event == ftxui::Event::Delete) {
        if (isCtrl(event))
            return "ctrl_delete";
        return "delete";
    }

    // Tab 键
    if (event == ftxui::Event::Tab) {
        if (isAlt(event)) {
            auto mods = parseModifiers(event);
            if (mods.shift)
                return "alt_shift_tab";
            return "alt_tab";
        }
        return "tab";
    }
    if (event == ftxui::Event::TabReverse)
        return "shift_tab";

    return "";
}

std::string EventParser::parseAltKey(const ftxui::Event& event) const {
    std::string input = event.input();

    // 在终端中，Alt+字符通常被编码为 ESC 字符（\033 或 \x1b，ASCII 27）后跟字符
    // 例如：Alt+A → \033a 或 \x1ba
    if (input.length() >= 2) {
        // 检查是否是 ESC 序列（\033 或 \x1b）
        if (input[0] == '\033' || input[0] == '\x1b') {
            // 提取字符部分
            std::string ch = input.substr(1);
            // 只取第一个字符
            if (ch.length() >= 1) {
                char c = ch[0];
                // 处理 Alt+Space（Space 字符是 ' '，ASCII 32）
                if (c == ' ') {
                    return "alt_space";
                }
                c = std::tolower(c);
                if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                    return "alt_" + std::string(1, c);
                }
            }
        }
    }

    // 也检查 isAlt() 方法（用于其他可能的 Alt 键表示方式）
    if (isAlt(event) && event.is_character()) {
        std::string ch = event.character();
        // 处理 Alt+Space
        if (ch == " ") {
            return "alt_space";
        }
        std::transform(ch.begin(), ch.end(), ch.begin(), ::tolower);
        if (ch.length() == 1) {
            // 字母、数字、以及一些特殊字符
            if ((ch[0] >= 'a' && ch[0] <= 'z') || (ch[0] >= '0' && ch[0] <= '9') || ch[0] == '=' ||
                ch[0] == '+' || ch[0] == '-' || ch[0] == '_') {
                return "alt_" + ch;
            }
        }
    }

    return "";
}

std::string EventParser::parseCtrlSpecialChar(const ftxui::Event& event) const {
    if (event.is_character()) {
        std::string ch = event.character();
        auto mods = parseModifiers(event);
        if (mods.ctrl) {
            if (ch == "/")
                return "ctrl_slash";
            if (ch == "\\")
                return "ctrl_backslash";
            if (ch == "-")
                return "ctrl_minus";
            if (ch == "=" || ch == "+")
                return "ctrl_plus";
            if (ch == " ")
                return "ctrl_space";
            if (ch == "'" || ch == "\"")
                return "ctrl_quote"; // Ctrl+' 或 Ctrl+"
        }
    }
    return "";
}

std::string EventParser::parseSpaceKey(const ftxui::Event& event) const {
    if (event.is_character()) {
        std::string input = event.input();
        std::string ch = event.character();

        // 检测 Space+A 组合
        // 方法1：输入字符串包含 " a" 或 " A"（Space 后跟 A）
        if (input.length() >= 2) {
            // 检查是否包含 Space 后跟 'a' 或 'A'
            for (size_t i = 0; i < input.length() - 1; ++i) {
                if (input[i] == ' ') {
                    char next_char = std::tolower(input[i + 1]);
                    if (next_char == 'a') {
                        return "space_a";
                    }
                }
            }
        }

        // 方法2：字符是 'a' 或 'A'，且输入字符串以 Space 开头
        if (ch.length() == 1) {
            char c = std::tolower(ch[0]);
            if (c == 'a' && input.length() >= 2 && input[0] == ' ') {
                return "space_a";
            }
        }

        // 方法3：输入字符串正好是 " a" 或 " A"
        if (input == " a" || input == " A") {
            return "space_a";
        }
    }
    return "";
}

std::string EventParser::eventToKey(const ftxui::Event& event) const {
    // 按优先级顺序解析

    // 0. 特殊键（Tab、Shift+Tab等）优先处理，避免被 Ctrl 组合键误识别
    // 注意：Tab 键在终端中可能被编码为 Ctrl+I，但我们应该优先识别为 Tab
    if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
        std::string key = parseSpecialKey(event);
        if (!key.empty())
            return key;
    }

    // 1. Ctrl 组合键（但排除 Tab，因为 Tab 已经在上面处理了）
    std::string key = parseCtrlKey(event);
    if (!key.empty())
        return key;

    // 2. 功能键
    key = parseFunctionKey(event);
    if (!key.empty())
        return key;

    // 3. 导航键
    key = parseNavigationKey(event);
    if (!key.empty())
        return key;

    // 4. 方向键
    key = parseArrowKey(event);
    if (!key.empty())
        return key;

    // 5. 特殊键（Tab 已经在上面处理，这里处理其他特殊键）
    key = parseSpecialKey(event);
    if (!key.empty())
        return key;

    // 6. Alt+字符组合
    key = parseAltKey(event);
    if (!key.empty())
        return key;

    // 7. Ctrl+特殊字符组合
    key = parseCtrlSpecialChar(event);
    if (!key.empty())
        return key;

    // 8. Space+字符组合（如 Space+A）
    key = parseSpaceKey(event);
    if (!key.empty())
        return key;

    // 9. 普通字符（不作为快捷键）
    if (event.is_character()) {
        std::string ch = event.character();
        if (ch.length() == 1 && ch[0] >= 32 && ch[0] < 127) {
            // 可打印字符，但不作为快捷键处理
            return "";
        }
    }

    return "";
}

} // namespace input
} // namespace pnana
