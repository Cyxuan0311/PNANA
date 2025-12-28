#ifndef PNANA_UI_FILE_PICKER_H
#define PNANA_UI_FILE_PICKER_H

#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <string>
#include <vector>
#include <filesystem>
#include <functional>

namespace pnana {
namespace ui {

// 文件选择器类型
enum class FilePickerType {
    FILE,        // 只选择文件
    FOLDER,      // 只选择文件夹
    BOTH         // 文件和文件夹都可以
};

// 文件选择器类
class FilePicker {
public:
    explicit FilePicker(Theme& theme);
    
    // 显示文件选择器
    void show(const std::string& start_path = ".", FilePickerType type = FilePickerType::BOTH,
              std::function<void(const std::string&)> on_select = nullptr,
              std::function<void()> on_cancel = nullptr);
    
    // 处理输入
    bool handleInput(ftxui::Event event);
    
    // 渲染选择器
    ftxui::Element render();
    
    // 是否可见
    bool isVisible() const { return visible_; }
    void setVisible(bool visible) { visible_ = visible; }
    
    // 重置
    void reset();

private:
    Theme& theme_;
    bool visible_;
    FilePickerType picker_type_;
    std::string current_path_;
    std::vector<std::string> items_;  // 文件/文件夹列表
    size_t selected_index_;
    std::string filter_input_;  // 过滤输入
    bool show_filter_;  // 是否显示过滤输入框
    std::string path_input_;  // 路径输入
    bool show_path_input_;  // 是否显示路径输入框
    bool type_filter_active_;  // 类型筛选是否激活
    FilePickerType current_type_filter_;  // 当前类型筛选
    
    std::function<void(const std::string&)> on_select_;
    std::function<void()> on_cancel_;
    
    // 辅助方法
    void loadDirectory();
    void navigateUp();
    void navigateDown();
    void selectItem();
    void cancel();
    bool isDirectory(const std::string& path) const;
    std::string getItemName(const std::string& path) const;
    std::vector<std::string> filterItems(const std::vector<std::string>& items, const std::string& filter) const;
    void updatePathFromInput();  // 从路径输入更新当前路径
    std::string resolvePath(const std::string& input_path) const;  // 解析路径（支持相对路径）
};

} // namespace ui
} // namespace pnana

#endif // PNANA_UI_FILE_PICKER_H

