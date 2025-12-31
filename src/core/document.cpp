#include "core/document.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <filesystem>

namespace pnana {
namespace core {

Document::Document() 
    : filepath_(""), 
      encoding_("UTF-8"),
      line_ending_(LineEnding::LF),
      modified_(false),
      read_only_(false),
      is_binary_(false) {
    lines_.push_back("");
    original_lines_.push_back("");
}

Document::Document(const std::string& filepath) 
    : Document() {
    load(filepath);
}

bool Document::load(const std::string& filepath) {
    // 检查路径是否是目录
    try {
        if (std::filesystem::exists(filepath) && std::filesystem::is_directory(filepath)) {
            last_error_ = "Cannot open directory as file: " + filepath;
            return false;
        }
    } catch (...) {
        // 如果检查失败，继续尝试打开（可能是新文件）
    }
    
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        // 如果文件不存在，创建新文件
        filepath_ = filepath;
        lines_.clear();
        lines_.push_back("");
        modified_ = false;
        return true;
    }
    
    lines_.clear();
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    file.close();
    
    // 检测二进制文件
    is_binary_ = false;
    if (!content.empty()) {
        // 检查是否包含大量空字符（二进制文件的典型特征）
        size_t null_count = 0;
        size_t check_size = std::min(content.size(), size_t(8192)); // 只检查前8KB
        for (size_t i = 0; i < check_size; ++i) {
            if (content[i] == '\0') {
                null_count++;
            }
        }
        
        // 如果前8KB中有超过1%的空字符，认为是二进制文件
        if (null_count > check_size / 100) {
            is_binary_ = true;
        } else {
            // 检查是否包含大量非可打印字符（排除常见的空白字符）
            size_t non_printable = 0;
            for (size_t i = 0; i < check_size; ++i) {
                unsigned char ch = static_cast<unsigned char>(content[i]);
                // 允许的字符：可打印字符、换行符、制表符、回车符
                if (ch < 32 && ch != '\n' && ch != '\r' && ch != '\t') {
                    non_printable++;
                }
            }
            
            // 如果非可打印字符超过5%，认为是二进制文件
            if (non_printable > check_size / 20) {
                is_binary_ = true;
            }
        }
    }
    
    // 如果是二进制文件，不解析内容
    if (is_binary_) {
        lines_.push_back("");
        filepath_ = filepath;
        modified_ = false;
        return true;
    }
    
    if (content.empty()) {
        lines_.push_back("");
    } else {
        detectLineEnding(content);
        
        std::istringstream iss(content);
        std::string line;
        while (std::getline(iss, line)) {
            // 移除行尾的\r（如果有）
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines_.push_back(line);
        }
        
        // 如果内容以换行符结尾，添加空行
        if (!content.empty() && (content.back() == '\n' || content.back() == '\r')) {
            if (lines_.empty() || !lines_.back().empty()) {
                lines_.push_back("");
            }
        }
    }
    
