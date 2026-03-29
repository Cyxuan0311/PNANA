#ifndef PNANA_FEATURES_TERMINAL_COLOR_H
#define PNANA_FEATURES_TERMINAL_COLOR_H

#include <array>
#include <cstdint>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace terminal {

// ANSI 颜色配置（可自定义主题）
struct AnsiColorPalette {
    std::array<uint8_t, 3> colors[16];

    // 使用默认的 ANSI 颜色（Solarized 风格）
    static AnsiColorPalette defaultPalette();

    // 从配置加载
    static AnsiColorPalette fromConfig(const std::string& theme_name);
};

// ANSI 颜色解析器
class AnsiColorParser {
  public:
    // 解析 ANSI 颜色码并返回格式化的 FTXUI 元素
    static ftxui::Element parse(const std::string& text);

    // 检查文本是否包含 ANSI 颜色码
    static bool hasAnsiCodes(const std::string& text);

    // 移除 ANSI 颜色码，返回纯文本
    static std::string stripAnsiCodes(const std::string& text);

    // 颜色转换（公开以便复用）
    static ftxui::Color ansiColorToFtxui(int ansi_code);
    static ftxui::Color ansi256ColorToFtxui(int color_code);
    static ftxui::Color rgbColorToFtxui(int r, int g, int b);

    // 获取 256 色表（用于缓存）
    static const std::array<ftxui::Color, 256>& get256ColorTable();

  private:
    // ANSI 转义序列状态
    enum class ParseState { Normal, Escape, CSI, OSC };

    // 解析 CSI 序列参数
    static std::vector<int> parseCsiParams(const std::string& params);
};

} // namespace terminal
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_TERMINAL_COLOR_H
