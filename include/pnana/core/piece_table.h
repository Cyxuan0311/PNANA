#ifndef PNANA_CORE_PIECE_TABLE_H
#define PNANA_CORE_PIECE_TABLE_H

#include "core/buffer_backend.h"
#include <fstream>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 片段表（Piece Table）实现 - 文本编辑器的"黄金标准"
// 特点：
// - 维护两个缓冲区：原始缓冲区（只读）和追加缓冲区（可写）
// - 使用红黑树管理片段序列，支持 O(log n) 插入删除
// - 内存效率高，不会复制未修改的文本
// - 支持无限撤销/重做（通过保存历史片段树）
// - Notepad++、Vim 等编辑器采用
class PieceTable : public BufferBackend {
  public:
    PieceTable();
    ~PieceTable() override = default;

    BufferBackendType getType() const override {
        return BufferBackendType::PIECE_TABLE;
    }
    const char* getName() const override {
        return "PieceTable";
    }

    // 文本内容操作
    void insert(size_t pos, const std::string& text) override;
    void remove(size_t pos, size_t length) override;
    std::string getText(size_t pos, size_t length) const override;
    std::string getFullText() const override;
    void clear() override {
        original_buffer_.clear();
        append_buffer_.clear();
        root_ = nil_;
        total_length_ = 0;
        line_count_ = 1;
        lines_dirty_ = true;
    }

    // 行操作
    void insertLine(size_t line_num, const std::string& content) override;
    void removeLine(size_t line_num) override;
    std::string getLine(size_t line_num) const override;
    size_t lineCount() const override;

    // 字符操作
    void insertChar(size_t pos, char ch) override;
    void removeChar(size_t pos) override;
    char getChar(size_t pos) const override;

    void replace(size_t pos, size_t length, const std::string& text) override;
    void swapLine(size_t line_a, size_t line_b) override;

    // 查询操作
    size_t length() const override;
    size_t lineLength(size_t line_num) const override;
    size_t positionToLineCol(size_t pos) const override;
    size_t lineColToPosition(size_t line, size_t col) const override;

    // 文件加载和保存
    bool loadFromFile(const std::string& filepath) override;
    bool saveToFile(const std::string& filepath) const override;

    // 内存使用统计
    size_t getMemoryUsage() const override;

    // 优化操作：合并相邻片段
    void optimize() override;

  private:
    enum class BufferType { ORIGINAL, APPEND };

    // 片段结构
    struct Piece {
        BufferType buffer_type;
        size_t start;  // 在缓冲区中的起始位置
        size_t length; // 片段长度

        Piece() : buffer_type(BufferType::ORIGINAL), start(0), length(0) {}
        Piece(BufferType type, size_t s, size_t len) : buffer_type(type), start(s), length(len) {}
    };

    // 红黑树节点
    struct RBNode {
        Piece piece;
        size_t subtree_length;   // 子树总长度
        size_t subtree_newlines; // 子树中换行符数量
        int color;               // 0 = red, 1 = black
        std::shared_ptr<RBNode> left, right, parent;

        RBNode() : subtree_length(0), subtree_newlines(0), color(0) {}
        explicit RBNode(const Piece& p) : piece(p), subtree_length(p.length), color(0) {
            subtree_newlines = 0;
            // newline count will be computed when needed
        }

        bool isLeftChild() const {
            return parent && parent->left.get() == this;
        }
    };

    std::string original_buffer_; // 原始文件内容（只读）
    std::string append_buffer_;   // 追加内容（可写）
    std::shared_ptr<RBNode> root_;
    std::shared_ptr<RBNode> nil_; // 哨兵节点
    mutable size_t total_length_;
    mutable size_t line_count_;
    mutable bool lines_dirty_;

    // 红黑树操作
    void rotateLeft(std::shared_ptr<RBNode> node);
    void rotateRight(std::shared_ptr<RBNode> node);
    void insertFixup(std::shared_ptr<RBNode> node);
    void removeFixup(std::shared_ptr<RBNode> node);

    // 树操作
    std::shared_ptr<RBNode> findNodeAt(size_t pos) const;
    std::pair<std::shared_ptr<RBNode>, size_t> findNodeAndOffset(size_t pos) const;
    void updateNodeInfo(std::shared_ptr<RBNode> node) const;

    // 片段操作
    void insertPiece(size_t pos, BufferType type, size_t start, size_t length);
    void removePieces(size_t pos, size_t length);

    // 遍历（public 以便 lambda 访问内部类型）
  public:
    void traverseInOrder(std::shared_ptr<RBNode> node,
                         std::function<void(const Piece&)> callback) const;

  private:
    // 辅助函数
    size_t countNewlines(const std::string& str, size_t start, size_t len) const;
    size_t findLineStart(size_t line_num) const;

    void recomputeLineCount() const;
};

// PieceTable 工厂类
class PieceTableFactory : public BufferBackendFactory {
  public:
    std::unique_ptr<BufferBackend> create() const override {
        return std::make_unique<PieceTable>();
    }
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_PIECE_TABLE_H
