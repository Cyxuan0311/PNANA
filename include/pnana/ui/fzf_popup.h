#ifndef PNANA_UI_FZF_POPUP_H
#define PNANA_UI_FZF_POPUP_H

#include "features/SyntaxHighlighter/syntax_highlighter.h"
#include "ui/theme.h"
#include "utils/file_type_color_mapper.h"
#include "utils/file_type_icon_mapper.h"
#include <filesystem>
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <string>
#include <vector>

namespace pnana {
namespace ui {

// FZF风格的模糊文件查找弹窗
class FzfPopup {
  public:
    explicit FzfPopup(Theme& theme);

    // 打开/关闭弹窗
    void open();
    void close();

    // 检查是否打开
    bool isOpen() const {
        return is_open_;
    }

    // 渲染弹窗
    ftxui::Element render();

    // 事件处理
    bool handleInput(ftxui::Event event);

    // 设置文件打开回调（选中文件后调用，参数为文件路径）
    void setFileOpenCallback(std::function<void(const std::string&)> callback);

    // 设置根目录（用于递归收集文件，默认为当前工作目录）
    void setRootDirectory(const std::string& root);

    // 设置光标颜色获取器（用于输入框光标，跟随编辑器光标配置）
    void setCursorColorGetter(std::function<ftxui::Color()> getter);

    // 设置加载完成回调（在后台线程完成收集后调用，应在主线程中更新 UI）
    // 参数：文件列表、预计算显示路径、规范根路径
    void setOnLoadComplete(
        std::function<void(std::vector<std::string>, std::vector<std::string>, std::string)>
            callback);

    // 主线程调用：接收后台收集到的文件列表（异步加载完成后）
    void receiveFiles(std::vector<std::string> files, std::vector<std::string> display_paths,
                      std::string root_path);

  private:
    Theme& theme_;
    bool is_open_;
    bool is_loading_; // 是否正在异步加载文件列表
    std::string input_;
    size_t cursor_pos_;                          // 输入框光标位置
    std::string root_directory_;                 // 根目录
    std::vector<std::string> all_files_;         // 所有文件路径
    std::vector<std::string> all_display_paths_; // 预计算显示路径，与 all_files_ 一一对应
    std::vector<std::string> filtered_files_;    // 过滤后的文件
    std::vector<std::string>
        filtered_display_paths_; // 预计算的显示路径（与 filtered_files_ 一一对应）
    size_t selected_index_;
    size_t scroll_offset_;
    size_t list_display_count_; // 文件列表显示数量

    utils::FileTypeIconMapper icon_mapper_;
    utils::FileTypeColorMapper color_mapper_;
    std::unique_ptr<features::SyntaxHighlighter> syntax_highlighter_;

    std::function<void(const std::string&)> file_open_callback_;
    std::function<void(std::vector<std::string>, std::vector<std::string>, std::string)>
        on_load_complete_callback_;
    std::string root_path_; // 规范根路径，用于显示相对路径
    std::function<ftxui::Color()> cursor_color_getter_; // 输入框光标颜色

    // 收集根目录下所有文件（递归，排除常见忽略目录）
    void collectAllFiles();

    // 根据输入过滤文件（模糊匹配）
    void filterFiles();

    // 模糊匹配：检查 query 是否为 path 的子序列（不区分大小写）
    static bool fuzzyMatch(const std::string& path, const std::string& query);

    // 读取文件内容用于预览（限制行数）
    std::string readFilePreview(const std::string& filepath, size_t max_lines = 30) const;

    // 获取文件类型用于语法高亮
    std::string getFileTypeForPath(const std::string& filepath) const;

    // 渲染组件
    ftxui::Element renderTitle() const;
    ftxui::Element renderInputBox() const; // 带光标的输入框
    ftxui::Element renderFileList() const; // 带图标的文件列表
    ftxui::Element renderPreview() const;  // 语法高亮预览
    ftxui::Element renderHelpBar() const;

    // 获取文件图标和颜色
    std::string getFileIcon(const std::string& filepath) const;
    ftxui::Color getFileColor(const std::string& filepath) const;
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_FZF_POPUP_H
