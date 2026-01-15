#include "core/document.h"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace pnana {
namespace core {

Document::Document()
    : filepath_(""), encoding_("UTF-8"), line_ending_(LineEnding::LF), modified_(false),
      read_only_(false), is_binary_(false) {
    lines_.push_back("");
    original_lines_.push_back("");
}

Document::Document(const std::string& filepath) : Document() {
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
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
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
    clearHistory(); // 清除撤销历史，因为已经保存了
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

    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, col, "", std::string(1, ch)));
}

void Document::insertText(size_t row, size_t col, const std::string& text) {
    if (row >= lines_.size() || text.empty()) {
        return;
    }

    if (col > lines_[row].length()) {
        col = lines_[row].length();
    }

    lines_[row].insert(col, text);

    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, col, "", text));
}

void Document::insertLine(size_t row) {
    if (row > lines_.size()) {
        row = lines_.size();
    }

    lines_.insert(lines_.begin() + row, "");

    // 插入新行需要记录到撤销栈
    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, 0, "", "\n"));
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

    pushChange(DocumentChange(DocumentChange::Type::DELETE, row, 0, deleted, ""));
}

void Document::deleteChar(size_t row, size_t col) {
    if (row >= lines_.size()) {
        return;
    }

    if (col < lines_[row].length()) {
        char deleted = lines_[row][col];
        lines_[row].erase(col, 1);

        pushChange(
            DocumentChange(DocumentChange::Type::DELETE, row, col, std::string(1, deleted), ""));
    } else if (row < lines_.size() - 1) {
        // 合并行
        std::string next_line = lines_[row + 1];
        std::string old_line = lines_[row];
        lines_[row] += next_line;
        lines_.erase(lines_.begin() + row + 1);
        // 记录行合并操作
        pushChange(DocumentChange(DocumentChange::Type::REPLACE, row, old_line.length(),
                                  old_line + "\n" + next_line, old_line + next_line));
    }
}

void Document::deleteRange(size_t start_row, size_t /*start_col*/, size_t end_row,
                           size_t /*end_col*/) {
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

    pushChange(DocumentChange(DocumentChange::Type::REPLACE, row, 0, old_content, content));
}

