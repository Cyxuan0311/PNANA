#include "core/document.h"
#include "utils/logger.h"
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

std::string Document::getContent() const {
    std::string content;
    for (size_t i = 0; i < lines_.size(); ++i) {
        content += lines_[i];
        if (i < lines_.size() - 1) {
            content += "\n"; // 添加换行符，除了最后一行
        }
    }
    return content;
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

    // 插入新行需要记录到撤销栈 - 使用特殊的DELETE类型表示撤销时删除这一行
    pushChange(DocumentChange(DocumentChange::Type::DELETE, row, 0, "", ""));
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

    // 删除行需要记录到撤销栈 - 使用特殊的INSERT类型表示撤销时插入这一行
    pushChange(DocumentChange(DocumentChange::Type::INSERT, row, 0, "", deleted));
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

void Document::deleteRange(size_t start_row, size_t start_col, size_t end_row, size_t end_col) {
    // Treat end_col as exclusive.
    if (start_row >= lines_.size() || end_row >= lines_.size() || start_row > end_row) {
        return;
    }

    // Clamp columns
    std::string old_content;

    if (start_row == end_row) {
        const std::string& line = lines_[start_row];
        size_t sc = std::min(start_col, line.length());
        size_t ec = std::min(end_col, line.length());
        if (sc >= ec) {
            return; // nothing to delete
        }
        old_content = line.substr(sc, ec - sc);
        lines_[start_row].erase(sc, ec - sc);
        pushChange(DocumentChange(DocumentChange::Type::DELETE, start_row, sc, old_content, ""));
        return;
    }

    // Multi-line deletion
    // first line: keep prefix before start_col
    size_t sc = std::min(start_col, lines_[start_row].length());
    size_t ec = std::min(end_col, lines_[end_row].length());

    // Build old_content = suffix of first line (from sc) + '\n' + middle lines + '\n' + prefix of
    // last line (0..ec)
    old_content = lines_[start_row].substr(sc);
    old_content += '\n';
    for (size_t r = start_row + 1; r < end_row; ++r) {
        old_content += lines_[r];
        old_content += '\n';
    }
    old_content += lines_[end_row].substr(0, ec);

    // Construct new first line = prefix before sc + suffix after ec of last line
    std::string new_first = lines_[start_row].substr(0, sc) + lines_[end_row].substr(ec);

    // Erase range of lines and replace first line with new_first
    lines_.erase(lines_.begin() + start_row, lines_.begin() + end_row + 1);
    lines_.insert(lines_.begin() + start_row, new_first);

    pushChange(DocumentChange(DocumentChange::Type::DELETE, start_row, sc, old_content, ""));
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
        LOG("[UNDO] No operations to undo");
        return false;
    }

    DocumentChange change = undo_stack_.back();
    undo_stack_.pop_back();
    LOG("[UNDO] Applying undo for operation: type=" +
        std::to_string(static_cast<int>(change.type)) + " row=" + std::to_string(change.row) +
        " col=" + std::to_string(change.col) +
        " old_len=" + std::to_string(change.old_content.length()) +
        " new_len=" + std::to_string(change.new_content.length()));

    // VSCode 风格的撤销逻辑：原子性操作，直接应用反向操作
    // 每个撤销点都是完整的、不可分割的操作
    bool success = false;
    switch (change.type) {
        case DocumentChange::Type::INSERT: {
            // 撤销插入操作：删除之前插入的内容
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t line_len = current_line.length();

                // 边界检查：确保插入位置有效
                if (change.col <= line_len) {
                    size_t insert_len = change.new_content.length();
                    size_t max_erase = line_len - change.col;
                    size_t erase_len = std::min(insert_len, max_erase);

                    if (erase_len > 0) {
                        // 验证要删除的内容是否匹配（额外的安全检查）
                        if (current_line.substr(change.col, erase_len) ==
                            change.new_content.substr(0, erase_len)) {
                            current_line.erase(change.col, erase_len);
                            success = true;
                            LOG("[UNDO] Successfully undid INSERT operation");
                        } else {
                            LOG("[UNDO] Content mismatch during INSERT undo - possible data "
                                "corruption");
                        }
                    } else {
                        LOG("[UNDO] No content to erase for INSERT undo");
                        success = true; // 空操作也算成功
                    }
                } else {
                    LOG("[UNDO] Invalid column position for INSERT undo: " +
                        std::to_string(change.col) + " > " + std::to_string(line_len));
                }
            } else {
                LOG("[UNDO] Invalid row for INSERT undo: " + std::to_string(change.row));
            }

            // 撤销后的光标位置：回到插入开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::DELETE: {
            // 撤销删除操作：恢复之前删除的内容
            if (change.row >= lines_.size()) {
                LOG("[UNDO] Invalid row for DELETE undo: " + std::to_string(change.row));
                break;
            }

            std::string& current_line = lines_[change.row];
            size_t line_len = current_line.length();

            // 边界检查：确保删除位置有效
            if (change.col > line_len) {
                LOG("[UNDO] Invalid column for DELETE undo: " + std::to_string(change.col) + " > " +
                    std::to_string(line_len));
                break;
            }

            // 检查是否是多行删除（包含换行符）
            if (change.old_content.find('\n') != std::string::npos) {
                // 多行删除的撤销：需要重新构建多行内容
                std::vector<std::string> restored_lines;
                std::istringstream iss(change.old_content);
                std::string line;

                while (std::getline(iss, line)) {
                    // 处理行尾的\r字符
                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }
                    restored_lines.push_back(line);
                }

                // 如果原始内容以换行符结尾，需要添加空行
                if (!change.old_content.empty() &&
                    (change.old_content.back() == '\n' || change.old_content.back() == '\r')) {
                    restored_lines.push_back("");
                }

                if (!restored_lines.empty()) {
                    // 保存插入位置之后的内容
                    std::string remaining_part = current_line.substr(change.col);

                    // 第一行：当前行前半部分 + 恢复的第一行
                    current_line = current_line.substr(0, change.col) + restored_lines[0];

                    // 如果有多行内容需要插入
                    if (restored_lines.size() > 1) {
                        // 插入中间行
                        for (size_t i = 1; i < restored_lines.size() - 1; ++i) {
                            lines_.insert(lines_.begin() + change.row + i, restored_lines[i]);
                        }

                        // 最后一行：恢复内容 + 原始的后半部分
                        std::string last_line = restored_lines.back() + remaining_part;
                        lines_.insert(lines_.begin() + change.row + restored_lines.size() - 1,
                                      last_line);
                    } else {
                        // 只有一行，直接追加剩余内容
                        current_line += remaining_part;
                    }

                    success = true;
                    LOG("[UNDO] Successfully undid multiline DELETE operation");
                }
            } else {
                // 单行删除的撤销：直接在指定位置插入内容
                current_line.insert(change.col, change.old_content);
                success = true;
                LOG("[UNDO] Successfully undid single-line DELETE operation");
            }

            // 撤销后的光标位置：回到删除开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col) {
                *out_col = (change.old_content.find('\n') != std::string::npos) ? 0 : change.col;
            }
            break;
        }

        case DocumentChange::Type::REPLACE: {
            // 撤销替换操作：恢复原始内容
            if (change.row < lines_.size()) {
                // 验证当前内容是否与预期的新内容匹配（安全检查）
                if (lines_[change.row] == change.new_content) {
                    lines_[change.row] = change.old_content;
                    success = true;
                    LOG("[UNDO] Successfully undid REPLACE operation");
                } else {
                    LOG("[UNDO] Content mismatch during REPLACE undo - expected: '" +
                        change.new_content + "', found: '" + lines_[change.row] + "'");
                }
            } else {
                LOG("[UNDO] Invalid row for REPLACE undo: " + std::to_string(change.row));
            }

            // 撤销后的光标位置：回到替换开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::NEWLINE: {
            // 撤销换行操作：合并两行，恢复原始行
            size_t target_row = change.row;

            // 查找正确的行（通过内容匹配）
            bool found_target = false;
            if (target_row < lines_.size() && lines_[target_row] == change.new_content) {
                found_target = true;
            } else {
                // 尝试查找包含新行第一部分内容的行
                for (size_t i = 0; i < lines_.size(); ++i) {
                    if (lines_[i] == change.new_content && i + 1 < lines_.size()) {
                        target_row = i;
                        found_target = true;
                        break;
                    }
                }
            }

            if (found_target && target_row + 1 < lines_.size()) {
                // 合并两行：第一行 + 换行符 + 第二行
                lines_[target_row] = change.old_content;
                lines_.erase(lines_.begin() + target_row + 1);
                success = true;
                LOG("[UNDO] Successfully undid NEWLINE operation");
            } else {
                LOG("[UNDO] Could not find valid NEWLINE operation to undo at row " +
                    std::to_string(target_row));
            }

            // 撤销后的光标位置：回到换行前的位置
            if (out_row)
                *out_row = target_row;
            if (out_col)
                *out_col = change.col;
            break;
        }

        case DocumentChange::Type::COMPLETION: {
            // 撤销补全操作：恢复被补全替换的原始文本
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t replace_start = change.col;

                if (replace_start <= current_line.length()) {
                    const std::string& completion_text = change.new_content;

                    // 精确匹配：检查指定位置是否正好是补全文本
                    if (replace_start + completion_text.length() <= current_line.length() &&
                        current_line.substr(replace_start, completion_text.length()) ==
                            completion_text) {
                        current_line.replace(replace_start, completion_text.length(),
                                             change.old_content);
                        success = true;
                        LOG("[UNDO] Successfully undid COMPLETION operation");

                    } else {
                        // 回退策略：在行中搜索补全文本
                        size_t found_pos = current_line.find(completion_text, replace_start);
                        if (found_pos != std::string::npos) {
                            current_line.replace(found_pos, completion_text.length(),
                                                 change.old_content);
                            success = true;
                            LOG("[UNDO] Successfully undid COMPLETION operation (fallback search)");
                        } else {
                            LOG("[UNDO] Could not find completion text to undo: '" +
                                completion_text + "'");
                        }
                    }
                } else {
                    LOG("[UNDO] Invalid position for COMPLETION undo: col=" +
                        std::to_string(replace_start) +
                        " > line_length=" + std::to_string(current_line.length()));
                }
            } else {
                LOG("[UNDO] Invalid row for COMPLETION undo: " + std::to_string(change.row));
            }

            // 撤销后的光标位置：回到补全开始的位置
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col;
            break;
        }
    }

    // 确保文档至少有一行（边界情况处理）
    if (lines_.empty()) {
        lines_.push_back("");
        LOG("[UNDO] Added empty line to maintain document integrity");
    }

    // 将操作移到重做栈（用于重做功能）
    redo_stack_.push_back(change);

    // 返回操作类型（用于智能光标定位）
    if (out_type) {
        *out_type = change.type;
    }

    // 详细的撤销完成日志
    std::string cursor_pos = "(" + std::to_string(out_row ? *out_row : 0) + "," +
                             std::to_string(out_col ? *out_col : 0) + ")";
    LOG("[UNDO] Completed undo operation: type=" + std::to_string(static_cast<int>(change.type)) +
        " success=" + (success ? "true" : "false") + " cursor=" + cursor_pos);

    // VSCode 行为：如果撤销栈为空，说明回到了初始状态，清除修改标志
    if (undo_stack_.empty()) {
        modified_ = false;
        LOG("[UNDO] Reached initial state, cleared modified flag");
    }

    return success;
}

