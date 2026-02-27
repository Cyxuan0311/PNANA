#ifndef PNANA_FEATURES_FILE_BROWSER_H
#define PNANA_FEATURES_FILE_BROWSER_H

#include "ui/theme.h"
#include <deque>
#include <filesystem>
#include <ftxui/dom/elements.hpp>
#include <set>
#include <string>
#include <vector>

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
    bool expanded;                  // 是否展开
    bool loaded;                    // 是否已加载子项
    int depth;                      // 深度（用于缩进）
    std::vector<FileItem> children; // 子项

    FileItem(const std::string& n, const std::string& p, bool is_dir, int d = 0)
        : name(n), path(p), is_directory(is_dir), is_hidden(false), size(0), expanded(false),
          loaded(false), depth(d) {
        is_hidden = (!name.empty() && name[0] == '.');
    }
};

// 删除操作记录（用于撤销删除）
struct DeleteOperation {
    std::string name;
    std::string path;
    std::string parent_path;
    bool is_directory{false};
    size_t size{0};
    std::vector<FileItem> original_children; // 目录删除时保存的子项结构（用于恢复）
};

// 文件浏览器
class FileBrowser {
  public:
    explicit FileBrowser(ui::Theme& theme);

    // 目录操作
    bool openDirectory(const std::string& path);
    void refresh();
    std::string getCurrentDirectory() const {
        return current_directory_;
    }

    // 导航
    void selectNext();
    void selectPrevious();
    void selectFirst();
    void selectLast();
    void selectPageUp();   // 向上翻页
    void selectPageDown(); // 向下翻页
    bool toggleSelected(); // 切换展开/折叠或打开文件
    bool goUp();           // 返回上级目录

    // 获取选中的文件
    std::string getSelectedFile() const;
    std::string getSelectedPath() const; // 获取选中项的完整路径
    bool hasSelection() const;
    size_t getSelectedIndex() const {
        return selected_index_;
    }                            // 获取当前选中索引
    size_t getItemCount() const; // 获取项目总数

    // 多选支持
    void toggleSelection(size_t index);                // 切换指定索引的选中状态
    void clearSelection();                             // 清除所有选中
    bool isSelected(size_t index) const;               // 检查指定索引是否被选中
    size_t getSelectedCount() const;                   // 获取选中项数量
    std::vector<std::string> getSelectedPaths() const; // 获取所有选中项的路径
    void selectRange(size_t start, size_t end);        // 选中范围（用于 Ctrl+上下键）

    // 获取展平的项目列表（用于 UI 渲染）
    const std::vector<FileItem*>& getFlatItems() const {
        return flat_items_;
    }

    // 渲染（使用 FileBrowserView）
    ftxui::Element render(int height);

    // 显示/隐藏
    void setVisible(bool visible);
    bool isVisible() const {
        return visible_;
    }
    void toggle() {
        setVisible(!visible_);
    }

    // 设置
    void setShowHidden(bool show) {
        show_hidden_ = show;
        refresh();
    }
    bool getShowHidden() const {
        return show_hidden_;
    }

    // 文件操作
    bool renameSelected(const std::string& new_name);
    bool deleteSelected();
    bool moveSelected(const std::string& target_path); // 移动文件/文件夹到目标路径
    bool undoDelete();
    bool canUndoDelete() const;
    void clearUndoStack();
    std::string getSelectedName() const;            // 获取选中项的名称（不含路径）
    bool selectItemByName(const std::string& name); // 根据名称选中项目

    // 批量文件操作（复制/剪切/粘贴）
    bool copySelected();                            // 复制选中的文件/文件夹
    bool cutSelected();                             // 剪切选中的文件/文件夹
    bool pasteFiles(const std::string& target_dir); // 粘贴文件到目标目录
    bool hasClipboardFiles() const;                 // 检查剪贴板中是否有文件
    bool isCutOperation() const;                    // 检查当前操作是剪切还是复制
    void clearClipboard(); // 清除剪贴板数据（复制/剪切状态）

  private:
    ui::Theme& theme_;
    std::string current_directory_;
    std::vector<FileItem> items_;
    size_t selected_index_;
    bool visible_;
    bool show_hidden_;
    bool directory_loaded_; // 标记目录是否已加载（用于延迟加载）

    // 多选支持
    std::set<size_t> selected_indices_; // 选中的索引集合

    // 剪贴板支持（文件操作）
    struct ClipboardData {
        std::vector<std::string> paths; // 文件路径列表
        bool is_cut;                    // true=剪切, false=复制
    };
    ClipboardData clipboard_data_; // 剪贴板数据

    // 辅助方法
    void loadDirectory();
    void loadDirectoryRecursive(FileItem& item); // 递归加载目录
    void flattenTree(const std::vector<FileItem>& tree, std::vector<FileItem*>& flat,
                     int depth = 0); // 展平树形结构用于显示
    bool copyFileOrDirectory(const std::string& source, const std::string& target); // 复制文件/目录

    // 树形结构相关
    std::vector<FileItem> tree_items_;  // 树形结构
    std::vector<FileItem*> flat_items_; // 展平后的项目（用于导航）

    // 删除撤销栈
    std::deque<DeleteOperation> delete_undo_stack_;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_FILE_BROWSER_H
