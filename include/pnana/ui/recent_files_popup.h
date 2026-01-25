#ifndef PNANA_UI_RECENT_FILES_POPUP_H
#define PNANA_UI_RECENT_FILES_POPUP_H

#include "features/recent_files_manager.h"
#include "ui/theme.h"
#include "utils/file_type_icon_mapper.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// 最近文件弹窗类
class RecentFilesPopup {
  public:
    explicit RecentFilesPopup(Theme& theme);

    // 设置数据
    void setData(bool is_open, const std::vector<features::ProjectItem>& recent_projects,
                 size_t selected_index);

    // 渲染弹窗
    ftxui::Element render();

    // 事件处理
    bool handleInput(ftxui::Event event);

    // 获取当前选中的文件索引
    size_t getSelectedIndex() const {
        return selected_index_;
    }

    // 检查是否打开
    bool isOpen() const {
        return is_open_;
    }

    // 打开/关闭弹窗
    void open();
    void close();

    // 设置文件打开回调
    void setFileOpenCallback(std::function<void(size_t)> callback);

  private:
    Theme& theme_;
    bool is_open_;
    std::vector<features::ProjectItem> recent_projects_;
    size_t selected_index_;
    utils::FileTypeIconMapper icon_mapper_;

    // 文件打开回调
    std::function<void(size_t)> file_open_callback_;

    // 渲染各个组件
    ftxui::Element renderTitle() const;
    ftxui::Element renderFileList() const;
    ftxui::Element renderHelpBar() const;

    // 辅助方法
    std::string getFileName(const std::string& filepath) const;
    std::string getFilePath(const std::string& filepath) const;
    std::string getFileIcon(const std::string& filepath) const;

    // 项目相关辅助方法
    std::string getProjectIcon(const features::ProjectItem& project) const;
    std::string getProjectName(const features::ProjectItem& project) const;
    std::string getProjectTypeString(const features::ProjectItem& project) const;
    std::string getProjectPath(const features::ProjectItem& project) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_RECENT_FILES_POPUP_H
