#ifndef PNANA_UI_SSH_TRANSFER_DIALOG_H
#define PNANA_UI_SSH_TRANSFER_DIALOG_H

#include "ui/theme.h"
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

struct SSHTransferItem {
    std::string local_path;
    std::string remote_path;
    std::string direction; // "upload" 或 "download"
    std::string status;    // "pending", "in_progress", "completed", "error"
    std::string error_message;
    size_t file_size;
    size_t transferred_size;
};

class SSHTransferDialog {
  public:
    SSHTransferDialog(Theme& theme);

    void show(std::function<void(const std::vector<SSHTransferItem>&)> on_start_transfer,
              std::function<void()> on_cancel);

    void hide();

    bool isVisible() const {
        return visible_;
    }

    bool handleInput(const ftxui::Event& event);

    ftxui::Element render();

    // 添加传输项目
    void addTransferItem(const SSHTransferItem& item);

    // 更新传输进度
    void updateProgress(const std::string& local_path, size_t transferred);

    // 设置传输状态
    void setTransferStatus(const std::string& local_path, const std::string& status,
                           const std::string& error_message = "");

    // 清空传输列表
    void clearTransfers();

  private:
    Theme& theme_;
    bool visible_;
    int current_field_;
    size_t cursor_position_;

    // UI 字段
    std::string local_path_input_;
    std::string remote_path_input_;
    std::string direction_; // "upload" 或 "download"

    // 传输队列
    std::vector<SSHTransferItem> transfer_items_;

    // 回调函数
    std::function<void(const std::vector<SSHTransferItem>&)> on_start_transfer_;
    std::function<void()> on_cancel_;

    // 辅助方法
    void addCurrentItem();
    void removeItem(size_t index);
    void moveToNextField();
    void moveToPreviousField();
    void insertChar(char ch);
    void deleteChar();
    void backspace();
    void moveCursorLeft();
    void moveCursorRight();
    std::string* getCurrentField();

    // 渲染辅助方法
    ftxui::Element renderTransferList();
    ftxui::Element renderProgressBar(const SSHTransferItem& item);
    ftxui::Element renderField(const std::string& label, std::string& value, size_t field_idx);
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_SSH_TRANSFER_DIALOG_H
