#ifndef PNANA_UI_PACKAGE_INSTALL_DIALOG_H
#define PNANA_UI_PACKAGE_INSTALL_DIALOG_H

#include "features/package_manager/package_manager_base.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <memory>
#include <mutex>
#include <string>

namespace pnana {
namespace ui {

// 安装包对话框
class PackageInstallDialog {
  public:
    explicit PackageInstallDialog(Theme& theme);
    ~PackageInstallDialog() = default;

    // 显示安装对话框
    void show(std::shared_ptr<features::package_manager::PackageManagerBase> manager);

    // 隐藏安装对话框
    void hide();

    // 是否可见
    bool isVisible() const {
        return visible_;
    }

    // 处理输入
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render() const;

  private:
    Theme& theme_;
    bool visible_;
    std::shared_ptr<features::package_manager::PackageManagerBase> manager_;

    // 包名输入
    std::string package_name_input_;
    size_t cursor_position_;

    // 操作状态（线程安全）
    mutable std::mutex operation_mutex_;
    std::string operation_status_; // 操作状态消息
    bool operation_in_progress_;   // 操作是否进行中
    bool operation_success_;       // 操作是否成功（仅在完成时有效）

    // 输入处理
    void insertChar(char ch);
    void backspace();
    void deleteChar();
    void moveCursorLeft();
    void moveCursorRight();

    // 渲染输入字段
    ftxui::Element renderInputField() const;

    // 渲染操作状态
    ftxui::Element renderOperationStatus() const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_PACKAGE_INSTALL_DIALOG_H
