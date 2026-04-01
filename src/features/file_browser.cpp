#include "features/file_browser.h"
#include "ui/file_browser_view.h"
#include <algorithm>
#include <cctype>
#include <deque>
#include <fstream>
#include <functional>

namespace pnana {
namespace features {

FileBrowser::FileBrowser(ui::Theme& theme)
    : theme_(theme), selected_index_(0), visible_(false), show_hidden_(false),
      directory_loaded_(false), clipboard_data_{} {
    // 初始化当前目录为绝对路径
    try {
        current_directory_ = fs::current_path().string();
    } catch (const std::exception&) {
        current_directory_ = ".";
    }
    // 延迟加载：不在构造函数中加载目录，只在首次显示时才加载
}

bool FileBrowser::openDirectory(const std::string& path) {
    if (path.size() >= 6 && path.compare(0, 6, "ssh://") == 0) {
        current_directory_ = path;
        loadDirectory();
        selected_index_ = 0;
        directory_loaded_ = true;
        return true;
    }
    try {
        if (fs::exists(path) && fs::is_directory(path)) {
            current_directory_ = fs::canonical(path).string();
            loadDirectory();
            selected_index_ = 0;
            directory_loaded_ = true;
            return true;
        } else {
            // 路径不存在或不是目录，尝试转换为绝对路径
            fs::path abs_path = fs::absolute(path);
            current_directory_ = abs_path.string();
            // 不加载目录内容，因为路径无效
            return false;
        }
    } catch (const std::exception&) {
        return false;
    }
}

void FileBrowser::setRemoteLoader(RemoteLoader fn) {
    remote_loader_ = std::move(fn);
}

void FileBrowser::clearRemoteLoader() {
    remote_loader_ = nullptr;
}

void FileBrowser::setRemoteFileOpExecutor(RemoteFileOpExecutor fn) {
    remote_file_op_exec_ = std::move(fn);
}

void FileBrowser::clearRemoteFileOpExecutor() {
    remote_file_op_exec_ = nullptr;
}

void FileBrowser::setRemoteRecursiveLoader(RemoteRecursiveLoader fn) {
    remote_recursive_loader_ = std::move(fn);
}

void FileBrowser::clearRemoteRecursiveLoader() {
    remote_recursive_loader_ = nullptr;
}

// ── 静态辅助 ────────────────────────────────────────────────────────────────

std::string FileBrowser::extractRemotePath(const std::string& ssh_uri) {
    if (ssh_uri.find("ssh://") != 0)
        return ssh_uri;
    // ssh://user@host[:port]/path  →  /path
    size_t slash = ssh_uri.find('/', 6); // after "ssh://"
    return (slash == std::string::npos) ? "/" : ssh_uri.substr(slash);
}

std::string FileBrowser::shellQuote(const std::string& s) {
    std::string q = "'";
    for (char c : s) {
        if (c == '\'')
            q += "'\\''";
        else
            q += c;
    }
    return q + "'";
}

void FileBrowser::refresh() {
    loadDirectory();
}

void FileBrowser::setVisible(bool visible) {
    bool was_visible = visible_;
    visible_ = visible;

    // 延迟加载：首次显示时加载目录
    if (visible && !was_visible && !directory_loaded_) {
        openDirectory(current_directory_.empty() ? "." : current_directory_);
    }
}

void FileBrowser::loadDirectory() {
    tree_items_.clear();
    flat_items_.clear();
    items_.clear();

    if (remote_loader_) {
        std::vector<FileItem> loaded = remote_loader_(current_directory_);

        // 将远程返回的内容按目录优先、大小写不敏感排序
        std::vector<FileItem> rdirs, rfiles;
        for (FileItem& item : loaded) {
            if (item.name == "..") {
                tree_items_.insert(tree_items_.begin(), std::move(item));
                continue;
            }
            if (item.is_directory)
                rdirs.push_back(std::move(item));
            else
                rfiles.push_back(std::move(item));
        }
        auto ciLessRemote = [](const FileItem& a, const FileItem& b) {
            std::string al = a.name, bl = b.name;
            std::transform(al.begin(), al.end(), al.begin(), ::tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            if (al != bl)
                return al < bl;
            return a.name < b.name;
        };
        std::sort(rdirs.begin(), rdirs.end(), ciLessRemote);
        std::sort(rfiles.begin(), rfiles.end(), ciLessRemote);
        tree_items_.insert(tree_items_.end(), rdirs.begin(), rdirs.end());
        tree_items_.insert(tree_items_.end(), rfiles.begin(), rfiles.end());

        flattenTree(tree_items_, flat_items_);
        for (auto* item : flat_items_) {
            items_.push_back(*item);
        }
        if (selected_index_ >= flat_items_.size() && !flat_items_.empty()) {
            selected_index_ = flat_items_.size() - 1;
        }
        std::set<size_t> valid_selections;
        for (size_t idx : selected_indices_) {
            if (idx < flat_items_.size())
                valid_selections.insert(idx);
        }
        selected_indices_ = valid_selections;
        return;
    }

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

            // 优化：延迟文件大小获取，只在需要显示时才获取
            // 文件大小在渲染时按需获取，避免启动时的大量系统调用
            // 这里只设置基本属性，size 保持为 0
            // 如果需要显示文件大小，可以在 render() 或 getItemSize() 中按需获取

            if (entry.is_directory()) {
                dirs.push_back(item);
            } else {
                files.push_back(item);
            }
        }

        // 排序：目录在前，文件在后，大小写不敏感
        auto ciLess = [](const FileItem& a, const FileItem& b) {
            std::string al = a.name, bl = b.name;
            std::transform(al.begin(), al.end(), al.begin(), ::tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            if (al != bl)
                return al < bl;
            return a.name < b.name;
        };
        std::sort(dirs.begin(), dirs.end(), ciLess);
        std::sort(files.begin(), files.end(), ciLess);

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

    // 清除无效的选中项
    std::set<size_t> valid_selections;
    for (size_t index : selected_indices_) {
        if (index < flat_items_.size()) {
            valid_selections.insert(index);
        }
    }
    selected_indices_ = valid_selections;
}

size_t FileBrowser::getItemCount() const {
    return flat_items_.size();
}

void FileBrowser::loadDirectoryRecursive(FileItem& item) {
    if (!item.is_directory || item.loaded) {
        return;
    }

    // SSH 远程模式：使用远程加载器
    if (remote_recursive_loader_) {
        std::vector<FileItem> loaded = remote_recursive_loader_(item.path);

        // 按目录优先、大小写不敏感排序
        std::vector<FileItem> rdirs, rfiles;
        for (FileItem& child : loaded) {
            if (child.is_directory)
                rdirs.push_back(std::move(child));
            else
                rfiles.push_back(std::move(child));
        }
        auto ciLessRemote = [](const FileItem& a, const FileItem& b) {
            std::string al = a.name, bl = b.name;
            std::transform(al.begin(), al.end(), al.begin(), ::tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            if (al != bl)
                return al < bl;
            return a.name < b.name;
        };
        std::sort(rdirs.begin(), rdirs.end(), ciLessRemote);
        std::sort(rfiles.begin(), rfiles.end(), ciLessRemote);

        item.children.insert(item.children.end(), rdirs.begin(), rdirs.end());
        item.children.insert(item.children.end(), rfiles.begin(), rfiles.end());
        item.loaded = true;
        return;
    }

    // 本地模式：使用本地文件系统
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

        // 排序（大小写不敏感）
        auto ciLessItem = [](const FileItem& a, const FileItem& b) {
            std::string al = a.name, bl = b.name;
            std::transform(al.begin(), al.end(), al.begin(), ::tolower);
            std::transform(bl.begin(), bl.end(), bl.begin(), ::tolower);
            if (al != bl)
                return al < bl;
            return a.name < b.name;
        };
        std::sort(dirs.begin(), dirs.end(), ciLessItem);
        std::sort(files.begin(), files.end(), ciLessItem);

        item.children.insert(item.children.end(), dirs.begin(), dirs.end());
        item.children.insert(item.children.end(), files.begin(), files.end());
        item.loaded = true;

    } catch (const std::exception&) {
        // 读取失败
    }
}

void FileBrowser::flattenTree(const std::vector<FileItem>& tree, std::vector<FileItem*>& flat,
                              int depth) {
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
        if (item->name == "..") {
            goUp();
            return false;
        }
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
                // 更新多选索引
                std::set<size_t> updated_selections;
                for (size_t old_idx : selected_indices_) {
                    // 找到对应的新索引（如果有）
                    for (size_t j = 0; j < flat_items_.size(); ++j) {
                        if (flat_items_[j]->path == flat_items_[old_idx]->path) {
                            updated_selections.insert(j);
                            break;
                        }
                    }
                }
                selected_indices_ = updated_selections;
                break;
            }
        }