bool Document::undo(size_t* out_row, size_t* out_col, DocumentChange::Type* out_type) {
    if (undo_stack_.empty()) {
        return false;
    }

    DocumentChange change = undo_stack_.back();
    undo_stack_.pop_back();

    // VSCode 风格的撤销逻辑：简单、可靠、原子性
    // 每个操作都是原子性的，直接应用反向操作，信任操作记录
    switch (change.type) {
        case DocumentChange::Type::INSERT: {
            // 撤销插入：删除插入的文本
            // VSCode 行为：直接删除，因为操作是原子性的
            if (change.row < lines_.size()) {
                size_t line_len = lines_[change.row].length();
                size_t insert_len = change.new_content.length();

                // 确保位置有效（更严格的边界检查）
                if (change.col <= line_len) {
                    // 计算实际可删除的长度（确保不超过行尾）
                    size_t max_erase = line_len - change.col;
                    size_t erase_len = std::min(insert_len, max_erase);
                    if (erase_len > 0) {
                        // 直接删除（VSCode 风格：信任操作记录）
                        lines_[change.row].erase(change.col, erase_len);
                    }
                }
            }
            // 光标回到插入开始位置（VSCode 行为）
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::DELETE: {
            // 撤销删除：恢复删除的文本
            // VSCode 行为：直接插入，因为操作是原子性的
            // 如果删除的内容包含换行符，需要分割成多行插入

            size_t insert_row = change.row;
            size_t insert_col = change.col;

            if (change.old_content.find('\n') != std::string::npos) {
                // 多行内容：需要分割并插入多行
                // 确保目标行存在
                while (insert_row >= lines_.size()) {
                    lines_.push_back("");
                }

                std::string& current_line = lines_[insert_row];
                size_t line_len = current_line.length();
                size_t actual_insert_col = std::min(insert_col, line_len);

                // 分割删除的内容（按换行符）
                std::vector<std::string> restored_lines;
                std::istringstream iss(change.old_content);
                std::string line;
                while (std::getline(iss, line)) {
                    // 移除行尾的\r（如果有）
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    restored_lines.push_back(line);
                }

                // 如果原内容以换行符结尾，最后一行应该是空行
                if (!change.old_content.empty() &&
                    (change.old_content.back() == '\n' || change.old_content.back() == '\r')) {
                    restored_lines.push_back("");
                }

                if (!restored_lines.empty()) {
                    // 保存当前行的后半部分（插入位置之后的内容）
                    std::string last_part = current_line.substr(actual_insert_col);

                    // 第一行：当前行前半部分 + 恢复的第一行
                    current_line = current_line.substr(0, actual_insert_col) + restored_lines[0];

                    // 如果有多个行，需要插入新行
                    if (restored_lines.size() > 1) {
                        // 中间行：直接插入为新行
                        for (size_t i = 1; i < restored_lines.size() - 1; ++i) {
                            lines_.insert(lines_.begin() + insert_row + i, restored_lines[i]);
                        }

                        // 最后一行：恢复的最后一行 + 当前行的后半部分
                        std::string last_line = restored_lines.back() + last_part;
                        lines_.insert(lines_.begin() + insert_row + restored_lines.size() - 1,
                                      last_line);
                    } else {
                        // 只有一行，直接追加当前行的后半部分
                        current_line += last_part;
                    }
                }
            } else {
                // 单行内容：判断是删除字符还是删除整行
                // 删除整行的特征：col == 0 且 old_content 不包含换行符且 old_content 非空
                bool is_delete_line = (change.col == 0 && !change.old_content.empty());

                if (is_delete_line) {
                    // 删除整行操作：在指定位置插入新行
                    // 确保插入位置有效
                    while (insert_row > lines_.size()) {
                        lines_.push_back("");
                    }

                    if (insert_row == lines_.size()) {
                        // 在末尾插入新行
                        lines_.push_back(change.old_content);
                    } else {
                        // 在指定位置插入新行
                        lines_.insert(lines_.begin() + insert_row, change.old_content);
                    }
                } else {
                    // 删除字符操作：直接插入到当前行
                    // 确保目标行存在
                    while (insert_row >= lines_.size()) {
                        lines_.push_back("");
                    }

                    std::string& current_line = lines_[insert_row];
                    size_t line_len = current_line.length();
                    // 确保插入位置不超过行尾（VSCode 行为：插入到删除位置）
                    size_t actual_insert_col = std::min(insert_col, line_len);
                    // 直接插入删除的内容
                    current_line.insert(actual_insert_col, change.old_content);
                }
            }

            // 光标回到删除开始位置（VSCode 行为）
            // 对于多行内容，光标应该回到第一行第一列
            // 对于单行内容，光标回到删除开始位置
            if (out_row)
                *out_row = insert_row;
            if (out_col) {
                if (change.old_content.find('\n') != std::string::npos) {
                    *out_col = 0; // 多行内容，光标在行首
                } else {
                    *out_col = insert_col; // 单行内容，光标在删除开始位置
                }
            }
            break;
        }

        case DocumentChange::Type::REPLACE: {
            // 撤销替换：恢复原内容
            if (change.row < lines_.size()) {
                lines_[change.row] = change.old_content;
            } else {
                // 如果行不存在，创建新行
                while (lines_.size() <= change.row) {
                    lines_.push_back("");
                }
                lines_[change.row] = change.old_content;
            }
            // 光标回到替换开始位置（VSCode 行为）
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::NEWLINE: {
            // 撤销换行：删除新行，恢复原行
            // old_content 是完整的原行，new_content 是 before_cursor，after_cursor 是移到新行的内容
            // change.row 是换行前的行号，换行后新行在 change.row + 1 位置

            size_t target_row = change.row;

            // 检查行号是否有效，如果无效，尝试通过内容查找
            if (change.row >= lines_.size() ||
                (change.row < lines_.size() && lines_[change.row] != change.new_content)) {
                // 行号可能已经改变，尝试通过内容查找正确的行
                // 查找包含 before_cursor 的行
                for (size_t i = 0; i < lines_.size(); ++i) {
                    if (lines_[i] == change.new_content && i + 1 < lines_.size()) {
                        target_row = i;
                        break;
                    }
                }
            }

            // 执行撤销：删除新行，恢复原行
            if (target_row < lines_.size()) {
                // 删除新插入的行（如果存在）
                if (target_row + 1 < lines_.size()) {
                    lines_.erase(lines_.begin() + target_row + 1);
                }

                // 恢复原行的完整内容
                lines_[target_row] = change.old_content;
            } else {
                // 如果行不存在，创建新行并恢复内容
                while (lines_.size() <= target_row) {
                    lines_.push_back("");
                }
                lines_[target_row] = change.old_content;
            }

            // 光标回到换行前的位置（VSCode 行为）
            if (out_row)
                *out_row = target_row;
            if (out_col)
                *out_col = change.col;
            break;
        }
    }

    // 确保文档至少有一行（边界情况处理）
    if (lines_.empty()) {
        lines_.push_back("");
    }

    // 将操作移到重做栈
    redo_stack_.push_back(change);

    // 返回操作类型（用于智能光标定位）
    if (out_type) {
        *out_type = change.type;
    }

    // VSCode 行为：如果撤销到最初状态（撤销栈为空），清除修改状态
    if (undo_stack_.empty()) {
        modified_ = false;
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
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
            break;

        case DocumentChange::Type::DELETE:
            if (change.row < lines_.size()) {
                lines_[change.row].erase(change.col, change.old_content.length());
            }
            // 光标应该回到删除开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;

        case DocumentChange::Type::REPLACE:
            if (change.row < lines_.size()) {
                lines_[change.row] = change.new_content;
            }
            // 光标应该移动到替换结束的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.new_content.length();
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
            if (out_row)
                *out_row = change.row + 1;
            if (out_col)
                *out_col = 0;
            break;
    }

    undo_stack_.push_back(change);

    // 如果重做后撤销栈为空，说明回到了原始状态，清除修改状态
    // 否则说明文件被修改了，设置修改状态为 true
    if (undo_stack_.empty()) {
        modified_ = false;
    } else {
        modified_ = true;
    }

    return true;
}

void Document::pushChange(const DocumentChange& change) {
    // VSCode 风格的智能合并策略（优化版）：
    // 1. 连续的 INSERT 操作会合并（连续输入字符，包括多字符输入）
    // 2. 连续的 DELETE 操作会合并（连续删除字符，如连续按 Backspace）
    // 3. INSERT 和 DELETE 不会互相合并（不同操作类型）
    // 4. REPLACE、NEWLINE 等操作总是创建新的撤销点
    // 5. 时间阈值：500ms（VSCode 使用约 500ms，更短的阈值提供更精确的撤销点）
    constexpr auto MERGE_THRESHOLD = std::chrono::milliseconds(500);

    if (!undo_stack_.empty()) {
        DocumentChange& last_change = undo_stack_.back();
        auto time_diff = change.timestamp - last_change.timestamp;

        // VSCode 行为：相同类型的连续操作可以合并
        if (time_diff < MERGE_THRESHOLD) {
            // 合并 INSERT 操作：连续输入字符（支持单字符和多字符）
            if (change.type == DocumentChange::Type::INSERT &&
                last_change.type == DocumentChange::Type::INSERT && change.row == last_change.row) {
                // 检查是否是连续的插入（光标位置连续）
                size_t last_insert_end = last_change.col + last_change.new_content.length();
                if (change.col == last_insert_end) {
                    // 连续插入：将新内容追加到上一个变更
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    return; // 合并完成
                }
            }

            // 合并 DELETE 操作：连续删除字符（VSCode 行为）
            // 注意：DELETE 操作中，col 是删除开始的位置
            // 连续删除时，位置应该相同或相邻
            if (change.type == DocumentChange::Type::DELETE &&
                last_change.type == DocumentChange::Type::DELETE && change.row == last_change.row) {
                // 向后删除（Delete 键）：位置相同，追加删除的内容
                if (change.col == last_change.col) {
                    last_change.old_content += change.old_content;
                    last_change.timestamp = change.timestamp;
                    return; // 合并完成
                }
                // 向前删除（Backspace 键）：位置相邻，插入到开头
                else if (change.col == last_change.col - change.old_content.length() ||
                         change.col + change.old_content.length() == last_change.col) {
                    last_change.old_content = change.old_content + last_change.old_content;
                    last_change.col = change.col; // 更新删除开始位置
                    last_change.timestamp = change.timestamp;
                    return; // 合并完成
                }
            }
        }
    }

    // 不满足合并条件，创建新的撤销点
    // VSCode 行为：不同操作类型、不连续的操作、REPLACE、NEWLINE 都会创建新撤销点
    undo_stack_.push_back(change);
    if (undo_stack_.size() > MAX_UNDO_STACK) {
        undo_stack_.pop_front();
    }
    redo_stack_.clear(); // 新的修改会清除重做栈（VSCode 行为）

    // VSCode 行为：一旦进行了编辑操作，就标记为已修改
    // 只有在用户明确保存文件时，才清除修改状态
    // 不应该在每次操作后都检查是否与原始内容相同
    // 即使撤销到原始状态，文件仍然保持修改状态，直到用户保存
    modified_ = true;
}

void Document::clearHistory() {
    undo_stack_.clear();
    redo_stack_.clear();
}

std::string Document::getSelection(size_t start_row, size_t start_col, size_t end_row,
                                   size_t end_col) const {
    if (start_row >= lines_.size() || end_row >= lines_.size()) {
        return "";
    }

    if (start_row == end_row) {
        const std::string& line = lines_[start_row];
        if (start_col >= line.length()) {
            return "";
        }
        size_t len =
            (end_col <= line.length()) ? (end_col - start_col) : (line.length() - start_col);
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
