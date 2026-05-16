#ifndef PNANA_CORE_SQRT_DECOMPOSITION_H
#define PNANA_CORE_SQRT_DECOMPOSITION_H

#include "core/buffer_backend.h"
#include <cmath>
#include <fstream>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 平方根分解缓冲区实现
// 特点：
// - 将文本分成大小为 sqrt(N) 的块
// - 平衡查询和更新效率
// - 适合中等规模文本（1MB - 10MB）
// - 块内使用简单数组，块间使用链表或数组管理
class SqrtDecomposition : public BufferBackend {
  public:
    SqrtDecomposition();
    ~SqrtDecomposition() override = default;

    BufferBackendType getType() const override {
        return BufferBackendType::SQRT_DECOMPOSITION;
    }
    const char* getName() const override {
        return "SqrtDecomposition";
    }

    // 文本内容操作
    void insert(size_t pos, const std::string& text) override;
    void remove(size_t pos, size_t length) override;
    std::string getText(size_t pos, size_t length) const override;
    std::string getFullText() const override;
    void clear() override {
        blocks_.clear();
        blocks_.emplace_back();
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

    // 优化操作：重新平衡块
    void optimize() override;

  private:
    struct Block {
        std::vector<char> data;
        size_t total_length; // 从开始到当前块的累计长度

        Block() : total_length(0) {}
        explicit Block(size_t size) : data(size), total_length(0) {}
    };

    std::vector<Block> blocks_;
    size_t total_length_;
    size_t block_size_;
    mutable size_t line_count_;
    mutable bool lines_dirty_;

    // 计算最优块大小
    void recalculateBlockSize();

    // 找到包含位置 pos 的块
    size_t findBlock(size_t pos) const;

    // 分割块以保持平衡
    void splitBlock(size_t block_idx);

    // 合并相邻块
    void mergeBlocks();

    // 从指定块开始刷新累计长度
    void refreshTotalsFrom(size_t start_idx);

    // 获取块的起始逻辑偏移
    size_t blockStartOffset(size_t block_idx) const;

    // 重新计算行数
    void recomputeLineCount() const;

    // 获取指定行的起始位置
    size_t getLineStart(size_t line_num) const;
};

// SqrtDecomposition 工厂类
class SqrtDecompositionFactory : public BufferBackendFactory {
  public:
    std::unique_ptr<BufferBackend> create() const override {
        return std::make_unique<SqrtDecomposition>();
    }
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_SQRT_DECOMPOSITION_H
