#include "features/file_browser.h"
#include "ui/file_browser_view.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace features {

FileBrowser::FileBrowser(ui::Theme& theme)
    : theme_(theme),
      current_directory_("."),
      selected_index_(0),
      visible_(false),
      show_hidden_(false) {
    openDirectory(".");
}

bool FileBrowser::openDirectory(const std::string& path) {
    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            current_directory_ = fs::canonical(path).string();
            loadDirectory();
            selected_index_ = 0;
            return true;
        }
    } catch (const std::exception&) {
        return false;
    }
    return false;
}

void FileBrowser::refresh() {
    loadDirectory();
}

void FileBrowser::loadDirectory() {
    tree_items_.clear();
    flat_items_.clear();
    items_.clear();
    
    try {
        // 读取目录内容
        std::vector<FileItem> dirs;
        std::vector<FileItem> files;
        
        for (const auto& entry : fs::directory_iterator(current_directory_)) {
            std::string name = entry.path().filename().string();
            
            // 跳过隐藏文件（如果设置不显示）
            if (!show_hidden_ && !name.empty() && name[0] == '.') {
                continue;
            }
            
            FileItem item(name, entry.path().string(), entry.is_directory(), 0);
            
            if (!entry.is_directory() && fs::is_regular_file(entry)) {
                try {
                    item.size = fs::file_size(entry);
                } catch (...) {
                    item.size = 0;
                }
            }
            
            if (entry.is_directory()) {
                dirs.push_back(item);
            } else {
                files.push_back(item);
            }
        }
        
        // 排序：目录在前，文件在后
        std::sort(dirs.begin(), dirs.end(), 
                 [](const FileItem& a, const FileItem& b) {
                     return a.name < b.name;
                 });
        std::sort(files.begin(), files.end(),
                 [](const FileItem& a, const FileItem& b) {
                     return a.name < b.name;
                 });
        
        // 构建树形结构
        tree_items_.insert(tree_items_.end(), dirs.begin(), dirs.end());
        tree_items_.insert(tree_items_.end(), files.begin(), files.end());
        
        // 展平树形结构用于显示和导航
        flattenTree(tree_items_, flat_items_);
        
        // 为了兼容性，也填充 items_
        for (auto* item : flat_items_) {
            items_.push_back(*item);
        }
        
    } catch (const std::exception&) {
        // 读取失败，保持空列表
    }
    
    // 确保选中索引有效
    if (selected_index_ >= flat_items_.size() && !flat_items_.empty()) {
        selected_index_ = flat_items_.size() - 1;
    }
}

size_t FileBrowser::getItemCount() const {
    return flat_items_.size();
}

void FileBrowser::loadDirectoryRecursive(FileItem& item) {
    if (!item.is_directory || item.loaded) {
        return;
    }
    
    try {
        item.children.clear();
        std::vector<FileItem> dirs;
        std::vector<FileItem> files;
        
        for (const auto& entry : fs::directory_iterator(item.path)) {
            std::string name = entry.path().filename().string();
            
            // 跳过隐藏文件（如果设置不显示）
            if (!show_hidden_ && !name.empty() && name[0] == '.') {
                continue;
            }
            
            FileItem child(name, entry.path().string(), entry.is_directory(), item.depth + 1);
            
            if (!entry.is_directory() && fs::is_regular_file(entry)) {
                try {
                    child.size = fs::file_size(entry);
                } catch (...) {
                    child.size = 0;
                }
            }
            
            if (entry.is_directory()) {
                dirs.push_back(child);
            } else {
                files.push_back(child);
            }
        }
        
        // 排序
        std::sort(dirs.begin(), dirs.end(), 
                 [](const FileItem& a, const FileItem& b) {
                     return a.name < b.name;
                 });
        std::sort(files.begin(), files.end(),
                 [](const FileItem& a, const FileItem& b) {
                     return a.name < b.name;
                 });
        
        item.children.insert(item.children.end(), dirs.begin(), dirs.end());
        item.children.insert(item.children.end(), files.begin(), files.end());
        item.loaded = true;
        
    } catch (const std::exception&) {
        // 读取失败
    }
}

void FileBrowser::flattenTree(const std::vector<FileItem>& tree, std::vector<FileItem*>& flat, int depth) {
    for (auto& item : const_cast<std::vector<FileItem>&>(tree)) {
        item.depth = depth;
        flat.push_back(&item);
        
        // 如果展开且是目录，递归添加子项
        if (item.is_directory && item.expanded) {
            if (!item.loaded) {
                loadDirectoryRecursive(item);
            }
            flattenTree(item.children, flat, depth + 1);
        }
    }
}

void FileBrowser::selectNext() {
    if (!flat_items_.empty() && selected_index_ < flat_items_.size() - 1) {
        selected_index_++;
    }
}

void FileBrowser::selectPrevious() {
    if (selected_index_ > 0) {
        selected_index_--;
    }
}

