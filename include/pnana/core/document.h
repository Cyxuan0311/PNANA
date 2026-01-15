#ifndef PNANA_CORE_DOCUMENT_H
#define PNANA_CORE_DOCUMENT_H

#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 文档修改记录（用于撤销/重做）
struct DocumentChange {
    enum class Type { INSERT, DELETE, REPLACE, NEWLINE };

    Type type;
    size_t row;
    size_t col;
    std::string old_content;
    std::string new_content;
    std::string after_cursor;                        // 用于 NEWLINE 类型：光标后的内容
    std::chrono::steady_clock::time_point timestamp; // 时间戳，用于智能合并

    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(""),
          timestamp(std::chrono::steady_clock::now()) {}

    // NEWLINE 类型的构造函数
    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c,
                   const std::string& after)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(after),
          timestamp(std::chrono::steady_clock::now()) {}

    // 带时间戳的构造函数（用于合并操作）
    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c,
                   const std::chrono::steady_clock::time_point& ts)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(""),
          timestamp(ts) {}
};

// 文档类 - 管理单个文件的内容
class Document {
  public:
    Document();
    explicit Document(const std::string& filepath);

    // 文件操作
    bool load(const std::string& filepath);
    bool save();
    bool saveAs(const std::string& filepath);
    bool reload();

    // 内容访问
    size_t lineCount() const {
        return lines_.size();
    }
    const std::string& getLine(size_t row) const;
    const std::vector<std::string>& getLines() const {
        return lines_;
    }
    std::vector<std::string>& getLines() {
        return lines_;
    }

    // 编辑操作
    void insertChar(size_t row, size_t col, char ch);
    void insertText(size_t row, size_t col, const std::string& text);
    void insertLine(size_t row);
    void deleteLine(size_t row);
    void deleteChar(size_t row, size_t col);
    void deleteRange(size_t start_row, size_t start_col, size_t end_row, size_t end_col);
    void replaceLine(size_t row, const std::string& content);

    // 撤销/重做
    // undo 返回是否成功，并通过输出参数返回修改位置
    bool undo(size_t* out_row = nullptr, size_t* out_col = nullptr,
              DocumentChange::Type* out_type = nullptr);
    bool redo(size_t* out_row = nullptr, size_t* out_col = nullptr);
    void pushChange(const DocumentChange& change);
    void clearHistory();

    // 选择和剪贴板
    std::string getSelection(size_t start_row, size_t start_col, size_t end_row,
                             size_t end_col) const;
    void setClipboard(const std::string& content) {
        clipboard_ = content;
    }
    std::string getClipboard() const {
        return clipboard_;
    }

    // 文件信息
    std::string getFilePath() const {
        return filepath_;
    }
    std::string getFileName() const;
    std::string getFileExtension() const;
    bool isModified() const {
        return modified_;
    }
    bool isReadOnly() const {
        return read_only_;
    }
    void setModified(bool modified) {
        modified_ = modified;
    }

    // 编码信息
    std::string getEncoding() const {
        return encoding_;
    }
    void setEncoding(const std::string& encoding) {
        encoding_ = encoding;
    }

    // 行尾类型
    enum class LineEnding { LF, CRLF, CR };
    LineEnding getLineEnding() const {
        return line_ending_;
    }
    void setLineEnding(LineEnding ending) {
        line_ending_ = ending;
    }

    // 错误信息
    std::string getLastError() const {
        return last_error_;
    }

    // 二进制文件检测
    bool isBinary() const {
        return is_binary_;
    }

  private:
    std::vector<std::string> lines_;
    std::vector<std::string> original_lines_; // 保存原始内容（用于判断是否修改）
    std::string filepath_;
    std::string encoding_;
    LineEnding line_ending_;
    bool modified_;
    bool read_only_;

    // 撤销/重做栈
    std::deque<DocumentChange> undo_stack_;
    std::deque<DocumentChange> redo_stack_;
    static constexpr size_t MAX_UNDO_STACK = 1000;

    // 剪贴板
    std::string clipboard_;

    // 错误信息
    std::string last_error_;

    // 二进制文件标志
    bool is_binary_;

    // 辅助方法
    void detectLineEnding(const std::string& content);
    std::string applyLineEnding(const std::string& line) const;
    void saveOriginalContent();           // 保存当前内容作为原始内容
    bool isContentSameAsOriginal() const; // 检查当前内容是否与原始内容相同
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_DOCUMENT_H
