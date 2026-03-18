#ifndef PNANA_CORE_ROPE_H
#define PNANA_CORE_ROPE_H

#include "core/buffer_backend.h"
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 绳索（Rope）数据结构实现
// 特点：
// - 使用平衡二叉树管理文本片段
// - 每个叶子节点存储一小段文本
// - 内部节点存储左右子树的总长度
// - 适合超大文本（> 10MB）
// - 避免大文本拷贝，插入删除效率高
class Rope : public BufferBackend {
  public:
    Rope();
    ~Rope() override = default;

    BufferBackendType getType() const override {
        return BufferBackendType::ROPE;
    }
    const char* getName() const override {
        return "Rope";
    }

    // 文本内容操作
    void insert(size_t pos, const std::string& text) override;
    void remove(size_t pos, size_t length) override;
    std::string getText(size_t pos, size_t length) const override;
    std::string getFullText() const override;
    void clear() override {
        root_ = std::make_shared<RopeNode>();
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

    // 优化操作：平衡树
    void optimize() override;

  private:
    static constexpr size_t LEAF_MAX_SIZE = 512; // 叶子节点最大文本长度

    struct RopeNode {
        std::shared_ptr<RopeNode> left;
        std::shared_ptr<RopeNode> right;
        std::string text;     // 仅叶子节点存储文本
        size_t length;        // 子树总长度
        size_t newline_count; // 子树中换行符数量

        RopeNode() : length(0), newline_count(0) {}
        explicit RopeNode(const std::string& t) : text(t), length(t.size()), newline_count(0) {
            for (char c : text) {
                if (c == '\n')
                    newline_count++;
            }
        }

        bool isLeaf() const {
            return !left && !right;
        }

        void updateLength() {
            length = text.size();
            newline_count = 0;
            for (char c : text) {
                if (c == '\n')
                    newline_count++;
            }
            if (left)
                length += left->length;
            if (right)
                length += right->length;
            if (left)
                newline_count += left->newline_count;
            if (right)
                newline_count += right->newline_count;
        }
    };

    std::shared_ptr<RopeNode> root_;
    mutable size_t line_count_;
    mutable bool lines_dirty_;

    // 辅助函数
    std::shared_ptr<RopeNode> concatenate(std::shared_ptr<RopeNode> left,
                                          std::shared_ptr<RopeNode> right);
    std::pair<std::shared_ptr<RopeNode>, std::shared_ptr<RopeNode>> split(
        std::shared_ptr<RopeNode> node, size_t pos);
    std::shared_ptr<RopeNode> insertAt(std::shared_ptr<RopeNode> node, size_t pos,
                                       const std::string& text);
    std::shared_ptr<RopeNode> removeRange(std::shared_ptr<RopeNode> node, size_t start, size_t end);
    char charAt(std::shared_ptr<RopeNode> node, size_t pos);
    char charAt(std::shared_ptr<RopeNode> node, size_t pos) const;
    std::string substring(std::shared_ptr<RopeNode> node, size_t start, size_t len);
    std::string substringConst(std::shared_ptr<RopeNode> node, size_t start, size_t len) const;
    void collectText(std::shared_ptr<RopeNode> node, std::string& result) const;
    size_t findLineStart(std::shared_ptr<RopeNode> node, size_t line_num) const;

    void recomputeLineCount() const {
        line_count_ = root_ ? (root_->newline_count + 1) : 1;
        lines_dirty_ = false;
    }
};

// Rope 工厂类
class RopeFactory : public BufferBackendFactory {
  public:
    std::unique_ptr<BufferBackend> create() const override {
        return std::make_unique<Rope>();
    }
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_ROPE_H