    filepath_ = filepath;
    modified_ = false;
    // 保存原始内容快照
    saveOriginalContent();
    return true;
}

bool Document::save() {
    if (filepath_.empty()) {
        return false;
    }
    return saveAs(filepath_);
}

bool Document::saveAs(const std::string& filepath) {
    // nano风格的安全保存：
    // 1. 获取原文件权限
    // 2. 写入临时文件
    // 3. 创建备份（可选）
    // 4. 原子性替换原文件
    
    struct stat file_stat;
    bool file_exists = (stat(filepath.c_str(), &file_stat) == 0);
    mode_t original_mode = file_exists ? file_stat.st_mode : 0644;
    
    // 创建临时文件名
    std::string temp_file = filepath + ".tmp~";
    
    // 写入临时文件
    {
        std::ofstream file(temp_file, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            last_error_ = "Cannot create temporary file: " + std::string(strerror(errno));
            return false;
        }
        
        // 写入所有行
        for (size_t i = 0; i < lines_.size(); ++i) {
            file << lines_[i];
            
            // 添加行尾（除了最后一行如果为空）
            if (i < lines_.size() - 1 || !lines_.back().empty()) {
                file << applyLineEnding(lines_[i]);
            }
        }
        
        // 检查写入是否成功
        if (!file.good()) {
            file.close();
            std::remove(temp_file.c_str());
            last_error_ = "Write error: " + std::string(strerror(errno));
            return false;
        }
        
        file.close();
        
        // 确保数据已写入磁盘
        if (file.fail()) {
            std::remove(temp_file.c_str());
            last_error_ = "Failed to close temporary file";
            return false;
        }
    }
    
    // 如果原文件存在，创建临时备份（用于安全保存）
    std::string backup_file = filepath + "~";
    bool backup_created = false;
    
    if (file_exists) {
        // 删除旧备份
        std::remove(backup_file.c_str());
        
        // 创建新备份
        if (std::rename(filepath.c_str(), backup_file.c_str()) == 0) {
            backup_created = true;
        }
        // 备份失败不是致命错误，继续保存
    }
    
    // 原子性替换：重命名临时文件为目标文件
    if (std::rename(temp_file.c_str(), filepath.c_str()) != 0) {
        std::remove(temp_file.c_str());
        // 如果备份存在，尝试恢复
        if (backup_created) {
            std::rename(backup_file.c_str(), filepath.c_str());
        }
        last_error_ = "Cannot rename temp file: " + std::string(strerror(errno));
        return false;
    }
    
    // 恢复原文件权限
    if (file_exists) {
        chmod(filepath.c_str(), original_mode);
    }
    
    // 保存成功后，删除备份文件（避免残留备份文件）
    if (backup_created) {
        std::remove(backup_file.c_str());
    }
    
    // 更新文档状态
    filepath_ = filepath;
    modified_ = false;
    // 保存原始内容快照（保存后的内容就是新的原始内容）
    saveOriginalContent();
    clearHistory();  // 清除撤销历史，因为已经保存了
    last_error_.clear();
    
    return true;
}

bool Document::reload() {
    if (filepath_.empty()) {
        return false;
    }
    return load(filepath_);
}

const std::string& Document::getLine(size_t row) const {
    static const std::string empty;
    if (row >= lines_.size()) {
        return empty;
    }
    return lines_[row];
}

std::string Document::getFileName() const {
    if (filepath_.empty()) {
        return "[Untitled]";
    }
    
    size_t pos = filepath_.find_last_of("/\\");
    if (pos != std::string::npos) {
        return filepath_.substr(pos + 1);
    }
    return filepath_;
}

std::string Document::getFileExtension() const {
    std::string filename = getFileName();
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0) {
        return filename.substr(pos + 1);
    }
    return "";
}

void Document::insertChar(size_t row, size_t col, char ch) {
    if (row >= lines_.size()) {
        return;
    }
    
    if (col > lines_[row].length()) {
        col = lines_[row].length();
    }
    
    std::string old_line = lines_[row];
    lines_[row].insert(col, 1, ch);
    
    pushChange(DocumentChange(
        DocumentChange::Type::INSERT,
        row, col, "", std::string(1, ch)
    ));
}

void Document::insertText(size_t row, size_t col, const std::string& text) {
    if (row >= lines_.size() || text.empty()) {
        return;
    }
    
    if (col > lines_[row].length()) {
        col = lines_[row].length();
    }
    
    lines_[row].insert(col, text);
    
    pushChange(DocumentChange(
        DocumentChange::Type::INSERT,
        row, col, "", text
    ));
}

void Document::insertLine(size_t row) {
    if (row > lines_.size()) {
        row = lines_.size();
    }
    
    lines_.insert(lines_.begin() + row, "");
    
    // 插入新行需要记录到撤销栈
    pushChange(DocumentChange(
        DocumentChange::Type::INSERT,
        row, 0, "", "\n"
    ));
}

