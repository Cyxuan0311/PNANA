#include "features/file_browser.h"
#include "ui/icons.h"
#include <algorithm>
#include <cctype>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

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

bool FileBrowser::hasSelection() const {
    return !flat_items_.empty() && selected_index_ < flat_items_.size();
}

std::string FileBrowser::truncateMiddle(const std::string& str, size_t max_length) const {
    if (str.length() <= max_length) {
        return str;
    }
    
    if (max_length < 5) {
        return str.substr(0, max_length);
    }
    
    // 计算两端保留的长度
    size_t left_len = (max_length - 3) / 2;
    size_t right_len = max_length - 3 - left_len;
    
    return str.substr(0, left_len) + "..." + str.substr(str.length() - right_len);
}

Element FileBrowser::render(int height) {
    auto& colors = theme_.getColors();
    
    Elements content;
    
    // 标题栏（Neovim 风格）
    content.push_back(
        hbox({
            text(" "),
            text(ui::icons::FOLDER_OPEN) | color(colors.function),
            text(" "),
            text(current_directory_) | bold | color(colors.foreground),
            filler(),
            text(" ") | color(colors.comment)
        }) | bgcolor(colors.menubar_bg)
    );
    
    content.push_back(separator());
    
    // 文件列表
    size_t visible_start = 0;
    size_t visible_count = static_cast<size_t>(height - 5);  // 减去标题、分隔符和底部状态栏（2行）
    
    // 调整滚动位置
    if (selected_index_ >= visible_start + visible_count) {
        visible_start = selected_index_ - visible_count + 1;
    }
    if (selected_index_ < visible_start) {
        visible_start = selected_index_;
    }
    
    // 渲染文件列表（Neovim 风格）
    for (size_t i = visible_start; 
         i < flat_items_.size() && i < visible_start + visible_count; 
         ++i) {
        FileItem* item = flat_items_[i];
        
        std::string icon = getFileIcon(*item);
        Color item_color = getFileColor(*item);
        
        // 构建树形结构连接线（Neovim 风格）
        std::string tree_prefix = "";
        for (int d = 0; d < item->depth; ++d) {
            // 检查这个深度层级是否有后续兄弟节点
            bool has_sibling = false;
            for (size_t j = i + 1; j < flat_items_.size(); ++j) {
                if (flat_items_[j]->depth == d) {
                    has_sibling = true;
                    break;
                }
                if (flat_items_[j]->depth < d) {
                    break;
                }
            }
            
            if (has_sibling) {
                tree_prefix += "│ ";  // 有后续兄弟，显示竖线
            } else {
                tree_prefix += "  ";  // 没有后续兄弟，显示空格
            }
        }
        
        // 展开/折叠图标和连接线（Neovim 风格）
        std::string expand_prefix = "";
        std::string expand_icon = "";
        
        // 检查是否有后续兄弟节点（同深度）
        bool has_sibling = false;
        for (size_t j = i + 1; j < flat_items_.size(); ++j) {
            if (flat_items_[j]->depth == item->depth) {
                has_sibling = true;
                break;
            }
            if (flat_items_[j]->depth < item->depth) {
                break;
            }
        }
        
        if (item->is_directory) {
            if (item->expanded) {
                expand_prefix = has_sibling ? "├─" : "└─";  // 展开状态
                expand_icon = "▼";
            } else {
                expand_prefix = has_sibling ? "├─" : "└─";  // 折叠状态
                expand_icon = "▶";
            }
        } else {
            // 文件使用相同的连接线
            expand_prefix = has_sibling ? "├─" : "└─";
            expand_icon = " ";
        }
        
        std::string display_name = item->name;
        
        // 构建行元素（Neovim 风格）
        Elements row_elements = {
            text(" "),
            text(tree_prefix) | color(colors.comment),
            text(expand_prefix) | color(colors.comment),
            text(expand_icon) | color(item_color),
            text(" "),
            text(icon) | color(item_color),
            text(" "),
            text(display_name) | color(item_color)
        };
        
        auto item_text = hbox(row_elements);
        
        // 选中项高亮（Neovim 风格）
        if (i == selected_index_) {
            item_text = item_text | bgcolor(colors.selection) | bold;
        } else {
            item_text = item_text | bgcolor(colors.background);
        }
        
        content.push_back(item_text);
    }
    
    // 填充空行（留出底部状态栏空间）
    while (content.size() < static_cast<size_t>(height - 2)) {
        content.push_back(text(""));
    }
    
    // Bottom status bar: show selected file's full path
    content.push_back(separator());
    
    std::string selected_path_display = "";
    if (hasSelection()) {
        std::string full_path = getSelectedPath();
        // Limit path length to 40 characters, truncate middle if exceeded
        selected_path_display = truncateMiddle(full_path, 40);
    } else {
        selected_path_display = "No selection";
    }
    
    content.push_back(
        hbox({
            text(" "),
            text(ui::icons::LOCATION) | color(colors.keyword),
            text(" "),
            text(selected_path_display) | color(colors.comment)
        }) | bgcolor(colors.menubar_bg)
    );
    
    return vbox(content) | bgcolor(colors.background);
}