bool Document::redo(size_t* out_row, size_t* out_col) {
    if (redo_stack_.empty()) {
        return false;
    }

    DocumentChange change = redo_stack_.back();
    redo_stack_.pop_back();
    LOG("[REDO] Popped change: type=" + std::to_string(static_cast<int>(change.type)) +
        " row=" + std::to_string(change.row) + " col=" + std::to_string(change.col) +
        " old_len=" + std::to_string(change.old_content.length()) +
        " new_len=" + std::to_string(change.new_content.length()));

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

        case DocumentChange::Type::COMPLETION:
            // 重做补全：重新应用补全文本替换
            if (change.row < lines_.size()) {
                std::string& current_line = lines_[change.row];
                size_t replace_start = change.col;

                // 优先匹配原始被替换文本，确保我们准确重做补全（不会重复添加）
                if (replace_start <= current_line.length()) {
                    const std::string& expected_old = change.old_content;
                    // 如果当前位置正好是原始文本，直接替换为补全文本
                    if (replace_start + expected_old.length() <= current_line.length() &&
                        current_line.substr(replace_start, expected_old.length()) == expected_old) {
                        current_line.replace(replace_start, expected_old.length(),
                                             change.new_content);
                    } else {
                        // 否则尝试在行中查找第一次出现的位置并替换（从 replace_start 开始查找）
                        size_t found = current_line.find(expected_old, replace_start);
                        if (found != std::string::npos) {
                            current_line.replace(found, expected_old.length(), change.new_content);
                        } else {
                            // 回退策略：按长度替换（尽量避免，但作为最后手段）
                            size_t max_replace = current_line.length() - replace_start;
                            size_t actual_replace_len =
                                std::min(expected_old.length(), max_replace);
                            if (actual_replace_len > 0) {
                                current_line.replace(replace_start, actual_replace_len,
                                                     change.new_content);
                            }
                        }
                    }
                }
            }
            // 光标应该移动到补全文本的末尾（VSCode 行为）
            if (out_row)
                *out_row = change.row;
            if (out_col)
                *out_col = change.col + change.new_content.length();
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
    // VSCode 风格的智能合并策略（优化版）
    // 核心原则：连续的相同类型操作会被合并，不同类型操作创建新的撤销点
    constexpr auto MERGE_THRESHOLD = std::chrono::milliseconds(500);

    LOG("[PUSHCHANGE] Incoming change: type=" + std::to_string(static_cast<int>(change.type)) +
        " row=" + std::to_string(change.row) + " col=" + std::to_string(change.col) +
        " old_len=" + std::to_string(change.old_content.length()) +
        " new_len=" + std::to_string(change.new_content.length()));

    // 原子操作：这些操作类型永远不会合并，必须创建新的撤销点
    if (change.type == DocumentChange::Type::COMPLETION ||
        change.type == DocumentChange::Type::REPLACE ||
        change.type == DocumentChange::Type::NEWLINE) {
        LOG("[PUSHCHANGE] Atomic operation: adding new undo point for " +
            std::to_string(static_cast<int>(change.type)));
        undo_stack_.push_back(change);
        if (undo_stack_.size() > MAX_UNDO_STACK) {
            undo_stack_.pop_front();
        }
        redo_stack_.clear(); // 新的修改清除重做栈
        modified_ = true;
        return;
    }

    // 尝试合并连续的INSERT或DELETE操作
    if (!undo_stack_.empty()) {
        DocumentChange& last_change = undo_stack_.back();
        auto time_diff = change.timestamp - last_change.timestamp;

        // 只在时间阈值内尝试合并
        if (time_diff < MERGE_THRESHOLD && change.row == last_change.row) {
            // 合并INSERT操作：连续输入字符
            if (change.type == DocumentChange::Type::INSERT &&
                last_change.type == DocumentChange::Type::INSERT) {
                // 检查是否是连续插入（新插入位置正好在上次插入结束位置）
                size_t last_insert_end = last_change.col + last_change.new_content.length();
                if (change.col == last_insert_end) {
                    LOG("[PUSHCHANGE] Merging consecutive INSERT operations at row=" +
                        std::to_string(change.row));
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    return; // 合并完成，不创建新撤销点
                }
                // 或者是在同一位置的插入（覆盖输入）
                else if (change.col == last_change.col) {
                    LOG("[PUSHCHANGE] Merging overlapping INSERT operations at row=" +
                        std::to_string(change.row) + " col=" + std::to_string(change.col));
                    // 这种情况较少见，但可能发生在快速输入时
                    last_change.new_content += change.new_content;
                    last_change.timestamp = change.timestamp;
                    return;
                }
            }

            // 合并DELETE操作：连续删除字符
            else if (change.type == DocumentChange::Type::DELETE &&
                     last_change.type == DocumentChange::Type::DELETE) {
                // 向后删除（Delete键）：在同一位置连续删除
                if (change.col == last_change.col) {
                    LOG("[PUSHCHANGE] Merging consecutive forward DELETE operations");
                    last_change.old_content += change.old_content;
                    last_change.timestamp = change.timestamp;
                    return;
                }
                // 向前删除（Backspace键）：位置连续
                else if (change.col + change.old_content.length() == last_change.col) {
                    LOG("[PUSHCHANGE] Merging consecutive backward DELETE operations");
                    // 将新的删除内容插入到开头
                    last_change.old_content = change.old_content + last_change.old_content;
                    last_change.col = change.col; // 更新删除起始位置
                    last_change.timestamp = change.timestamp;
                    return;
                }
            }
        }
    }

    // 创建新的撤销点
    LOG("[PUSHCHANGE] Creating new undo point for operation type=" +
        std::to_string(static_cast<int>(change.type)));
    undo_stack_.push_back(change);
    if (undo_stack_.size() > MAX_UNDO_STACK) {
        undo_stack_.pop_front();
    }
    redo_stack_.clear(); // 新的修改清除重做栈
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

// 折叠范围管理
void Document::setFoldingRanges(const std::vector<pnana::features::FoldingRange>& ranges) {
    folding_ranges_ = ranges;
    // 注意：不要在这里清理folded_lines_，因为setFolded方法会单独调用来设置折叠状态
}

void Document::clearFoldingRanges() {
    folding_ranges_.clear();
    folded_lines_.clear();
}

void Document::setFolded(int start_line, bool folded) {
    (void)start_line;
    (void)folded;
    if (folded) {
        // 检查是否有对应的折叠范围
        for (const auto& range : folding_ranges_) {
            if (range.startLine == start_line) {
                folded_lines_.insert(start_line);
                return;
            }
        }
        // no-op for debug
    } else {
        folded_lines_.erase(start_line);
        // no-op for debug
    }
}

bool Document::isFolded(int line) const {
    return folded_lines_.count(line) > 0;
}

bool Document::isLineInFoldedRange(int line) const {
    for (const auto& range : folding_ranges_) {
        if (folded_lines_.count(range.startLine) && range.containsLine(line) &&
            line != range.startLine) {
            return true;
        }
    }
    return false;
}

void Document::toggleFold(int start_line) {
    if (isFolded(start_line)) {
        setFolded(start_line, false);
    } else {
        setFolded(start_line, true);
    }
}

void Document::unfoldAll() {
    folded_lines_.clear();
}

void Document::foldAll() {
    folded_lines_.clear();
    for (const auto& range : folding_ranges_) {
        folded_lines_.insert(range.startLine);
    }
}

std::vector<size_t> Document::getVisibleLines(size_t start_line, size_t end_line) const {
    std::vector<size_t> visible_lines;
    size_t max_line = std::min(end_line, lineCount() - 1);

    for (size_t line = start_line; line <= max_line; ++line) {
        if (!isLineInFoldedRange(static_cast<int>(line))) {
            visible_lines.push_back(line);
        }
    }

    return visible_lines;
}

size_t Document::getVisibleLineCount() const {
    return getVisibleLines(0, lineCount() - 1).size();
}

size_t Document::displayLineToActualLine(size_t display_line) const {
    auto visible_lines = getVisibleLines(0, lineCount() - 1);
    if (display_line < visible_lines.size()) {
        return visible_lines[display_line];
    }
    return lineCount() - 1; // 返回最后一行
}

size_t Document::actualLineToDisplayLine(size_t actual_line) const {
    auto visible_lines = getVisibleLines(0, actual_line);
    return visible_lines.size() - 1; // 返回可见行索引
}

} // namespace core
} // namespace pnana
