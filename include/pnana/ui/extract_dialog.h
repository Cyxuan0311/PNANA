#ifndef PNANA_UI_EXTRACT_DIALOG_H
#define PNANA_UI_EXTRACT_DIALOG_H

#include "features/extract.h"
#include "ui/theme.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 解压文件选择对话框
class ExtractDialog {
  public:
    explicit ExtractDialog(Theme& theme);

    // 显示对话框
    void show(const std::string& current_directory,
              std::function<void(const features::ArchiveFile&)> on_select,
              std::function<void()> on_cancel);

    // 隐藏对话框
    void hide();

    // 检查是否可见
    bool isVisible() const {
        return visible_;
    }

    // 处理输入事件
    bool handleInput(ftxui::Event event);

    // 渲染对话框
    ftxui::Element render();

    // 设置压缩文件列表
    void setArchiveFiles(const std::vector<features::ArchiveFile>& files);

  private:
    Theme& theme_;
    bool visible_;
    std::vector<features::ArchiveFile> archive_files_;
    size_t selected_index_;
    size_t scroll_offset_;

    std::function<void(const features::ArchiveFile&)> on_select_;
    std::function<void()> on_cancel_;

    // 渲染辅助方法
    ftxui::Element renderFileList();
    ftxui::Element renderHelpBar();
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_EXTRACT_DIALOG_H