std::string FileBrowser::getFileIcon(const FileItem& item) const {
    if (item.is_directory) {
        if (item.name == "..") {
            return ui::icons::FOLDER_UP; // 上级目录
        }
        return ui::icons::FOLDER; // 文件夹图标
    }
    
    std::string ext = getFileExtension(item.name);
    std::string name_lower = item.name;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);
    
    // 根据扩展名返回图标（使用 JetBrains Nerd Font 图标）
    
    // C/C++ 文件
    if (ext == "cpp" || ext == "cc" || ext == "cxx" || ext == "c++") return ui::icons::CPP;
    if (ext == "h" || ext == "hpp" || ext == "hxx" || ext == "hh") return ui::icons::CPP;
    if (ext == "c") return ui::icons::C;
    
    // 脚本语言
    if (ext == "py" || ext == "pyw" || ext == "pyi") return ui::icons::PYTHON;
    if (ext == "js" || ext == "jsx" || ext == "mjs") return ui::icons::JAVASCRIPT;
    if (ext == "ts" || ext == "tsx") return ui::icons::TYPESCRIPT;
    if (ext == "rb" || ext == "rbw") return ui::icons::RUBY;
    if (ext == "php" || ext == "php3" || ext == "php4" || ext == "php5" || ext == "phtml") return ui::icons::PHP;
    if (ext == "sh" || ext == "bash" || ext == "zsh" || ext == "fish") return ui::icons::SHELL;
    if (name_lower == "makefile" || name_lower == "makefile.am" || name_lower == "makefile.in") return ui::icons::MAKEFILE;
    
    // 编译语言
    if (ext == "java" || ext == "class" || ext == "jar") return ui::icons::JAVA;
    if (ext == "go") return ui::icons::GO;
    if (ext == "rs") return ui::icons::RUST;
    
    // Web 技术
    if (ext == "html" || ext == "htm" || ext == "xhtml") return ui::icons::HTML;
    if (ext == "css" || ext == "scss" || ext == "sass" || ext == "less") return ui::icons::CSS;
    
    // 数据格式
    if (ext == "json" || ext == "jsonc") return ui::icons::JSON;
    if (ext == "xml" || ext == "xsd" || ext == "xsl") return ui::icons::XML;
    if (ext == "yml" || ext == "yaml") return ui::icons::YAML;
    if (ext == "toml") return ui::icons::CONFIG;
    
    // 文档
    if (ext == "md" || ext == "markdown") return ui::icons::MARKDOWN;
    if (ext == "txt" || ext == "log") return ui::icons::FILE_TEXT;
    if (ext == "pdf") return ui::icons::PDF;
    
    // 数据库
    if (ext == "sql") return ui::icons::SQL;
    if (ext == "db" || ext == "sqlite" || ext == "sqlite3" || ext == "db3") return ui::icons::DATABASE;
    
    // 图片
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || 
        ext == "bmp" || ext == "svg" || ext == "ico" || ext == "webp") {
        return ui::icons::IMAGE;
    }
    
    // 视频
    if (ext == "mp4" || ext == "avi" || ext == "mov" || ext == "wmv" || 
        ext == "flv" || ext == "mkv" || ext == "webm") {
        return ui::icons::VIDEO;
    }
    
    // 音频
    if (ext == "mp3" || ext == "wav" || ext == "flac" || ext == "aac" || 
        ext == "ogg" || ext == "m4a") {
        return ui::icons::AUDIO;
    }
    
    // 压缩包
    if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "bz2" || 
        ext == "xz" || ext == "7z" || ext == "rar" || ext == "z") {
        return ui::icons::ARCHIVE;
    }
    
    // 配置文件
    if (ext == "conf" || ext == "config" || ext == "ini" || ext == "cfg" || 
        ext == "properties" || name_lower == ".gitignore" || name_lower == ".gitconfig" ||
        name_lower == ".gitattributes") {
        return ui::icons::CONFIG;
    }
    
    // Git 相关
    if (name_lower == ".gitignore" || name_lower == ".gitattributes" || 
        name_lower == ".gitmodules" || name_lower == ".gitconfig") {
        return ui::icons::GITIGNORE;
    }
    
    // CMake
    if (ext == "cmake" || name_lower == "cmakelists.txt") return ui::icons::CMAKE;
    
    // Docker
    if (name_lower == "dockerfile" || ext == "dockerignore") return ui::icons::DOCKER;
    
    // 可执行文件（Unix）
    if (ext == "exe" || ext == "bin" || ext == "out" || ext == "app") {
        return ui::icons::EXECUTABLE;
    }
    
    return ui::icons::FILE; // 默认文件图标
}

std::string FileBrowser::getFileExtension(const std::string& filename) const {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

Color FileBrowser::getFileColor(const FileItem& item) const {
    auto& colors = theme_.getColors();
    
    if (item.is_directory) {
        return colors.function;  // 蓝色
    }
    
    std::string ext = getFileExtension(item.name);
    
    // 根据文件类型返回颜色
    if (ext == "cpp" || ext == "c" || ext == "h" || ext == "hpp") {
        return colors.keyword;  // 紫色
    }
    if (ext == "py" || ext == "js" || ext == "ts" || ext == "java") {
        return colors.string;  // 绿色
    }
    if (ext == "md" || ext == "txt") {
        return colors.foreground;  // 白色
    }
    if (ext == "json" || ext == "xml" || ext == "yml") {
        return colors.number;  // 橙色
    }
    
    return colors.comment;  // 灰色
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

