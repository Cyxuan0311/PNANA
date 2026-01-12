#include "features/md_render/markdown_parser.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstring>

namespace pnana {
namespace features {

MarkdownParser::MarkdownParser() {
    // 初始化解析上下文
    context_.root = std::make_shared<MarkdownElement>(MarkdownElementType::TEXT);
    context_.element_stack.push(context_.root);
}

std::shared_ptr<MarkdownElement> MarkdownParser::parse(const std::string& markdown) {
    // 重置上下文
    context_.root = std::make_shared<MarkdownElement>(MarkdownElementType::TEXT);
    context_.element_stack = std::stack<std::shared_ptr<MarkdownElement>>();
    context_.element_stack.push(context_.root);
    context_.current_text.clear();
    context_.in_code_block = false;
    context_.list_level = 0;
    context_.in_table = false;

    // 设置MD4C解析器
    MD_PARSER parser = {
        0,                 // abi_version
        MD_DIALECT_GITHUB, // flags - 使用GitHub风格的markdown
        enter_block_callback,
        leave_block_callback,
        enter_span_callback,
        leave_span_callback,
        text_callback,
        nullptr, // debug_log
        nullptr  // syntax
    };

    // 解析markdown
    md_parse(markdown.c_str(), markdown.size(), &parser, this);

    return context_.root;
}

int MarkdownParser::enter_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata) {
    auto* parser = static_cast<MarkdownParser*>(userdata);
    parser->handle_enter_block(type, detail);
    return 0;
}

int MarkdownParser::leave_block_callback(MD_BLOCKTYPE type, void* detail, void* userdata) {
    auto* parser = static_cast<MarkdownParser*>(userdata);
    parser->handle_leave_block(type, detail);
    return 0;
}

int MarkdownParser::enter_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
    auto* parser = static_cast<MarkdownParser*>(userdata);
    parser->handle_enter_span(type, detail);
    return 0;
}

int MarkdownParser::leave_span_callback(MD_SPANTYPE type, void* detail, void* userdata) {
    auto* parser = static_cast<MarkdownParser*>(userdata);
    parser->handle_leave_span(type, detail);
    return 0;
}

int MarkdownParser::text_callback(MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size,
                                  void* userdata) {
    auto* parser = static_cast<MarkdownParser*>(userdata);
    std::string text_str(text, size);
    // 诊断日志：记录部分文本回调内容，帮助确认 md4c 是否正确传递文本
    if (size > 0) {
        std::string snippet = text_str.substr(0, std::min<size_t>(text_str.size(), 200));
        LOG("[DEBUG MD4C TEXT] type=" + std::to_string(static_cast<int>(type)) +
            " size=" + std::to_string(size) + " snippet=\"" + snippet + "\"");
    } else {
        LOG("[DEBUG MD4C TEXT] type=" + std::to_string(static_cast<int>(type)) + " size=0");
    }
    parser->handle_text(type, text_str);
    return 0;
}

void MarkdownParser::handle_enter_block(MD_BLOCKTYPE type, void* detail) {
    MarkdownElementType elem_type;
    std::string content;

    switch (type) {
        case MD_BLOCK_H:
            elem_type = MarkdownElementType::HEADING;
            if (detail) {
                auto* h_detail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
                // 设置标题级别（在leave时处理）
                // 记录标题级别用于后续处理
                context_.heading_level = h_detail->level;
            }
            break;

        case MD_BLOCK_P:
            elem_type = MarkdownElementType::PARAGRAPH;
            break;

        case MD_BLOCK_CODE:
            elem_type = MarkdownElementType::CODE_BLOCK;
            context_.in_code_block = true;
            break;

        case MD_BLOCK_QUOTE:
            elem_type = MarkdownElementType::BLOCKQUOTE;
            break;

        case MD_BLOCK_UL:
        case MD_BLOCK_OL:
            // 列表容器，不创建单独元素
            return;

        case MD_BLOCK_LI:
            elem_type = MarkdownElementType::LIST_ITEM;
            if (detail) {
                auto* li_detail = static_cast<MD_BLOCK_LI_DETAIL*>(detail);
                (void)li_detail; // 可以根据li_detail->is_task和li_detail->task_mark来处理任务列表
            }
            break;

        case MD_BLOCK_HR:
            elem_type = MarkdownElementType::HORIZONTAL_RULE;
            break;

        case MD_BLOCK_TABLE:
            elem_type = MarkdownElementType::TABLE;
            context_.in_table = true;
            break;

        case MD_BLOCK_TR:
            elem_type = MarkdownElementType::TABLE_ROW;
            break;

        case MD_BLOCK_TH:
        case MD_BLOCK_TD:
            elem_type = MarkdownElementType::TABLE_CELL;
            break;

        default:
            return; // 不处理其他块类型
    }

    auto element = create_element(elem_type, content);
    add_to_current_parent(element);
    context_.element_stack.push(element);
}

