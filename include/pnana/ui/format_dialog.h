#ifndef PNANA_UI_FORMAT_DIALOG_H
#define PNANA_UI_FORMAT_DIALOG_H

#include "utils/file_type_color_mapper.h"
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

class Theme;

/**
 * 代码格式化对话框
 * 显示可格式化文件列表，允许用户选择要格式化的文件
 */
class FormatDialog {
  public:
    explicit FormatDialog(Theme& theme);
    ~FormatDialog() = default;

    /**
     * 打开对话框
     * @param files 可格式化的文件列表
     * @param directory_path 目录路径
     */
    void open(const std::vector<std::string>& files, const std::string& directory_path);

    /**
     * 关闭对话框
     */
    void close();

    /**
     * 检查对话框是否打开
     * @return 是否打开
     */
    bool isOpen() const;

    /**
     * 处理输入事件
     * @param event 输入事件
     * @return 是否处理了事件
     */
    bool handleInput(ftxui::Event event);

    /**
     * 渲染对话框
     * @return FTXUI 元素
     */
    ftxui::Element render();

    /**
     * 设置确认回调
     * @param callback 确认回调函数，参数为选中的文件列表
     */
    void setOnConfirm(std::function<void(const std::vector<std::string>&)> callback);

    /**
     * 设置取消回调
     * @param callback 取消回调函数
     */
    void setOnCancel(std::function<void()> callback);

    /**
     * 获取所有文件列表
     * @return 文件列表
     */
    const std::vector<std::string>& getFiles() const;

    /**
     * 获取选中的文件列表
     * @return 选中的文件列表
     */
    std::vector<std::string> getSelectedFiles() const;

    /**
     * 获取过滤后的文件列表（根据搜索查询）
     * @return 过滤后的文件列表
     */
    std::vector<std::string> getFilteredFiles() const;

    /**
     * 从文件路径中提取文件名
     * @param file_path 文件路径
     * @return 文件名
     */
    std::string getFileName(const std::string& file_path) const;

  private:
    /**
     * 切换文件选中状态
     * @param index 文件索引
     */
    void toggleSelection(size_t index);

    /**
     * 获取文件扩展名
     * @param filename 文件名
     * @return 扩展名
     */
    std::string getFileExtension(const std::string& filename) const;

    Theme& theme_;
    std::unique_ptr<pnana::utils::FileTypeColorMapper> color_mapper_;
    bool is_open_;
    std::vector<std::string> files_;
    std::string directory_path_;
    std::set<size_t> selected_files_; // 选中的文件索引集合
    size_t selected_index_;           // 当前选中的索引（用于导航）
    size_t scroll_offset_;            // 滚动偏移量（用于按需加载）
    size_t max_visible_items_;        // 最大可见项目数
    std::string search_query_;        // 搜索查询字符串
    bool search_focused_;             // 搜索框是否获得焦点

    std::function<void(const std::vector<std::string>&)> on_confirm_;
    std::function<void()> on_cancel_;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_FORMAT_DIALOG_H