void Document::deleteLine(size_t row) {
    if (row >= lines_.size()) {
        return;
    }
    
    std::string deleted = lines_[row];
    
    if (lines_.size() == 1) {
        lines_[0] = "";
    } else {
        lines_.erase(lines_.begin() + row);
    }
    
    pushChange(DocumentChange(
        DocumentChange::Type::DELETE,
        row, 0, deleted, ""
    ));
}

void Document::deleteChar(size_t row, size_t col) {
    if (row >= lines_.size()) {
        return;
    }
    
    if (col < lines_[row].length()) {
        char deleted = lines_[row][col];
        lines_[row].erase(col, 1);
        
        pushChange(DocumentChange(
            DocumentChange::Type::DELETE,
            row, col, std::string(1, deleted), ""
        ));
    } else if (row < lines_.size() - 1) {
        // 合并行
        std::string next_line = lines_[row + 1];
        std::string old_line = lines_[row];
        lines_[row] += next_line;
        lines_.erase(lines_.begin() + row + 1);
        // 记录行合并操作
        pushChange(DocumentChange(
            DocumentChange::Type::REPLACE,
            row, old_line.length(), old_line + "\n" + next_line, old_line + next_line
        ));
    }
}

void Document::deleteRange(size_t start_row, size_t /*start_col*/,
                          size_t end_row, size_t /*end_col*/) {
    if (start_row >= lines_.size() || end_row >= lines_.size()) {
        return;
    }
    
    // 简化实现：后续可以优化
    // 注意：这里没有调用 pushChange，因为这是一个简化的实现
    // 如果需要完整的撤销支持，应该在这里记录删除的内容
}

void Document::replaceLine(size_t row, const std::string& content) {
    if (row >= lines_.size()) {
        return;
    }
    
    std::string old_content = lines_[row];
    lines_[row] = content;
    
    pushChange(DocumentChange(
        DocumentChange::Type::REPLACE,
        row, 0, old_content, content
    ));
}

bool Document::undo(size_t* out_row, size_t* out_col) {
    if (undo_stack_.empty()) {
        return false;
    }
    
    DocumentChange change = undo_stack_.back();
    undo_stack_.pop_back();
    
    // 应用反向操作
    switch (change.type) {
        case DocumentChange::Type::INSERT:
            // 删除插入的内容
            if (change.row < lines_.size()) {
                size_t len = change.new_content.length();
                if (change.col + len <= lines_[change.row].length()) {
                    lines_[change.row].erase(change.col, len);
                }
            }
            // 光标应该回到插入位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col;
            break;
            
        case DocumentChange::Type::DELETE:
            // 恢复删除的内容
            if (change.row < lines_.size()) {
                lines_[change.row].insert(change.col, change.old_content);
            }
            // 光标应该回到删除开始的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col;
            break;
            
        case DocumentChange::Type::REPLACE:
            // 恢复原内容
            if (change.row < lines_.size()) {
                lines_[change.row] = change.old_content;
            }
            // 光标应该回到替换开始的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col;
            break;
            
        case DocumentChange::Type::NEWLINE:
            // 撤销换行：删除新插入的行，恢复原行的完整内容
            if (change.row + 1 < lines_.size()) {
                // 删除新插入的行
                lines_.erase(lines_.begin() + change.row + 1);
            }
            // 恢复原行的完整内容（before_cursor + after_cursor）
            if (change.row < lines_.size()) {
                lines_[change.row] = change.old_content + change.after_cursor;
            }
            // 光标应该回到换行前的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col;
            break;
    }
    
    redo_stack_.push_back(change);
    
    // 检查撤销后内容是否与原始内容相同（参考 VSCode 行为）
    if (isContentSameAsOriginal()) {
        modified_ = false;
    } else {
        modified_ = true;
    }
    
    return true;
}

