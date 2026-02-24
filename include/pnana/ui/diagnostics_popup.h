#ifndef PNANA_UI_DIAGNOSTICS_POPUP_H
#define PNANA_UI_DIAGNOSTICS_POPUP_H

#include "features/lsp/lsp_client.h"
#include "ui/theme.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class DiagnosticsPopup {
  public:
    DiagnosticsPopup(Theme& theme);

    // 设置诊断信息
    void setDiagnostics(const std::vector<pnana::features::Diagnostic>& diagnostics);

    // 显示/隐藏弹窗
    void show();
    void hide();
    bool isVisible() const;

    // 导航
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();

    // 获取当前选中的诊断
    const pnana::features::Diagnostic* getSelectedDiagnostic() const;

    // 复制当前选中的诊断信息
    std::string getSelectedDiagnosticText() const;

    // 获取诊断类型的字符串表示（public方法）
    std::string getSeverityString(int severity) const;

    // 设置跳转回调
    void setJumpCallback(std::function<void(const pnana::features::Diagnostic&)> callback);

    // 跳转到选中的诊断位置
    void jumpToSelectedDiagnostic();

    // 输入处理
    bool handleInput(ftxui::Event event);

    // 设置复制回调
    void setCopyCallback(std::function<void(const std::string&)> callback);

    // 渲染
    ftxui::Element render() const;

    // 获取诊断数量
    size_t getDiagnosticCount() const;

    // 获取错误数量
    size_t getErrorCount() const;
    size_t getWarningCount() const;

  private:
    Theme& theme_;
    std::vector<pnana::features::Diagnostic> diagnostics_;
    size_t selected_index_;
    bool visible_;

    // 跳转回调函数
    std::function<void(const pnana::features::Diagnostic&)> jump_callback_;

    // 复制回调函数
    std::function<void(const std::string&)> copy_callback_;

    // 渲染单个诊断项（index 为 1-based 序号，total 为总数用于显示 x/N）
    ftxui::Element renderDiagnosticItem(const pnana::features::Diagnostic& diagnostic,
                                        bool is_selected, size_t index, size_t total) const;

    // 获取诊断类型的颜色
    ftxui::Color getSeverityColor(int severity) const;

    // 格式化诊断信息为可复制的文本
    std::string formatDiagnosticText(const pnana::features::Diagnostic& diagnostic) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_DIAGNOSTICS_POPUP_H
