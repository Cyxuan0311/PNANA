#include "ui/diagnostics_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <sstream>

using namespace ftxui;
using namespace pnana::ui::icons;

namespace pnana {
namespace ui {

DiagnosticsPopup::DiagnosticsPopup(Theme& theme)
    : theme_(theme), selected_index_(0), visible_(false), jump_callback_(nullptr),
      copy_callback_(nullptr) {}

void DiagnosticsPopup::setDiagnostics(const std::vector<pnana::features::Diagnostic>& diagnostics) {
    diagnostics_ = diagnostics;
    selected_index_ = diagnostics_.empty() ? 0 : 0;
}

void DiagnosticsPopup::show() {
    visible_ = true;
    selected_index_ = diagnostics_.empty() ? 0 : 0;
}

void DiagnosticsPopup::hide() {
    visible_ = false;
}

bool DiagnosticsPopup::isVisible() const {
    return visible_;
}

void DiagnosticsPopup::selectNext() {
    if (!diagnostics_.empty()) {
        selected_index_ = (selected_index_ + 1) % diagnostics_.size();
    }
}

void DiagnosticsPopup::selectPrevious() {
    if (!diagnostics_.empty()) {
        selected_index_ = (selected_index_ + diagnostics_.size() - 1) % diagnostics_.size();
    }
}

void DiagnosticsPopup::selectFirst() {
    selected_index_ = diagnostics_.empty() ? 0 : 0;
}

void DiagnosticsPopup::selectLast() {
    selected_index_ = diagnostics_.empty() ? 0 : diagnostics_.size() - 1;
}

const pnana::features::Diagnostic* DiagnosticsPopup::getSelectedDiagnostic() const {
    if (diagnostics_.empty() || selected_index_ >= diagnostics_.size()) {
        return nullptr;
    }
    return &diagnostics_[selected_index_];
}

std::string DiagnosticsPopup::getSelectedDiagnosticText() const {
    const auto* diagnostic = getSelectedDiagnostic();
    if (!diagnostic) {
        return "";
    }
    return formatDiagnosticText(*diagnostic);
}

Element DiagnosticsPopup::render() const {
    if (!visible_ || diagnostics_.empty()) {
        return text("");
    }

    auto& colors = theme_.getColors();

    Elements content;

    // 标题栏（参考其他弹窗样式）
    Elements title_elements;
    title_elements.push_back(text(pnana::ui::icons::WARNING) | color(colors.warning));
    title_elements.push_back(text(" LSP Diagnostics ") | color(colors.foreground) | bold);
    content.push_back(hbox(title_elements) | center);

    content.push_back(separator());

    // 诊断项列表
    Elements items;
    size_t max_display = 8; // 减少显示数量，让弹窗更紧凑

    // 如果选中项不在前8个中，滚动显示
    size_t start_idx = 0;
    if (selected_index_ >= max_display) {
        start_idx = selected_index_ - max_display + 1;
    }

    size_t end_idx = std::min(start_idx + max_display, diagnostics_.size());

    for (size_t i = start_idx; i < end_idx; ++i) {
        bool is_selected = (i == selected_index_);
        items.push_back(renderDiagnosticItem(diagnostics_[i], is_selected));
    }

    content.push_back(vbox(items) | vscroll_indicator | frame | size(HEIGHT, LESS_THAN, 12));

    // 统计信息
    std::string stats = std::to_string(diagnostics_.size()) + " diagnostics";
    content.push_back(separator());
    content.push_back(text(stats) | dim | center);

    // 帮助信息
    content.push_back(separator());
    content.push_back(text("↑↓ Navigate | Enter Jump | Ctrl+P Copy | Esc Close | Alt+E Close") |
                      dim | center);

    Element dialog_content = vbox(content);

    // 使用window样式，参考search_dialog
    return window(text("Diagnostics"), dialog_content) | size(WIDTH, GREATER_THAN, 70) |
           size(HEIGHT, GREATER_THAN, 12) | bgcolor(colors.background) |
           color(colors.dialog_border) | border;
}

Element DiagnosticsPopup::renderDiagnosticItem(const pnana::features::Diagnostic& diagnostic,
                                               bool is_selected) const {
    // 严重程度图标
    std::string severity_icon;
    Color severity_color = getSeverityColor(diagnostic.severity);

    switch (diagnostic.severity) {
        case 1: // ERROR
            severity_icon = ERROR;
            break;
        case 2: // WARNING
            severity_icon = WARNING;
            break;
        case 3: // INFORMATION
            severity_icon = INFO;
            break;
        case 4: // HINT
            severity_icon = BULB;
            break;
        default:
            severity_icon = "?";
            break;
    }

    // 构建诊断文本
    std::string location = "[" + std::to_string(diagnostic.range.start.line + 1) + ":" +
                           std::to_string(diagnostic.range.start.character + 1) + "] ";

    std::string severity_str =
        getSeverityString(static_cast<pnana::features::DiagnosticSeverity>(diagnostic.severity));

    // 限制消息长度
    std::string message = diagnostic.message;
    if (message.length() > 80) {
        message = message.substr(0, 77) + "...";
    }

    std::string full_text = severity_icon + " " + severity_str + " " + location + message;

    // 应用样式
    Element element = text(full_text);
    if (is_selected) {
        element = element | bgcolor(theme_.getColors().current_line) |
                  color(theme_.getColors().foreground);
    } else {
        element = element | color(severity_color);
    }

    return element;
}

std::string DiagnosticsPopup::getSeverityString(int severity) const {
    switch (severity) {
        case 1: // ERROR
            return "Error";
        case 2: // WARNING
            return "Warning";
        case 3: // INFORMATION
            return "Info";
        case 4: // HINT
            return "Hint";
        default:
            return "Unknown";
    }
}

Color DiagnosticsPopup::getSeverityColor(int severity) const {
    auto& colors = theme_.getColors();
    switch (severity) {
        case 1: // ERROR
            return colors.error;
        case 2: // WARNING
            return colors.warning;
        case 3: // INFORMATION
            return colors.info;
        case 4: // HINT
            return colors.success;
        default:
            return colors.foreground;
    }
}

std::string DiagnosticsPopup::formatDiagnosticText(
    const pnana::features::Diagnostic& diagnostic) const {
    std::stringstream ss;

    ss << "Location: Line " << (diagnostic.range.start.line + 1) << ", Column "
       << (diagnostic.range.start.character + 1) << "\n";
    ss << "Type: "
       << getSeverityString(static_cast<pnana::features::DiagnosticSeverity>(diagnostic.severity))
       << "\n";
    ss << "Message: " << diagnostic.message << "\n";

    if (!diagnostic.code.empty()) {
        ss << "Code: " << diagnostic.code << "\n";
    }

    if (!diagnostic.source.empty()) {
        ss << "Source: " << diagnostic.source << "\n";
    }

    return ss.str();
}

size_t DiagnosticsPopup::getDiagnosticCount() const {
    return diagnostics_.size();
}

size_t DiagnosticsPopup::getErrorCount() const {
    return std::count_if(diagnostics_.begin(), diagnostics_.end(),
                         [](const pnana::features::Diagnostic& d) {
                             return d.severity == pnana::features::DiagnosticSeverity::ERROR;
                         });
}

size_t DiagnosticsPopup::getWarningCount() const {
    return std::count_if(diagnostics_.begin(), diagnostics_.end(),
                         [](const pnana::features::Diagnostic& d) {
                             return d.severity == 2; // WARNING
                         });
}

void DiagnosticsPopup::setJumpCallback(
    std::function<void(const pnana::features::Diagnostic&)> callback) {
    jump_callback_ = std::move(callback);
}

void DiagnosticsPopup::jumpToSelectedDiagnostic() {
    if (jump_callback_ && !diagnostics_.empty() && selected_index_ < diagnostics_.size()) {
        jump_callback_(diagnostics_[selected_index_]);
    }
}

bool DiagnosticsPopup::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    // 处理导航键
    if (event == Event::ArrowUp) {
        selectPrevious();
        return true;
    } else if (event == Event::ArrowDown) {
        selectNext();
        return true;
    } else if (event == Event::Home) {
        selectFirst();
        return true;
    } else if (event == Event::End) {
        selectLast();
        return true;
    } else if (event == Event::Return) {
        // Enter 键：跳转到选中的诊断
        jumpToSelectedDiagnostic();
        return true;
    } else if (event == Event::Escape) {
        // Esc 键：隐藏弹窗
        hide();
        return true;
    }

    // 处理 Ctrl+P (复制选中的诊断信息)
    if (event == Event::Special({27, 80, 0}) ||                  // Ctrl+P
        (event.is_character() && event.character() == "\x10")) { // Ctrl+P 的另一种表示
        // 这里我们需要一个回调来处理复制操作
        // 由于 DiagnosticsPopup 没有直接访问 Editor 的权限，我们通过回调处理
        if (copy_callback_) {
            copy_callback_(getSelectedDiagnosticText());
        }
        return true;
    }

    return false;
}

void DiagnosticsPopup::setCopyCallback(std::function<void(const std::string&)> callback) {
    copy_callback_ = std::move(callback);
}

} // namespace ui
} // namespace pnana
