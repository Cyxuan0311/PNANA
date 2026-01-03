#ifndef PNANA_UI_ENCODING_DIALOG_H
#define PNANA_UI_ENCODING_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 编码选择对话框类
class EncodingDialog {
  public:
    explicit EncodingDialog(Theme& theme);

    // 打开对话框
    void open(const std::string& current_encoding = "UTF-8");

    // 关闭对话框
    void close();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 设置确认回调
    void setOnConfirm(std::function<void(const std::string&)> callback) {
        on_confirm_ = callback;
    }

    // 设置取消回调
    void setOnCancel(std::function<void()> callback) {
        on_cancel_ = callback;
    }

    // 获取当前选中的编码
    std::string getSelectedEncoding() const;

  private:
    Theme& theme_;
    bool visible_;
    std::vector<std::string> encodings_;
    size_t selected_index_;
    std::string current_encoding_;

    std::function<void(const std::string&)> on_confirm_;
    std::function<void()> on_cancel_;

    void selectNext();
    void selectPrevious();
    void executeSelected();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_ENCODING_DIALOG_H
