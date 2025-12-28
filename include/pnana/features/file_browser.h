#ifndef PNANA_FEATURES_FILE_BROWSER_H
#define PNANA_FEATURES_FILE_BROWSER_H

#include <string>
#include <vector>
#include <filesystem>
#include "ui/theme.h"
#include <ftxui/dom/elements.hpp>

namespace pnana {
namespace features {

namespace fs = std::filesystem;

// 文件/文件夹项（树形结构）
struct FileItem {
    std::string name;
    std::string path;
    bool is_directory;
    bool is_hidden;
    size_t size;
    bool expanded;          // 是否展开
    bool loaded;            // 是否已加载子项
    int depth;              // 深度（用于缩进）
    std::vector<FileItem> children;  // 子项
    
    FileItem(const std::string& n, const std::string& p, bool is_dir, int d = 0)
        : name(n), path(p), is_directory(is_dir), is_hidden(false), 
          size(0), expanded(false), loaded(false), depth(d) {
        is_hidden = (!name.empty() && name[0] == '.');
    }
};

// 文件浏览器
class FileBrowser {
public:
    explicit FileBrowser(ui::Theme& theme);
    
    // 目录操作
    bool openDirectory(const std::string& path);
    void refresh();
    std::string getCurrentDirectory() const { return current_directory_; }
    
    // 导航
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();
    bool toggleSelected();  // 切换展开/折叠或打开文件
    bool goUp();  // 返回上级目录
    
    // 获取选中的文件
    std::string getSelectedFile() const;
    std::string getSelectedPath() const;  // 获取选中项的完整路径
    bool hasSelection() const;
    size_t getSelectedIndex() const { return selected_index_; }  // 获取当前选中索引
    size_t getItemCount() const;  // 获取项目总数
    
    // 渲染
    ftxui::Element render(int height);
    
    // 显示/隐藏
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void toggle() { visible_ = !visible_; }
    
    // 设置
    void setShowHidden(bool show) { show_hidden_ = show; refresh(); }
    bool getShowHidden() const { return show_hidden_; }
    
    // 文件操作
    bool renameSelected(const std::string& new_name);
    bool deleteSelected();
    std::string getSelectedName() const;  // 获取选中项的名称（不含路径）
    
private:
    ui::Theme& theme_;
    std::string current_directory_;
    std::vector<FileItem> items_;
    size_t selected_index_;
    bool visible_;
    bool show_hidden_;
    
    // 辅助方法
    void loadDirectory();
    void loadDirectoryRecursive(FileItem& item);  // 递归加载目录
    void flattenTree(const std::vector<FileItem>& tree, std::vector<FileItem*>& flat, int depth = 0);  // 展平树形结构用于显示
    std::string getFileIcon(const FileItem& item) const;
    std::string getFileExtension(const std::string& filename) const;
    ftxui::Color getFileColor(const FileItem& item) const;
    std::string truncateMiddle(const std::string& str, size_t max_length) const;
    
    // 树形结构相关
    std::vector<FileItem> tree_items_;  // 树形结构
    std::vector<FileItem*> flat_items_;  // 展平后的项目（用于导航）
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_FILE_BROWSER_H

