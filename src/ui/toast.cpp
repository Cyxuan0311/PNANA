#include "ui/toast.h"
#include <cmath>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace ui {

// 静态成员初始化（默认禁用 Toast）
bool Toast::s_enabled_ = false;
ToastStyle Toast::s_default_style_ = ToastStyle::CLASSIC;
int Toast::s_default_duration_ms_ = 3000;
int Toast::s_default_max_width_ = 50;
bool Toast::s_default_show_icon_ = true;
bool Toast::s_default_bold_text_ = false;

Toast::Toast() = default;
Toast::~Toast() = default;

void Toast::show(const ToastConfig& config) {
    // 如果全局禁用，直接返回
    if (!s_enabled_) {
        return;
    }

    current_config_ = config;
    visible_ = true;
    fading_out_ = false;
    show_time_ = std::chrono::steady_clock::now();
}

void Toast::showSuccess(const std::string& message, int duration_ms) {
    show(ToastConfig::success(message, duration_ms));
}

void Toast::showInfo(const std::string& message, int duration_ms) {
    show(ToastConfig::info(message, duration_ms));
}

void Toast::showWarning(const std::string& message, int duration_ms) {
    show(ToastConfig::warning(message, duration_ms));
}

void Toast::showError(const std::string& message, int duration_ms) {
    show(ToastConfig::error(message, duration_ms));
}

void Toast::hide() {
    visible_ = false;
    fading_out_ = false;
}

ToastStyle Toast::resolveStyle() const {
    if (current_config_.style == ToastStyle::AUTO) {
        return s_default_style_;
    }
    return current_config_.style;
}

int Toast::resolveDurationMs() const {
    if (current_config_.duration_ms >= 0) {
        return current_config_.duration_ms;
    }
    return s_default_duration_ms_;
}

int Toast::resolveMaxWidth() const {
    if (current_config_.max_width > 0) {
        return current_config_.max_width;
    }
    return s_default_max_width_ > 0 ? s_default_max_width_ : 50;
}

void Toast::update() {
    if (!visible_) {
        return;
    }

    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - show_time_);

    int effective_duration = resolveDurationMs();

    // 如果 duration_ms 为 0，不自动消失
    if (effective_duration == 0) {
        return;
    }

    // 开始淡出（在持续时间到达时）
    if (!fading_out_ && elapsed.count() >= effective_duration) {
        fading_out_ = true;
        fade_start_time_ = now;
    }

    // 完全隐藏（淡出动画完成后）
    if (fading_out_) {
        auto fade_elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(now - fade_start_time_);
        if (fade_elapsed.count() >= FADE_OUT_DURATION_MS) {
            hide();
        }
    }
}

const char* Toast::getIcon(ToastType type) const {
    switch (type) {
        case ToastType::SUCCESS:
            return icons::CHECK_CIRCLE;
        case ToastType::INFO:
            return icons::INFO_CIRCLE;
        case ToastType::WARNING:
            return icons::WARNING;
        case ToastType::ERROR:
            return icons::ERROR;
        default:
            return icons::INFO_CIRCLE;
    }
}

Color Toast::getIconColor(ToastType type, const ThemeColors& colors) const {
    switch (type) {
        case ToastType::SUCCESS:
            return colors.success;
        case ToastType::INFO:
            return colors.info;
        case ToastType::WARNING:
            return colors.warning;
        case ToastType::ERROR:
            return colors.error;
        default:
            return colors.info;
    }
}

Color Toast::getBorderColor(ToastType type, const ThemeColors& colors) const {
    switch (type) {
        case ToastType::SUCCESS:
            return colors.success;
        case ToastType::INFO:
            return colors.info;
        case ToastType::WARNING:
            return colors.warning;
        case ToastType::ERROR:
            return colors.error;
        default:
            return colors.dialog_border;
    }
}

Color Toast::getTypeColor(ToastType type, const ThemeColors& colors) const {
    switch (type) {
        case ToastType::SUCCESS:
            return colors.success;
        case ToastType::INFO:
            return colors.info;
        case ToastType::WARNING:
            return colors.warning;
        case ToastType::ERROR:
            return colors.error;
        default:
            return colors.info;
    }
}

