#ifndef PNANA_FEATURES_MARKDOWN_PARSER_H
#define PNANA_FEATURES_MARKDOWN_PARSER_H

#include <memory>
#include <stack>
#include <string>
#include <vector>

extern "C" {
#include "../../../../third-party/md4c/md4c.h"
}

namespace pnana {
namespace features {

// Markdown元素类型
enum class MarkdownElementType {
    TEXT,
    HEADING,
    PARAGRAPH,
    CODE_BLOCK,
    INLINE_CODE,
    BOLD,
    ITALIC,
    LINK,
    IMAGE,
    LIST_ITEM,
    BLOCKQUOTE,
    HORIZONTAL_RULE,
    TABLE,
    TABLE_ROW,
    TABLE_CELL
};

// Markdown元素
struct MarkdownElement {
    MarkdownElementType type;
    std::string content;
    int level = 0;          // 用于标题级别
    std::string url;        // 用于链接和图片
    std::string title;      // 用于链接和图片
    std::string lang;       // 用于代码块的语言信息
    bool is_header = false; // 用于表格单元格
    std::vector<std::shared_ptr<MarkdownElement>> children;

    MarkdownElement(MarkdownElementType t, const std::string& c = "") : type(t), content(c) {}
};

// Markdown解析器上下文
struct MarkdownParserContext {
    std::shared_ptr<MarkdownElement> root;
    std::stack<std::shared_ptr<MarkdownElement>> element_stack;
    std::string current_text;
    bool in_code_block = false;
    int list_level = 0;
    bool in_table = false;
    int heading_level = 0; // 新增：当前标题级别
};

// Markdown解析器
class MarkdownParser {
  public:
    MarkdownParser();

    // 解析markdown文本，返回解析后的元素树
    std::shared_ptr<MarkdownElement> parse(const std::string& markdown);

  private:
    MarkdownParserContext context_;

    // MD4C回调函数
    static int enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata);
    static int enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata);
    static int leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata);
    static int text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata);

    // 事件处理函数
    void handle_enter_block(MD_BLOCKTYPE type, void* detail);
    void handle_leave_block(MD_BLOCKTYPE type, void* detail);
    void handle_enter_span(MD_SPANTYPE type, void* detail);
    void handle_leave_span(MD_SPANTYPE type, void* detail);
    void handle_text(MD_TEXTTYPE type, const std::string& text);

    // 辅助方法
    std::shared_ptr<MarkdownElement> create_element(MarkdownElementType type,
                                                    const std::string& content = "");
    void add_to_current_parent(std::shared_ptr<MarkdownElement> element);
    std::shared_ptr<MarkdownElement> get_current_parent();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_MARKDOWN_PARSER_H