void FileBrowser::selectFirst() {
    selected_index_ = 0;
}

void FileBrowser::selectLast() {
    if (!flat_items_.empty()) {
        selected_index_ = flat_items_.size() - 1;
    }
}

void FileBrowser::selectPageUp() {
    if (flat_items_.empty()) {
        return;
    }

    // 假设页面大小为可见区域的高度，这里简化为固定值20
    // 实际应该根据UI的高度动态计算
    const size_t page_size = 20;

    if (selected_index_ < page_size) {
        selected_index_ = 0;
    } else {
        selected_index_ -= page_size;
    }
}

void FileBrowser::selectPageDown() {
    if (flat_items_.empty()) {
        return;
    }

    // 假设页面大小为可见区域的高度，这里简化为固定值20
    // 实际应该根据UI的高度动态计算
    const size_t page_size = 20;

    size_t max_index = flat_items_.size() - 1;
    if (selected_index_ + page_size > max_index) {
        selected_index_ = max_index;
    } else {
        selected_index_ += page_size;
    }
}

bool FileBrowser::toggleSelected() {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return false;
    }
    
    FileItem* item = flat_items_[selected_index_];
    
    if (item->is_directory) {
        // 切换展开/折叠状态
        item->expanded = !item->expanded;
        
        // 如果展开，加载子目录
        if (item->expanded && !item->loaded) {
            loadDirectoryRecursive(*item);
        }
        
        // 重新展平树形结构
        flat_items_.clear();
        items_.clear();
        flattenTree(tree_items_, flat_items_);
        
        // 为了兼容性，也填充 items_
        for (auto* it : flat_items_) {
            items_.push_back(*it);
        }
        
        // 保持选中当前项
        for (size_t i = 0; i < flat_items_.size(); ++i) {
            if (flat_items_[i] == item) {
                selected_index_ = i;
                break;
            }
        }
        
        return false;  // 不是文件，不打开
    }
    
    return true;  // 是文件，返回true表示可以打开
}

bool FileBrowser::goUp() {
    if (current_directory_ != "/") {
        fs::path parent = fs::path(current_directory_).parent_path();
        return openDirectory(parent.string());
    }
    return false;
}

std::string FileBrowser::getSelectedFile() const {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return "";
    }
    
    const FileItem* item = flat_items_[selected_index_];
    return item->is_directory ? "" : item->path;
}

std::string FileBrowser::getSelectedPath() const {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return "";
    }
    
    return flat_items_[selected_index_]->path;
}

std::string FileBrowser::getSelectedName() const {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return "";
    }
    
    return flat_items_[selected_index_]->name;
}

bool FileBrowser::selectItemByName(const std::string& name) {
    // 在展平的项目列表中查找匹配名称的项目
    for (size_t i = 0; i < flat_items_.size(); ++i) {
        if (flat_items_[i]->name == name) {
            selected_index_ = i;
            return true;
        }
    }
    return false;
}

bool FileBrowser::hasSelection() const {
    return !flat_items_.empty() && selected_index_ < flat_items_.size();
}

// 为了保持向后兼容，保留 render 方法，但使用 FileBrowserView
ftxui::Element FileBrowser::render(int height) {
    static ui::FileBrowserView view(theme_);
    return view.render(*this, height);
}

bool FileBrowser::renameSelected(const std::string& new_name) {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return false;
    }
    
    FileItem* item = flat_items_[selected_index_];
    
    // 不能重命名上级目录
    if (item->name == "..") {
        return false;
    }
    
    try {
        fs::path old_path = item->path;
        fs::path parent_path = fs::path(old_path).parent_path();
        fs::path new_path = parent_path / new_name;
        
        // 检查新名称是否已存在
        if (fs::exists(new_path)) {
            return false;
        }
        
        // 重命名
        fs::rename(old_path, new_path);
        
        // 更新路径和名称
        item->name = new_name;
        item->path = new_path.string();
        
        // 刷新目录列表
        loadDirectory();
        
        // 尝试选中新名称的项
        for (size_t i = 0; i < flat_items_.size(); ++i) {
            if (flat_items_[i]->name == new_name && flat_items_[i]->path == new_path.string()) {
                selected_index_ = i;
                break;
            }
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileBrowser::deleteSelected() {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size()) {
        return false;
    }
    
    FileItem* item = flat_items_[selected_index_];
    
    // 不能删除上级目录
    if (item->name == "..") {
        return false;
    }
    
    try {
        fs::path path = item->path;
        
        if (item->is_directory) {
            // 删除目录（递归）
            fs::remove_all(path);
        } else {
            // 删除文件
            fs::remove(path);
        }
        
        // 刷新目录列表
        loadDirectory();
        
        // 调整选中索引
        if (selected_index_ >= flat_items_.size() && !flat_items_.empty()) {
            selected_index_ = flat_items_.size() - 1;
        }
        
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace features
} // namespace pnana

