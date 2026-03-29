#include "features/terminal/terminal_color.h"
#include <algorithm>
#include <sstream>

namespace pnana {
namespace features {
namespace terminal {

// AnsiColorPalette 实现
AnsiColorPalette AnsiColorPalette::defaultPalette() {
    AnsiColorPalette palette;
    // 使用 Solarized 风格的颜色
    palette.colors[0] = {0x07, 0x36, 0x42};  // base02
    palette.colors[1] = {0xdc, 0x32, 0x2f};  // red
    palette.colors[2] = {0x85, 0x99, 0x00};  // green
    palette.colors[3] = {0xb5, 0x89, 0x00};  // yellow
    palette.colors[4] = {0x26, 0x8b, 0xd2};  // blue
    palette.colors[5] = {0xd3, 0x36, 0x82};  // magenta
    palette.colors[6] = {0x2a, 0xa1, 0x98};  // cyan
    palette.colors[7] = {0xee, 0xe8, 0xd5};  // base2
    palette.colors[8] = {0x00, 0x2b, 0x36};  // base03
    palette.colors[9] = {0xcb, 0x4b, 0x16};  // orange
    palette.colors[10] = {0x58, 0x6e, 0x75}; // base01
    palette.colors[11] = {0x65, 0x7b, 0x83}; // base00
    palette.colors[12] = {0x83, 0x94, 0x96}; // base0
    palette.colors[13] = {0x6c, 0x71, 0xc4}; // violet
    palette.colors[14] = {0x93, 0xa1, 0xa1}; // base1
    palette.colors[15] = {0xfd, 0xf6, 0xe3}; // base3
    return palette;
}

AnsiColorPalette AnsiColorPalette::fromConfig(const std::string& theme_name) {
    // TODO: 从配置文件加载主题
    // 目前返回默认主题
    (void)theme_name;
    return defaultPalette();
}

// 256 色表缓存（用于高性能访问）
static const std::array<ftxui::Color, 256>& build256ColorTable() {
    static std::array<ftxui::Color, 256> table = []() {
        std::array<ftxui::Color, 256> tbl{};

        // 0-15: ANSI 标准色和亮色
        // 标准色 (0-7)
        tbl[0] = ftxui::Color::RGB(0x00, 0x00, 0x00); // Black
        tbl[1] = ftxui::Color::RGB(0xcd, 0x31, 0x31); // Red
        tbl[2] = ftxui::Color::RGB(0x0e, 0xb8, 0x34); // Green
        tbl[3] = ftxui::Color::RGB(0xb5, 0x89, 0x00); // Yellow
        tbl[4] = ftxui::Color::RGB(0x26, 0x8a, 0xd2); // Blue
        tbl[5] = ftxui::Color::RGB(0xd3, 0x36, 0x82); // Magenta
        tbl[6] = ftxui::Color::RGB(0x2a, 0xa1, 0xa3); // Cyan
        tbl[7] = ftxui::Color::RGB(0xee, 0xee, 0xee); // White

        // 亮色 (8-15)
        tbl[8] = ftxui::Color::RGB(0x55, 0x55, 0x55);  // Bright Black (Gray)
        tbl[9] = ftxui::Color::RGB(0xff, 0x6a, 0x00);  // Bright Red
        tbl[10] = ftxui::Color::RGB(0x5a, 0xfd, 0x57); // Bright Green
        tbl[11] = ftxui::Color::RGB(0xff, 0xff, 0x5f); // Bright Yellow
        tbl[12] = ftxui::Color::RGB(0x5f, 0x87, 0xff); // Bright Blue
        tbl[13] = ftxui::Color::RGB(0xff, 0x55, 0xff); // Bright Magenta
        tbl[14] = ftxui::Color::RGB(0x5a, 0xff, 0xff); // Bright Cyan
        tbl[15] = ftxui::Color::RGB(0xff, 0xff, 0xff); // Bright White

        // 16-231: 6x6x6 颜色立方体
        // 使用精确的色值：00, 95, 135, 175, 215, 255
        static const uint8_t color_values[6] = {0x00, 0x5f, 0x87, 0xaf, 0xd7, 0xff};
        int idx = 16;
        for (int r = 0; r < 6; r++) {
            for (int g = 0; g < 6; g++) {
                for (int b = 0; b < 6; b++) {
                    tbl[idx++] =
                        ftxui::Color::RGB(color_values[r], color_values[g], color_values[b]);
                }
            }
        }

        // 232-255: 灰度渐变 (从 8 到 238，步长为 10)
        for (int i = 0; i < 24; i++) {
            uint8_t gray = static_cast<uint8_t>(8 + i * 10);
            tbl[232 + i] = ftxui::Color::RGB(gray, gray, gray);
        }

        return tbl;
    }();

    return table;
}

const std::array<ftxui::Color, 256>& AnsiColorParser::get256ColorTable() {
    return build256ColorTable();
}

ftxui::Element AnsiColorParser::parse(const std::string& text) {
    if (!hasAnsiCodes(text)) {
        return ftxui::text(text);
    }

    std::vector<ftxui::Element> elements;
    std::string current_text;
    ParseState state = ParseState::Normal;
    std::string escape_sequence;

    // 默认样式
    ftxui::Color fg_color = ftxui::Color::Default;
    ftxui::Color bg_color = ftxui::Color::Default;
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool blink = false;
    bool reverse = false;
    bool strikethrough = false;

    auto add_current_element = [&]() {
        if (!current_text.empty()) {
            ftxui::Element elem = ftxui::text(current_text);

            // 应用样式
            if (bold)
                elem = elem | ftxui::bold;
            // Note: italic not supported in current FTXUI version
            // if (italic) elem = elem | ftxui::italic;
            if (underline)
                elem = elem | ftxui::underlined;
            if (blink)
                elem = elem | ftxui::blink;
            if (reverse)
                elem = elem | ftxui::inverted;
            if (strikethrough)
                elem = elem | ftxui::strikethrough;

            // 应用颜色
            if (fg_color != ftxui::Color::Default) {
                elem = elem | ftxui::color(fg_color);
            }
            if (bg_color != ftxui::Color::Default) {
                elem = elem | ftxui::bgcolor(bg_color);
            }

            elements.push_back(elem);
            current_text.clear();
        }
    };

    for (size_t i = 0; i < text.length(); ++i) {
        char c = text[i];

        switch (state) {
            case ParseState::Normal:
                if (c == '\x1b') {         // ESC
                    add_current_element(); // 添加之前的文本
                    state = ParseState::Escape;
                    escape_sequence = "\x1b";
                } else {
                    current_text += c;
                }
                break;

            case ParseState::Escape:
                escape_sequence += c;
                if (c == '[') {
                    state = ParseState::CSI;
                } else if (c == ']') {
                    state = ParseState::OSC;
                } else {
                    // 不是CSI或OSC序列，回到正常状态
                    current_text += escape_sequence;
                    state = ParseState::Normal;
                }
                break;

            case ParseState::CSI:
                escape_sequence += c;
                if (c >= '@' && c <= '~') { // CSI结束符
                    // 解析CSI序列
                    std::string params_str =
                        escape_sequence.substr(2, escape_sequence.length() - 3);
                    auto params = parseCsiParams(params_str);

                    if (!params.empty()) {
                        int command = params.back();
                        params.pop_back();

                        switch (command) {
                            case 0: // 重置所有样式
                                fg_color = ftxui::Color::Default;
                                bg_color = ftxui::Color::Default;
                                bold = italic = underline = blink = reverse = strikethrough = false;
                                break;
                            case 1: // 粗体
                                bold = true;
                                break;
                            case 3: // 斜体
                                italic = true;
                                break;
                            case 4: // 下划线
                                underline = true;
                                break;
                            case 5: // 闪烁
                            case 6: // 快速闪烁
                                blink = true;
                                break;
                            case 7: // 反转
                                reverse = true;
                                break;
                            case 9: // 删除线
                                strikethrough = true;
                                break;
                            case 21: // 关闭粗体
                                bold = false;
                                break;
                            case 22: // 关闭粗体/亮色
                                bold = false;
                                break;
                            case 23: // 关闭斜体
                                italic = false;
                                break;
                            case 24: // 关闭下划线
                                underline = false;
                                break;
                            case 25: // 关闭闪烁
                                blink = false;
                                break;
                            case 27: // 关闭反转
                                reverse = false;
                                break;
                            case 29: // 关闭删除线
                                strikethrough = false;
                                break;
                            case 39: // 默认前景色
                                fg_color = ftxui::Color::Default;
                                break;
                            case 49: // 默认背景色
                                bg_color = ftxui::Color::Default;
                                break;
                        }

                        // 前景色 (30-37: 标准色, 38: 256色或RGB, 90-97: 亮色)
                        if (command >= 30 && command <= 37) {
                            fg_color = ansiColorToFtxui(command - 30);
                        } else if (command >= 90 && command <= 97) {
                            fg_color = ansiColorToFtxui(command - 82); // 90-97 -> 8-15 (亮色)
                        } else if (command == 38 && params.size() >= 2) {
                            if (params[0] == 5 && params.size() >= 1) { // 256色
                                fg_color = ansi256ColorToFtxui(params[1]);
                            } else if (params[0] == 2 && params.size() >= 4) { // RGB
                                fg_color = rgbColorToFtxui(params[1], params[2], params[3]);
                            }
                        }

                        // 背景色 (40-47: 标准色, 48: 256色或RGB, 100-107: 亮色)
                        if (command >= 40 && command <= 47) {
                            bg_color = ansiColorToFtxui(command - 40);
                        } else if (command >= 100 && command <= 107) {
                            bg_color = ansiColorToFtxui(command - 92); // 100-107 -> 8-15 (亮色)
                        } else if (command == 48 && params.size() >= 2) {
                            if (params[0] == 5 && params.size() >= 1) { // 256色
                                bg_color = ansi256ColorToFtxui(params[1]);
                            } else if (params[0] == 2 && params.size() >= 4) { // RGB
                                bg_color = rgbColorToFtxui(params[1], params[2], params[3]);
                            }
                        }
                    }

                    state = ParseState::Normal;
                    escape_sequence.clear();
                }
                break;

            case ParseState::OSC:
                escape_sequence += c;
                if (c == '\x07' || (c == '\\' && i > 0 && text[i - 1] == '\x1b')) { // OSC结束符
                    // OSC序列通常用于窗口标题等，我们忽略它们
                    state = ParseState::Normal;
                    escape_sequence.clear();
                }
                break;
        }
    }

    // 添加最后的文本
    add_current_element();

    if (elements.size() == 1) {
        return elements[0];
    } else {
        return ftxui::hbox(std::move(elements));
    }
}

bool AnsiColorParser::hasAnsiCodes(const std::string& text) {
    return text.find('\x1b') != std::string::npos;
}

std::string AnsiColorParser::stripAnsiCodes(const std::string& text) {
    std::string result;
    ParseState state = ParseState::Normal;

    for (char c : text) {
        switch (state) {
            case ParseState::Normal:
                if (c == '\x1b') {
                    state = ParseState::Escape;
                } else {
                    result += c;
                }
                break;

            case ParseState::Escape:
                if (c == '[') {
                    state = ParseState::CSI;
                } else if (c == ']') {
                    state = ParseState::OSC;
                } else {
                    result += '\x1b';
                    result += c;
                    state = ParseState::Normal;
                }
                break;

            case ParseState::CSI:
                if (c >= '@' && c <= '~') {
                    state = ParseState::Normal;
                }
                break;

            case ParseState::OSC:
                if (c == '\x07' || c == '\\') {
                    state = ParseState::Normal;
                }
                break;
        }
    }

    return result;
}

ftxui::Color AnsiColorParser::ansiColorToFtxui(int ansi_code) {
    // 精确的 ANSI 颜色映射表
    // 参考：https://en.wikipedia.org/wiki/ANSI_escape_code#3/4_bit
    // 这些颜色值与主流终端模拟器（xterm, gnome-terminal, iTerm2）保持一致

    static const std::vector<ftxui::Color> ansi_colors = {
        // 标准色 (0-7)
        ftxui::Color::RGB(0x00, 0x00, 0x00), // 0: Black
        ftxui::Color::RGB(0xcd, 0x31, 0x31), // 1: Red
        ftxui::Color::RGB(0x0e, 0xb8, 0x34), // 2: Green
        ftxui::Color::RGB(0xb5, 0x89, 0x00), // 3: Yellow
        ftxui::Color::RGB(0x26, 0x8a, 0xd2), // 4: Blue
        ftxui::Color::RGB(0xd3, 0x36, 0x82), // 5: Magenta
        ftxui::Color::RGB(0x2a, 0xa1, 0xa3), // 6: Cyan
        ftxui::Color::RGB(0xee, 0xee, 0xee), // 7: White

        // 亮色 (8-15)
        ftxui::Color::RGB(0x55, 0x55, 0x55), // 8: Bright Black (Gray)
        ftxui::Color::RGB(0xff, 0x6a, 0x00), // 9: Bright Red
        ftxui::Color::RGB(0x5a, 0xfd, 0x57), // 10: Bright Green
        ftxui::Color::RGB(0xff, 0xff, 0x5f), // 11: Bright Yellow
        ftxui::Color::RGB(0x5f, 0x87, 0xff), // 12: Bright Blue
        ftxui::Color::RGB(0xff, 0x55, 0xff), // 13: Bright Magenta
        ftxui::Color::RGB(0x5a, 0xff, 0xff), // 14: Bright Cyan
        ftxui::Color::RGB(0xff, 0xff, 0xff)  // 15: Bright White
    };

    if (ansi_code >= 0 && ansi_code < static_cast<int>(ansi_colors.size())) {
        return ansi_colors[ansi_code];
    }
    return ftxui::Color::Default;
}

ftxui::Color AnsiColorParser::ansi256ColorToFtxui(int color_code) {
    // 使用预构建的 256 色表（高性能缓存）
    const auto& table = get256ColorTable();

    if (color_code >= 0 && color_code < 256) {
        return table[color_code];
    }

    // 超出范围返回默认色
    return ftxui::Color::Default;
}

ftxui::Color AnsiColorParser::rgbColorToFtxui(int r, int g, int b) {
    return ftxui::Color::RGB(r, g, b);
}

std::vector<int> AnsiColorParser::parseCsiParams(const std::string& params_str) {
    std::vector<int> params;
    std::istringstream iss(params_str);
    std::string param;

    while (std::getline(iss, param, ';')) {
        if (!param.empty()) {
            try {
                params.push_back(std::stoi(param));
            } catch (...) {
                // 忽略无效参数
            }
        } else {
            params.push_back(0); // 空参数默认为0
        }
    }

    return params;
}

} // namespace terminal
} // namespace features
} // namespace pnana
