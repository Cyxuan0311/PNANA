#ifndef PNANA_UI_SPLIT_DIALOG_H
#define PNANA_UI_SPLIT_DIALOG_H

#include "features/split_view.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>

namespace pnana {
namespace ui {

// 分屏信息
struct SplitInfo {
    size_t region_index;
    size_t document_index;
    std::string document_name;
    bool is_active;
    bool is_modified;

    SplitInfo(size_t reg_idx, size_t doc_idx, const std::string& name, bool active, bool modified)
        : region_index(reg_idx), document_index(doc_idx), document_name(name), is_active(active),
          is_modified(modified) {}
};

// 分屏选择对话框
class SplitDialog {
  public:
    explicit SplitDialog(Theme& theme);

    // 显示创建分屏对话框
    void showCreate(std::function<void(features::SplitDirection)> on_select = nullptr,
                    std::function<void()> on_cancel = nullptr);

    // 显示关闭分屏对话框
    void showClose(const std::vector<SplitInfo>& splits,
                   std::function<void(size_t)> on_close = nullptr,
                   std::function<void()> on_cancel = nullptr);

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }
    void setVisible(bool visible) {
        visible_ = visible;
    }

    // 重置
    void reset();

  private:
    enum class DialogMode {
        CREATE, // 创建分屏模式
        CLOSE   // 关闭分屏模式
    };

    Theme& theme_;
    bool visible_;
    DialogMode mode_;
    size_t selected_index_;

    // 创建模式
    std::function<void(features::SplitDirection)> on_create_select_;
    std::function<void()> on_create_cancel_;

    // 关闭模式
    std::vector<SplitInfo> splits_;
    std::function<void(size_t)> on_close_;
    std::function<void()> on_close_cancel_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SPLIT_DIALOG_H
