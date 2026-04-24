#ifndef PNANA_CORE_DOCUMENT_H
#define PNANA_CORE_DOCUMENT_H

#include "core/buffer_backend.h"
#include "core/buffer_factory.h"
#include "features/lsp/lsp_types.h"
#include <chrono>
#include <cstdint>
#include <deque>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace pnana {
namespace core {

// 文档修改记录（用于撤销/重做）
struct DocumentChange {
    enum class Type { INSERT, DELETE, REPLACE, NEWLINE, COMPLETION, MOVE_LINE, COMMENT_TOGGLE };

    struct MoveTag {};
    static constexpr MoveTag MOVE_TAG = MoveTag{};

    Type type;
    size_t row;
    size_t col;
    std::string old_content;
    std::string new_content;
    std::string after_cursor;
    std::chrono::steady_clock::time_point timestamp;
    size_t target_row;

    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(""),
          timestamp(std::chrono::steady_clock::now()), target_row(0) {}

    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c,
                   const std::string& after)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(after),
          timestamp(std::chrono::steady_clock::now()), target_row(0) {}

    DocumentChange(Type t, size_t r, size_t c, const std::string& replaced_text,
                   const std::string& completion_text, bool /*is_completion*/)
        : type(t), row(r), col(c), old_content(replaced_text), new_content(completion_text),
          after_cursor(""), timestamp(std::chrono::steady_clock::now()), target_row(0) {}

    DocumentChange(Type t, size_t r, size_t c, const std::string& old_c, const std::string& new_c,
                   const std::chrono::steady_clock::time_point& ts)
        : type(t), row(r), col(c), old_content(old_c), new_content(new_c), after_cursor(""),
          timestamp(ts), target_row(0) {}

    DocumentChange(Type t, size_t r, size_t target_r, const std::string& old_c,
                   const std::string& new_c, MoveTag)
        : type(t), row(r), col(0), old_content(old_c), new_content(new_c), after_cursor(""),
          timestamp(std::chrono::steady_clock::now()), target_row(target_r) {}
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

    // 缓冲区后端管理
    BufferBackendType getBufferBackendType() const {
        return backend_type_;
    }
    const char* getBufferBackendName() const;
    void setBufferBackend(BufferBackendType type);
    void autoSelectBufferBackend(); // 根据文件类型和大小自动选择

    // 内容访问
    size_t lineCount() const;
    const std::string& getLine(size_t row) const;
    const std::vector<std::string>& getLines() const;
    std::vector<std::string>& getLines();

    // 获取完整的文档内容（所有行合并）
    std::string getContent() const;

    // 编辑操作（使用缓冲区后端）
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
    void setFilePath(const std::string& filepath) {
        filepath_ = filepath;
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

    // 折叠范围管理
    void setFoldingRanges(const std::vector<pnana::features::FoldingRange>& ranges);
    const std::vector<pnana::features::FoldingRange>& getFoldingRanges() const {
        return folding_ranges_;
    }
    void clearFoldingRanges();

    // 折叠状态管理
    void setFolded(int start_line, bool folded);
    bool isFolded(int line) const;
    bool isLineInFoldedRange(int line) const;
    void toggleFold(int start_line);
    void unfoldAll();
    void foldAll();

    // 获取可见行（排除折叠的行）
    std::vector<size_t> getVisibleLines(size_t start_line = 0, size_t end_line = SIZE_MAX) const;
    size_t getVisibleLineCount() const;

    // 将显示行号转换为实际行号
    size_t displayLineToActualLine(size_t display_line) const;
    // 将实际行号转换为显示行号
    size_t actualLineToDisplayLine(size_t actual_line) const;

  private:
    // 缓冲区后端（支持多种实现：GapBuffer, SqrtDecomposition, Rope, PieceTable）
    std::unique_ptr<BufferBackend> buffer_backend_;
    BufferBackendType backend_type_;

    // 保留原有的 lines_ 作为缓存（兼容旧代码）
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

    // 大文件不保存 original 快照，仅用 modified_ 判断是否修改（节省 saveOriginalContent
    // 耗时与内存）
    bool large_file_skip_original_ = false;

    // 懒加载大文件：只建行偏移表，按需读行；首次编辑或 getLines() 时 materialize
    bool lazy_loaded_ = false;
    std::vector<uint64_t> line_offsets_; // line_offsets_[i] = 第 i 行起始字节偏移，size = 行数+1
    mutable std::unordered_map<size_t, std::string> line_cache_;
    mutable std::deque<size_t> line_cache_lru_;
    static constexpr size_t LINE_CACHE_MAX = 4096;
    void materialize(); // 将懒加载文档全部读入 lines_，并关闭懒加载
    std::string loadLineFromFile(size_t row) const;

    // 折叠范围
    std::vector<pnana::features::FoldingRange> folding_ranges_;

    // 已折叠的行范围（存储起始行号）
    std::set<int> folded_lines_;

    // 辅助方法
    void detectLineEnding(const std::string& content);
    std::string applyLineEnding(const std::string& line) const;
    void saveOriginalContent();           // 保存当前内容作为原始内容
    bool isContentSameAsOriginal() const; // 检查当前内容是否与原始内容相同
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_DOCUMENT_H