void MarkdownParser::handle_leave_block(MD_BLOCKTYPE type, void* detail) {
    switch (type) {
        case MD_BLOCK_H:
            if (detail) {
                auto* h_detail = static_cast<MD_BLOCK_H_DETAIL*>(detail);
                if (!context_.element_stack.empty()) {
                    auto current = context_.element_stack.top();
                    current->level = h_detail->level;
                }
            }
            break;

        case MD_BLOCK_CODE:
            context_.in_code_block = false;
            break;

        case MD_BLOCK_TABLE:
            context_.in_table = false;
            break;

        default:
            break;
    }

    if (!context_.element_stack.empty()) {
        context_.element_stack.pop();
    }
}

void MarkdownParser::handle_enter_span(MD_SPANTYPE type, void* detail) {
    MarkdownElementType elem_type;
    std::string content;

    switch (type) {
        case MD_SPAN_EM:
            elem_type = MarkdownElementType::ITALIC;
            break;

        case MD_SPAN_STRONG:
            elem_type = MarkdownElementType::BOLD;
            break;

        case MD_SPAN_CODE:
            elem_type = MarkdownElementType::INLINE_CODE;
            break;

        case MD_SPAN_A:
            elem_type = MarkdownElementType::LINK;
            if (detail) {
                auto* a_detail = static_cast<MD_SPAN_A_DETAIL*>(detail);
                (void)a_detail; // URL和title会在leave时处理
            }
            break;

        case MD_SPAN_IMG:
            elem_type = MarkdownElementType::IMAGE;
            if (detail) {
                auto* img_detail = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
                (void)img_detail; // URL和title会在leave时处理
            }
            break;

        default:
            return; // 不处理其他span类型
    }

    auto element = create_element(elem_type, content);
    add_to_current_parent(element);
    context_.element_stack.push(element);
}

void MarkdownParser::handle_leave_span(MD_SPANTYPE type, void* detail) {
    if (context_.element_stack.empty()) {
        return;
    }

    auto current = context_.element_stack.top();

    switch (type) {
        case MD_SPAN_A:
            if (detail) {
                auto* a_detail = static_cast<MD_SPAN_A_DETAIL*>(detail);
                current->url = std::string(a_detail->href.text, a_detail->href.size);
                current->title = std::string(a_detail->title.text, a_detail->title.size);
            }
            break;

        case MD_SPAN_IMG:
            if (detail) {
                auto* img_detail = static_cast<MD_SPAN_IMG_DETAIL*>(detail);
                current->url = std::string(img_detail->src.text, img_detail->src.size);
                current->title = std::string(img_detail->title.text, img_detail->title.size);
            }
            break;

        default:
            break;
    }

    context_.element_stack.pop();
}

void MarkdownParser::handle_text(MD_TEXTTYPE type, const std::string& text) {
    switch (type) {
        case MD_TEXT_NORMAL:
        case MD_TEXT_CODE:
            // 添加文本内容到当前元素
            if (!context_.element_stack.empty()) {
                auto current = context_.element_stack.top();
                // 特殊处理标题：去除开头的 # 符号和空格
                if (current->type == MarkdownElementType::HEADING) {
                    std::string processed_text = text;
                    // 去除开头的 # 符号和空格
                    size_t start = processed_text.find_first_not_of("# ");
                    if (start != std::string::npos) {
                        processed_text = processed_text.substr(start);
                    } else {
                        processed_text.clear();
                    }
                    current->content += processed_text;
                } else {
                    current->content += text;
                }
            }
            break;

        case MD_TEXT_BR:
        case MD_TEXT_SOFTBR:
            // 处理换行
            if (!context_.element_stack.empty()) {
                auto current = context_.element_stack.top();
                current->content += "\n";
            }
            break;

        default:
            break;
    }
}

std::shared_ptr<MarkdownElement> MarkdownParser::create_element(MarkdownElementType type,
                                                                const std::string& content) {
    return std::make_shared<MarkdownElement>(type, content);
}

void MarkdownParser::add_to_current_parent(std::shared_ptr<MarkdownElement> element) {
    auto parent = get_current_parent();
    if (parent) {
        parent->children.push_back(element);
    }
}

std::shared_ptr<MarkdownElement> MarkdownParser::get_current_parent() {
    if (context_.element_stack.empty()) {
        return nullptr;
    }
    return context_.element_stack.top();
}

} // namespace features
} // namespace pnana