Element Toast::render(const Theme& theme) const {
    if (!visible_) {
        return text("");
    }

    const auto& colors = theme.getColors();

    const ToastStyle style = resolveStyle();
    const char* icon = getIcon(current_config_.type);
    Color icon_color = getIconColor(current_config_.type, colors);
    Color border_color = getBorderColor(current_config_.type, colors);
    Color type_color = getTypeColor(current_config_.type, colors);
    Color text_color = colors.foreground;
    int max_width = resolveMaxWidth();

    Elements content;

    // 图标显示规则：受全局开关和当前 toast 开关共同控制
    bool use_icon = s_default_show_icon_ && current_config_.show_icon;

    if (use_icon) {
        content.push_back(text(icon) | color(icon_color) | bold);
        content.push_back(text(" "));
    }

    // 将消息按最大宽度换行显示（保留单词完整性）
    auto wrapText = [](const std::string& s, int width) {
        std::vector<std::string> lines;
        if (width <= 0) {
            lines.push_back(s);
            return lines;
        }
        std::istringstream iss(s);
        std::string word;
        std::string cur;
        while (iss >> word) {
            if ((int)cur.size() + (int)word.size() + (cur.empty() ? 0 : 1) <= width) {
                if (!cur.empty())
                    cur += ' ';
                cur += word;
            } else {
                if (!cur.empty()) {
                    lines.push_back(cur);
                    cur.clear();
                }
                // 如果单词本身比宽度长，硬切分该单词
                if ((int)word.size() > width) {
                    size_t pos = 0;
                    while (pos < word.size()) {
                        lines.push_back(word.substr(pos, width));
                        pos += width;
                    }
                } else {
                    cur = word;
                }
            }
        }
        if (!cur.empty())
            lines.push_back(cur);
        // 保留原始换行符：如果输入包含 '\n', split those lines and re-wrap each
        // (简单处理：如果 original string contains '\n', rebuild from raw lines)
        if (s.find('\n') != std::string::npos) {
            lines.clear();
            std::string tmp;
            std::istringstream iss2(s);
            while (std::getline(iss2, tmp)) {
                // wrap each tmp line
                std::istringstream iss3(tmp);
                std::string w;
                std::string cur2;
                while (iss3 >> w) {
                    if ((int)cur2.size() + (int)w.size() + (cur2.empty() ? 0 : 1) <= width) {
                        if (!cur2.empty())
                            cur2 += ' ';
                        cur2 += w;
                    } else {
                        if (!cur2.empty()) {
                            lines.push_back(cur2);
                            cur2.clear();
                        }
                        if ((int)w.size() > width) {
                            size_t pos = 0;
                            while (pos < w.size()) {
                                lines.push_back(w.substr(pos, width));
                                pos += width;
                            }
                        } else {
                            cur2 = w;
                        }
                    }
                }
                if (!cur2.empty())
                    lines.push_back(cur2);
                // Preserve explicit blank line
                if (tmp.empty())
                    lines.push_back("");
            }
        }
        return lines;
    };

    int effective_max = max_width;
    // 留出一些边距供图标和左右空格使用
    int available_width = std::max(1, effective_max - 6);

    std::vector<std::string> wrapped = wrapText(current_config_.message, available_width);
    Elements msg_lines;
    for (size_t i = 0; i < wrapped.size(); ++i) {
        msg_lines.push_back(text(wrapped[i]) | color(text_color));
    }
    Element message_text = vbox(std::move(msg_lines)) | flex;
    content.push_back(message_text);

    bool use_bold = current_config_.bold_text || s_default_bold_text_ ||
                    style == ToastStyle::CLASSIC || style == ToastStyle::OUTLINE ||
                    style == ToastStyle::ACCENT;

    Element toast_content = hbox(std::move(content));
    if (use_bold) {
        toast_content = toast_content | bold;
    }

    // 统一留白，增强不同样式的视觉差异
    Element padded_content = hbox({text(" "), toast_content, text(" ")});

    Element toast_element;
    switch (style) {
        case ToastStyle::MINIMAL: {
            // 极简：无边框，无背景，仅类型色文本
            toast_element = padded_content | color(type_color) | size(WIDTH, LESS_THAN, max_width);
            break;
        }
        case ToastStyle::SOLID: {
            // 实色：整块填充类型色，白字高对比
            toast_element = vbox({padded_content}) | bgcolor(type_color) | color(Color::White) |
                            size(WIDTH, LESS_THAN, max_width);
            break;
        }
        case ToastStyle::ACCENT: {
            // 强调：左侧彩条 + 细边框 + 轻底色
            Element accent_bar = text(" ") | bgcolor(type_color) | size(WIDTH, EQUAL, 2);
            Element panel =
                hbox({text(" "), toast_content, text(" ")}) | bgcolor(colors.current_line);
            toast_element = hbox({accent_bar, panel}) | border | color(border_color) |
                            size(WIDTH, LESS_THAN, max_width);
            break;
        }
        case ToastStyle::OUTLINE: {
            // 线框：双线边框，透明背景
            toast_element = vbox({padded_content}) | borderDouble | color(border_color) |
                            size(WIDTH, LESS_THAN, max_width);
            break;
        }
        case ToastStyle::AUTO:
        case ToastStyle::CLASSIC:
        default: {
            // 经典：单线边框 + 轻底色
            toast_element = vbox({padded_content}) | border | color(border_color) |
                            bgcolor(colors.current_line) | size(WIDTH, LESS_THAN, max_width);
            break;
        }
    }

    if (fading_out_) {
        toast_element = toast_element | dim;
    }

    return toast_element;
}

} // namespace ui
} // namespace pnana