        return false; // 不是文件，不打开
    }

    return true; // 是文件，返回true表示可以打开
}

bool FileBrowser::goUp() {
    if (remote_loader_) {
        if (current_directory_.size() <= 6)
            return false; // ssh:// only
        size_t scheme_end = current_directory_.find("://");
        if (scheme_end == std::string::npos)
            return false;
        size_t path_start = current_directory_.find('/', scheme_end + 3);
        if (path_start == std::string::npos)
            return false;
        std::string path_part = current_directory_.substr(path_start);
        if (path_part == "/" || path_part.empty())
            return false;
        while (!path_part.empty() && path_part.back() == '/')
            path_part.pop_back();
        size_t last_slash = path_part.rfind('/');
        std::string parent_path = (last_slash == std::string::npos || last_slash == 0)
                                      ? current_directory_.substr(0, path_start) + "/"
                                      : current_directory_.substr(0, path_start + last_slash);
        return openDirectory(parent_path);
    }
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

// 多选支持
void FileBrowser::toggleSelection(size_t index) {
    if (index >= flat_items_.size()) {
        return;
    }
    if (selected_indices_.find(index) != selected_indices_.end()) {
        selected_indices_.erase(index);
    } else {
        selected_indices_.insert(index);
    }
}

void FileBrowser::clearSelection() {
    selected_indices_.clear();
}

bool FileBrowser::isSelected(size_t index) const {
    return selected_indices_.find(index) != selected_indices_.end();
}

size_t FileBrowser::getSelectedCount() const {
    return selected_indices_.size();
}

std::vector<std::string> FileBrowser::getSelectedPaths() const {
    std::vector<std::string> paths;
    for (size_t index : selected_indices_) {
        if (index < flat_items_.size()) {
            paths.push_back(flat_items_[index]->path);
        }
    }
    return paths;
}

void FileBrowser::selectRange(size_t start, size_t end) {
    if (start > end) {
        std::swap(start, end);
    }
    for (size_t i = start; i <= end && i < flat_items_.size(); ++i) {
        selected_indices_.insert(i);
    }
}

// 批量文件操作
bool FileBrowser::copySelected() {
    clipboard_data_.paths = getSelectedPaths();
    clipboard_data_.is_cut = false;
    return !clipboard_data_.paths.empty();
}

bool FileBrowser::cutSelected() {
    clipboard_data_.paths = getSelectedPaths();
    clipboard_data_.is_cut = true;
    return !clipboard_data_.paths.empty();
}

bool FileBrowser::pasteFiles(const std::string& target_dir) {
    if (clipboard_data_.paths.empty())
        return false;

    // ── 远程模式 ──────────────────────────────────────────────────────────────
    if (remote_file_op_exec_) {
        std::string dst_remote =
            (target_dir.find("ssh://") == 0) ? extractRemotePath(target_dir) : target_dir;
        if (!dst_remote.empty() && dst_remote.back() != '/')
            dst_remote += '/';

        bool all_ok = true;
        for (const std::string& src_uri : clipboard_data_.paths) {
            std::string src_remote =
                (src_uri.find("ssh://") == 0) ? extractRemotePath(src_uri) : src_uri;
            // 提取文件名
            std::string fname = src_remote;
            auto slash = fname.rfind('/');
            if (slash != std::string::npos)
                fname = fname.substr(slash + 1);

            std::string dst_path = dst_remote + fname;
            std::string cmd = clipboard_data_.is_cut
                                  ? "mv " + shellQuote(src_remote) + " " + shellQuote(dst_path)
                                  : "cp -r " + shellQuote(src_remote) + " " + shellQuote(dst_path);
            auto [ok, out] = remote_file_op_exec_(cmd);
            if (!ok)
                all_ok = false;
        }

        if (clipboard_data_.is_cut)
            clipboard_data_.paths.clear();
        refresh();
        return all_ok;
    }

    // ── 本地模式 ──────────────────────────────────────────────────────────────
    try {
        fs::path target = fs::path(target_dir);
        if (!fs::exists(target) || !fs::is_directory(target))
            return false;

        for (const std::string& source_path : clipboard_data_.paths) {
            fs::path source = source_path;
            if (!fs::exists(source))
                continue;

            fs::path target_path = target / source.filename();

            int counter = 1;
            std::string base_name = target_path.filename().string();
            std::string base_ext;
            size_t dot_pos = base_name.find_last_of('.');
            if (dot_pos != std::string::npos) {
                base_ext = base_name.substr(dot_pos);
                base_name = base_name.substr(0, dot_pos);
            }

            while (fs::exists(target_path)) {
                std::string new_name = base_name + "_" + std::to_string(counter) + base_ext;
                target_path = target / new_name;
                counter++;
            }

            if (clipboard_data_.is_cut) {
                fs::rename(source, target_path);
            } else {
                if (!copyFileOrDirectory(source.string(), target_path.string()))
                    return false;
            }
        }

        if (clipboard_data_.is_cut)
            clipboard_data_.paths.clear();
        refresh();
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileBrowser::hasClipboardFiles() const {
    return !clipboard_data_.paths.empty();
}

bool FileBrowser::isCutOperation() const {
    return clipboard_data_.is_cut;
}

void FileBrowser::clearClipboard() {
    clipboard_data_.paths.clear();
    clipboard_data_.is_cut = false;
}

bool FileBrowser::copyFileOrDirectory(const std::string& source, const std::string& target) {
    try {
        fs::path source_path = source;
        fs::path target_path = target;

        if (fs::is_directory(source_path)) {
            // 复制目录（递归）
            fs::create_directories(target_path);
            for (const auto& entry : fs::recursive_directory_iterator(source_path)) {
                fs::path relative_path = fs::relative(entry.path(), source_path);
                fs::path target_file = target_path / relative_path;

                if (entry.is_directory()) {
                    fs::create_directories(target_file);
                } else {
                    fs::copy_file(entry.path(), target_file, fs::copy_options::overwrite_existing);
                }
            }
        } else {
            // 复制文件
            fs::copy_file(source_path, target_path, fs::copy_options::overwrite_existing);
        }

        return true;
    } catch (const std::exception&) {
        return false;
    }
}

// 为了保持向后兼容，保留 render 方法，但使用 FileBrowserView
ftxui::Element FileBrowser::render(int height) {
    static ui::FileBrowserView view(theme_);
    return view.render(*this, height);
}

bool FileBrowser::renameSelected(const std::string& new_name) {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size())
        return false;

    FileItem* item = flat_items_[selected_index_];
    if (item->name == "..")
        return false;

    // ── 远程模式 ──────────────────────────────────────────────────────────────
    if (remote_file_op_exec_) {
        std::string old_remote = extractRemotePath(item->path);
        // 构建同目录下的新路径
        std::string parent = old_remote;
        auto pos = parent.rfind('/');
        if (pos != std::string::npos)
            parent = parent.substr(0, pos);
        std::string new_remote = parent + "/" + new_name;
        std::string cmd = "mv " + shellQuote(old_remote) + " " + shellQuote(new_remote);
        auto [ok, out] = remote_file_op_exec_(cmd);
        if (!ok)
            return false;
        loadDirectory();
        for (size_t i = 0; i < flat_items_.size(); ++i) {
            if (flat_items_[i]->name == new_name) {
                selected_index_ = i;
                break;
            }
        }
        return true;
    }

    // ── 本地模式 ──────────────────────────────────────────────────────────────
    try {
        fs::path old_path = item->path;
        fs::path parent_path = fs::path(old_path).parent_path();
        fs::path new_path = parent_path / new_name;

        if (fs::exists(new_path))
            return false;

        fs::rename(old_path, new_path);
        item->name = new_name;
        item->path = new_path.string();
        loadDirectory();

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
    if (flat_items_.empty() || selected_index_ >= flat_items_.size())
        return false;

    FileItem* item = flat_items_[selected_index_];
    if (item->name == "..")
        return false;

    // ── 远程模式 ──────────────────────────────────────────────────────────────
    if (remote_file_op_exec_) {
        std::string remote_path = extractRemotePath(item->path);
        std::string cmd = item->is_directory ? "rm -rf " + shellQuote(remote_path)
                                             : "rm -f " + shellQuote(remote_path);
        auto [ok, out] = remote_file_op_exec_(cmd);
        if (!ok)
            return false;
        loadDirectory();
        if (selected_index_ >= flat_items_.size() && !flat_items_.empty())
            selected_index_ = flat_items_.size() - 1;
        return true;
    }

    // ── 本地模式 ──────────────────────────────────────────────────────────────
    try {
        fs::path path = item->path;

        DeleteOperation operation;
        operation.name = item->name;
        operation.path = item->path;
        operation.parent_path = fs::path(item->path).parent_path().string();
        operation.is_directory = item->is_directory;
        operation.size = item->size;

        if (item->is_directory) {
            std::function<void(const FileItem&, std::vector<FileItem>&)> collectChildren =
                [&](const FileItem& parent, std::vector<FileItem>& children) {
                    for (const auto& child : parent.children) {
                        FileItem child_copy = child;
                        if (!child.children.empty())
                            collectChildren(child, child_copy.children);
                        children.push_back(child_copy);
                    }
                };
            collectChildren(*item, operation.original_children);
        }

        if (delete_undo_stack_.size() >= 100)
            delete_undo_stack_.pop_front();
        delete_undo_stack_.push_back(operation);

        if (item->is_directory)
            fs::remove_all(path);
        else
            fs::remove(path);

        loadDirectory();
        if (selected_index_ >= flat_items_.size() && !flat_items_.empty())
            selected_index_ = flat_items_.size() - 1;
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileBrowser::createDirectory(const std::string& name) {
    if (name.empty())
        return false;

    // ── 远程模式 ──────────────────────────────────────────────────────────────
    if (remote_file_op_exec_) {
        std::string dir_remote = extractRemotePath(current_directory_);
        std::string new_path = dir_remote;
        if (!new_path.empty() && new_path.back() != '/')
            new_path += '/';
        new_path += name;
        std::string cmd = "mkdir -p " + shellQuote(new_path);
        auto [ok, out] = remote_file_op_exec_(cmd);
        if (!ok)
            return false;
        loadDirectory();
        selectItemByName(name);
        return true;
    }

    // ── 本地模式 ──────────────────────────────────────────────────────────────
    try {
        fs::path folder_path = fs::path(current_directory_) / name;
        if (!fs::create_directory(folder_path))
            return false;
        loadDirectory();
        selectItemByName(name);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileBrowser::moveSelected(const std::string& target_path) {
    if (flat_items_.empty() || selected_index_ >= flat_items_.size())
        return false;

    FileItem* item = flat_items_[selected_index_];
    if (item->name == "..")
        return false;

    // ── 远程模式 ──────────────────────────────────────────────────────────────
    if (remote_file_op_exec_) {
        std::string src_remote = extractRemotePath(item->path);
        // target_path 可能是 ssh:// URI，也可能是裸远程路径
        std::string dst_remote =
            (target_path.find("ssh://") == 0) ? extractRemotePath(target_path) : target_path;
        std::string cmd = "mv " + shellQuote(src_remote) + " " + shellQuote(dst_remote);
        auto [ok, out] = remote_file_op_exec_(cmd);
        if (!ok)
            return false;
        loadDirectory();
        return true;
    }

    // ── 本地模式 ──────────────────────────────────────────────────────────────
    try {
        fs::path source_path = item->path;
        fs::path target = target_path;

        if (!target.is_absolute())
            target = fs::path(current_directory_) / target;

        fs::path target_dir, target_name;
        if (fs::exists(target) && fs::is_directory(target)) {
            target_dir = fs::canonical(target);
            target_name = source_path.filename();
        } else {
            target_dir = fs::canonical(target.parent_path());
            target_name = target.filename();
        }

        fs::path final_target = target_dir / target_name;
        if (fs::equivalent(source_path, final_target))
            return false;
        if (fs::exists(final_target))
            return false;
        if (!fs::exists(target_dir))
            return false;

        fs::rename(source_path, final_target);
        loadDirectory();

        if (fs::equivalent(fs::path(current_directory_), target_dir))
            selectItemByName(target_name.string());
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileBrowser::undoDelete() {
    if (delete_undo_stack_.empty()) {
        return false;
    }

    // 获取最近删除的操作
    DeleteOperation operation = delete_undo_stack_.back();
    delete_undo_stack_.pop_back();

    try {
        fs::path restore_path = fs::path(operation.parent_path) / operation.name;

        // 检查目标位置是否已有同名文件/文件夹
        if (fs::exists(restore_path)) {
            // 尝试使用唯一名称
            int counter = 1;
            std::string base_name = operation.name;
            while (fs::exists(restore_path)) {
                operation.name = base_name + "_restored_" + std::to_string(counter);
                restore_path = fs::path(operation.parent_path) / operation.name;
                counter++;
            }
        }

        if (operation.is_directory) {
            // 恢复目录及其内容
            fs::create_directory(restore_path);

            // 恢复子项
            std::function<void(const std::vector<FileItem>&, const fs::path&)> restoreChildren =
                [&](const std::vector<FileItem>& children, const fs::path& parent_path) {
                    for (const auto& child : children) {
                        fs::path child_path = parent_path / child.name;
                        if (child.is_directory) {
                            fs::create_directory(child_path);
                            // 递归恢复子目录
                            if (!child.children.empty()) {
                                restoreChildren(child.children, child_path);
                            }
                        } else {
                            // 对于文件，由于原始内容已丢失，这里只是重建空文件
                            // 实际使用中可能需要备份原始内容
                            std::ofstream ofs(child_path.string(), std::ios::binary);
                            if (ofs) {
                                ofs.close();
                            }
                        }
                    }
                };

            if (!operation.original_children.empty()) {
                restoreChildren(operation.original_children, restore_path);
            }
        } else {
            // 恢复文件（创建空文件，实际内容需要备份系统支持）
            std::ofstream ofs(restore_path.string(), std::ios::binary);
            if (ofs) {
                ofs.close();
            }
        }

        // 刷新目录列表
        loadDirectory();

        // 选中新恢复的项
        selectItemByName(operation.name);

        return true;
    } catch (const std::exception&) {
        // 恢复失败，将操作放回栈中
        delete_undo_stack_.push_back(operation);
        return false;
    }
}

bool FileBrowser::canUndoDelete() const {
    return !delete_undo_stack_.empty();
}

void FileBrowser::clearUndoStack() {
    delete_undo_stack_.clear();
}

} // namespace features
} // namespace pnana
