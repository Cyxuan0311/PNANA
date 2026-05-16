#ifndef PNANA_CORE_GAP_BUFFER_H
#define PNANA_CORE_GAP_BUFFER_H

#include "core/buffer_backend.h"
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 间隙缓冲区实现
// 特点：
// - 在缓冲区中维护一个"间隙"，所有插入删除都在间隙处进行
// - 移动光标时需要移动间隙位置
// - 适合小文本（< 1MB）和频繁局部编辑的场景
// - 实现简单，单字符操作非常快
class GapBuffer : public BufferBackend {
  public:
    explicit GapBuffer(size_t initial_gap_size = 1024);
    ~GapBuffer() override = default;

    BufferBackendType getType() const override {
        return BufferBackendType::GAP_BUFFER;
    }
    const char* getName() const override {
        return "GapBuffer";
    }

    // 文本内容操作
    void insert(size_t pos, const std::string& text) override;
    void remove(size_t pos, size_t length) override;
    std::string getText(size_t pos, size_t length) const override;
    std::string getFullText() const override;
    void clear() override {
        gap_start_ = 0;
        gap_end_ = gap_size_;
        lines_dirty_ = true;
        line_count_ = 1;
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

    // 优化操作：当间隙过大时缩小
    void optimize() override;

  private:
    std::vector<char> buffer_;  // 主缓冲区
    size_t gap_start_;          // 间隙起始位置
    size_t gap_end_;            // 间隙结束位置
    size_t gap_size_;           // 间隙大小
    mutable size_t line_count_; // 缓存的行数
    mutable bool lines_dirty_;  // 行缓存是否失效

    // 移动间隙到指定位置
    void moveGap(size_t pos);

    // 扩大缓冲区
    void grow(size_t min_size = 0);

    // 重新计算行数
    void recomputeLineCount() const;

    // 获取指定行的起始位置
    size_t getLineStart(size_t line_num) const;
};

// GapBuffer 工厂类
class GapBufferFactory : public BufferBackendFactory {
  public:
    std::unique_ptr<BufferBackend> create() const override {
        return std::make_unique<GapBuffer>();
    }
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_GAP_BUFFER_H