bool Document::redo(size_t* out_row, size_t* out_col) {
    if (redo_stack_.empty()) {
        return false;
    }
    
    DocumentChange change = redo_stack_.back();
    redo_stack_.pop_back();
    
    // 重新应用操作
    switch (change.type) {
        case DocumentChange::Type::INSERT:
            if (change.row < lines_.size()) {
                lines_[change.row].insert(change.col, change.new_content);
            }
            // 光标应该移动到插入结束的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col + change.new_content.length();
            break;
            
        case DocumentChange::Type::DELETE:
            if (change.row < lines_.size()) {
                lines_[change.row].erase(change.col, change.old_content.length());
            }
            // 光标应该回到删除开始的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.col;
            break;
            
        case DocumentChange::Type::REPLACE:
            if (change.row < lines_.size()) {
                lines_[change.row] = change.new_content;
            }
            // 光标应该移动到替换结束的位置
            if (out_row) *out_row = change.row;
            if (out_col) *out_col = change.new_content.length();
            break;
            
        case DocumentChange::Type::NEWLINE:
            // 重做换行：分割当前行，插入新行
            if (change.row < lines_.size()) {
                // 设置当前行为 before_cursor
                lines_[change.row] = change.new_content;
                // 插入新行并设置 after_cursor
                if (change.row + 1 <= lines_.size()) {
                    lines_.insert(lines_.begin() + change.row + 1, change.after_cursor);
                }
            }
            // 光标应该移动到新行的开始
            if (out_row) *out_row = change.row + 1;
            if (out_col) *out_col = 0;
            break;
    }
    
    undo_stack_.push_back(change);
    
    // 检查重做后内容是否与原始内容相同（参考 VSCode 行为）
    if (isContentSameAsOriginal()) {
        modified_ = false;
    } else {
        modified_ = true;
    }
    
    return true;
}

void Document::pushChange(const DocumentChange& change) {
    undo_stack_.push_back(change);
    if (undo_stack_.size() > MAX_UNDO_STACK) {
        undo_stack_.pop_front();
    }
    redo_stack_.clear();  // 新的修改会清除重做栈（参考 VSCode 行为）
    
    // 检查修改后内容是否与原始内容相同
    if (isContentSameAsOriginal()) {
        modified_ = false;
    } else {
        modified_ = true;
    }
}

void Document::clearHistory() {
    undo_stack_.clear();
    redo_stack_.clear();
}

std::string Document::getSelection(size_t start_row, size_t start_col,
                                  size_t end_row, size_t end_col) const {
    if (start_row >= lines_.size() || end_row >= lines_.size()) {
        return "";
    }
    
    if (start_row == end_row) {
        const std::string& line = lines_[start_row];
        if (start_col >= line.length()) {
            return "";
        }
        size_t len = (end_col <= line.length()) ? (end_col - start_col) : (line.length() - start_col);
        return line.substr(start_col, len);
    }
    
    std::string result;
    for (size_t row = start_row; row <= end_row; ++row) {
        if (row == start_row) {
            result += lines_[row].substr(start_col);
        } else if (row == end_row) {
            result += "\n" + lines_[row].substr(0, end_col);
        } else {
            result += "\n" + lines_[row];
        }
    }
    
    return result;
}

void Document::detectLineEnding(const std::string& content) {
    if (content.find("\r\n") != std::string::npos) {
        line_ending_ = LineEnding::CRLF;
    } else if (content.find('\r') != std::string::npos) {
        line_ending_ = LineEnding::CR;
    } else {
        line_ending_ = LineEnding::LF;
    }
}

std::string Document::applyLineEnding(const std::string& /* line */) const {
    switch (line_ending_) {
        case LineEnding::CRLF:
            return "\r\n";
        case LineEnding::CR:
            return "\r";
        case LineEnding::LF:
        default:
            return "\n";
    }
}

void Document::saveOriginalContent() {
    original_lines_ = lines_;
}

bool Document::isContentSameAsOriginal() const {
    // 比较当前内容与原始内容是否完全相同
    if (lines_.size() != original_lines_.size()) {
        return false;
    }
    
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (lines_[i] != original_lines_[i]) {
            return false;
        }
    }
    
    return true;
}

} // namespace core
} // namespace pnana

