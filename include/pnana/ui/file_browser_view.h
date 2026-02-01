#ifndef PNANA_UI_FILE_BROWSER_VIEW_H
#define PNANA_UI_FILE_BROWSER_VIEW_H

#include "features/file_browser.h"
#include "ui/theme.h"
#include "utils/file_type_color_mapper.h"
#include <ftxui/dom/elements.hpp>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 文件浏览器视图类 - 负责 UI 渲染
class FileBrowserView {
  public:
    explicit FileBrowserView(Theme& theme);

    // 渲染文件浏览器
    ftxui::Element render(const features::FileBrowser& browser, int height);

    // 滚动控制
    void scrollTo(size_t index);
    void scrollUp(size_t lines = 1);
    void scrollDown(size_t lines = 1);
    void scrollToTop();
    void scrollToBottom();

  private:
    Theme& theme_;
    utils::FileTypeColorMapper color_mapper_;
    size_t scroll_offset_; // 当前滚动偏移量（显示的第一个项目的索引）

    // UI 辅助方法
    std::string getFileIcon(const features::FileItem& item) const;
    std::string getFileExtension(const std::string& filename) const;
    std::string truncateMiddle(const std::string& str, size_t max_length) const;

    // 渲染辅助方法
    ftxui::Element renderHeader(const std::string& current_directory) const;
    ftxui::Element renderFileList(const features::FileBrowser& browser, size_t visible_start,
                                  size_t visible_count) const;
    ftxui::Element renderStatusBar(const features::FileBrowser& browser) const;
    ftxui::Element renderFileItem(const features::FileItem* item, size_t index,
                                  size_t selected_index,
                                  const std::vector<features::FileItem*>& flat_items,
                                  const features::FileBrowser& browser) const;
    std::string buildTreePrefix(const features::FileItem* item, size_t index,
                                const std::vector<features::FileItem*>& flat_items) const;
    std::string buildExpandPrefix(const features::FileItem* item, size_t index,
                                  const std::vector<features::FileItem*>& flat_items) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_FILE_BROWSER_VIEW_H
