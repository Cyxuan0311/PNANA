#ifndef PNANA_CORE_BUFFER_BACKEND_H
#define PNANA_CORE_BUFFER_BACKEND_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace pnana {
namespace core {

// 缓冲区后端类型枚举
enum class BufferBackendType {
    GAP_BUFFER,         // 间隙缓冲区：适合小文本、频繁局部编辑
    SQRT_DECOMPOSITION, // 平方根分解：适合中等规模文本
    ROPE,               // 绳索结构：适合超大文本
    PIECE_TABLE         // 片段表：通用黄金标准
};

// 缓冲区后端抽象基类
class BufferBackend {
  public:
    virtual ~BufferBackend() = default;

    // 获取后端类型
    virtual BufferBackendType getType() const = 0;

    // 获取后端名称（用于调试）
    virtual const char* getName() const = 0;

    // 文本内容操作
    virtual void insert(size_t pos, const std::string& text) = 0;
    virtual void remove(size_t pos, size_t length) = 0;
    virtual std::string getText(size_t pos, size_t length) const = 0;
    virtual std::string getFullText() const = 0;
    virtual void clear() = 0; // 清空缓冲区

    // 行操作
    virtual void insertLine(size_t line_num, const std::string& content) = 0;
    virtual void removeLine(size_t line_num) = 0;
    virtual std::string getLine(size_t line_num) const = 0;
    virtual size_t lineCount() const = 0;

    // 字符操作
    virtual void insertChar(size_t pos, char ch) = 0;
    virtual void removeChar(size_t pos) = 0;
    virtual char getChar(size_t pos) const = 0;

    virtual void replace(size_t pos, size_t length, const std::string& text) {
        remove(pos, length);
        insert(pos, text);
    }
    virtual void swapLine(size_t line_a, size_t line_b) {
        if (line_a == line_b)
            return;
        std::string content_a = getLine(line_a);
        std::string content_b = getLine(line_b);
        removeLine(line_a);
        insertLine(line_a, content_b);
        removeLine(line_b);
        insertLine(line_b, content_a);
    }

    // 查询操作
    virtual size_t length() const = 0;
    virtual size_t lineLength(size_t line_num) const = 0;
    virtual size_t positionToLineCol(size_t pos) const = 0;
    virtual size_t lineColToPosition(size_t line, size_t col) const = 0;

    static constexpr size_t LINE_COL_SHIFT = 32;
    static size_t encodeLineCol(size_t line, size_t col) {
        return (line << LINE_COL_SHIFT) | col;
    }
    static size_t decodeLine(size_t encoded) {
        return encoded >> LINE_COL_SHIFT;
    }
    static size_t decodeCol(size_t encoded) {
        return encoded & ((size_t(1) << LINE_COL_SHIFT) - 1);
    }

    // 文件加载和保存
    virtual bool loadFromFile(const std::string& filepath) = 0;
    virtual bool saveToFile(const std::string& filepath) const = 0;

    // 内存使用统计
    virtual size_t getMemoryUsage() const = 0;

    // 优化操作（可选）
    virtual void optimize() = 0;
};

// 后端工厂接口
class BufferBackendFactory {
  public:
    virtual ~BufferBackendFactory() = default;
    virtual std::unique_ptr<BufferBackend> create() const = 0;
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_BUFFER_BACKEND_H
