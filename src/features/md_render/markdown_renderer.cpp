#include "features/md_render/markdown_renderer.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <regex>
#include <sstream>

namespace pnana {
namespace features {

MarkdownRenderer::MarkdownRenderer(const MarkdownRenderConfig& config) : config_(config) {}

ftxui::Element MarkdownRenderer::render(const std::string& markdown) {
    MarkdownParser parser;
    auto root = parser.parse(markdown);
    return render_element(root);
}

ftxui::Element MarkdownRenderer::render_element(const std::shared_ptr<MarkdownElement>& element,
                                                int indent) {
    if (!element) {
        return ftxui::text("");
    }

    using namespace ftxui;

    switch (element->type) {
        case MarkdownElementType::HEADING:
            return render_heading(element);
        case MarkdownElementType::PARAGRAPH:
            return render_paragraph(element);
        case MarkdownElementType::CODE_BLOCK:
            return render_code_block(element);
        case MarkdownElementType::INLINE_CODE:
            return render_inline_code(element);
        case MarkdownElementType::BOLD:
            return render_bold(element);
        case MarkdownElementType::ITALIC:
            return render_italic(element);
        case MarkdownElementType::LINK:
            return render_link(element);
        case MarkdownElementType::IMAGE:
            return render_image(element);
        case MarkdownElementType::LIST_ITEM:
            return render_list_item(element, indent);
        case MarkdownElementType::BLOCKQUOTE:
            return render_blockquote(element);
        case MarkdownElementType::HORIZONTAL_RULE:
            return render_horizontal_rule();
        case MarkdownElementType::TABLE:
            return render_table(element);
        case MarkdownElementType::TABLE_ROW:
            return render_table_row(element);
        case MarkdownElementType::TABLE_CELL:
            return render_table_cell(element);
        case MarkdownElementType::TEXT:
        default: {
            Elements children_elements;
            for (const auto& child : element->children) {
                children_elements.push_back(render_element(child, indent));
            }
            if (children_elements.empty()) {
                return render_text(element->content);
            }
            return vbox(std::move(children_elements));
        }
    }
}

ftxui::Element MarkdownRenderer::render_heading(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;

    // 渲染标题内容（解析器已经处理了#前缀，这里直接渲染内容）
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    Element heading_content;
    if (content_elements.empty()) {
        heading_content = text(element->content);
    } else {
        heading_content = hbox(std::move(content_elements));
    }

    // 根据标题级别应用不同的样式，模拟 glow 的效果
    switch (element->level) {
        case 1:
            // H1: 居中、反白、加粗，模拟 glow 的醒目效果
            return heading_content | inverted | bold | center;
        case 2:
            // H2: 更大的字体，鲜艳的颜色
            return heading_content | ftxui::color(Color::Cyan) | bold;
        case 3:
            // H3: 中等大小，蓝色
            return heading_content | ftxui::color(Color::Blue) | bold;
        case 4:
            // H4: 标准大小，绿色
            return heading_content | ftxui::color(Color::Green) | bold;
        case 5:
            // H5: 稍小，黄色
            return heading_content | ftxui::color(Color::Yellow) | bold;
        case 6:
        default:
            // H6: 最小，灰色
            return heading_content | ftxui::color(Color::GrayLight) | bold;
    }
}

ftxui::Element MarkdownRenderer::render_paragraph(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    if (content_elements.empty()) {
        // 处理多行文本
        std::istringstream iss(element->content);
        std::string line;
        Elements lines;
        while (std::getline(iss, line)) {
            if (!line.empty()) {
                lines.push_back(wrap_text(line, config_.max_width));
            }
        }
        if (lines.empty()) {
            return text(""); // 空段落
        }
        return vbox(std::move(lines));
    }

    // 如果有子元素，水平排列并限制宽度
    return hbox(std::move(content_elements)) | size(WIDTH, LESS_THAN, config_.max_width);
}

ftxui::Element MarkdownRenderer::render_code_block(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    auto code_color = get_code_color();

    // 分行并渲染每一行，支持宽度约束
    Elements lines;
    auto raw_lines = split_lines(element->content);
    for (auto& ln : raw_lines) {
        lines.push_back(text(ln) | ftxui::color(code_color));
    }

    // 创建顶部和底部的边框线
    std::string border_line(config_.max_width - 4, u8"─"[0]);
    Element top_border = text("┌" + border_line + "┐") | ftxui::color(Color::GrayLight);
    Element bottom_border = text("└" + border_line + "┘") | ftxui::color(Color::GrayLight);

    // 代码块主体，带背景色和内边距
    Element code_content = vbox(std::move(lines)) | bgcolor(Color::GrayDark) |
                           size(WIDTH, LESS_THAN, config_.max_width - 4) |
                           size(HEIGHT, GREATER_THAN, 1);

    // 添加左右边框的竖线
    Elements bordered_lines;
    for (const auto& line : lines) {
        bordered_lines.push_back(
            hbox({text("│") | ftxui::color(Color::GrayLight), text(" "), line, text(" ")}));
    }

    if (bordered_lines.empty()) {
        bordered_lines.push_back(
            hbox({text("│") | ftxui::color(Color::GrayLight), text(" "), text(""), text(" ")}));
    }

    Element code_box = vbox(std::move(bordered_lines)) | bgcolor(Color::GrayDark);

    // 组合整个代码块：顶部边框 + 代码内容 + 底部边框
    return vbox({top_border, code_box, bottom_border});
}

ftxui::Element MarkdownRenderer::render_inline_code(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    auto code_color = get_code_color();
    return text(element->content) | ftxui::color(code_color) | bgcolor(Color::GrayDark) |
           underlined;
}

ftxui::Element MarkdownRenderer::render_bold(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    Element content;
    if (content_elements.empty()) {
        content = text(element->content);
    } else {
        content = hbox(std::move(content_elements));
    }

    return content | get_bold_decorator();
}

ftxui::Element MarkdownRenderer::render_italic(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    Element content;
    if (content_elements.empty()) {
        content = text(element->content);
    } else {
        content = hbox(std::move(content_elements));
    }

    return content | get_italic_decorator();
}

ftxui::Element MarkdownRenderer::render_link(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;

    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    Element link_text;
    if (content_elements.empty()) {
        link_text = text(element->content);
    } else {
        link_text = hbox(std::move(content_elements));
    }

    // 显示链接文本，glow 风格：只显示链接文本，不显示URL
    std::string display_text = element->content;
    if (display_text.empty() && !content_elements.empty()) {
        // 如果没有显示文本，使用URL作为显示文本
        display_text = element->url;
    }

    // 如果没有内容，尝试从子元素构建
    if (display_text.empty() && content_elements.empty()) {
        display_text = "[Link]"; // 兜底显示
    }

    Element result;
    if (!content_elements.empty()) {
        result = hbox(std::move(content_elements));
    } else {
        result = text(display_text);
    }

    // 应用链接样式：亮蓝色 + 下划线
    return result | ftxui::color(Color::BlueLight) | underlined;
}

ftxui::Element MarkdownRenderer::render_image(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    // 对于终端环境，显示图片的替代文本或标题
    std::string display_text = element->title.empty() ? element->content : element->title;
    if (display_text.empty()) {
        display_text = "[Image]";
    }

    return text(display_text) | dim;
}

ftxui::Element MarkdownRenderer::render_list_item(const std::shared_ptr<MarkdownElement>& element,
                                                  int indent) {
    using namespace ftxui;

    std::string indent_str(indent * 2, ' ');
    std::string marker = "• "; // 使用 bullet point

    Elements content_elements;

    // 如果有子元素，垂直排列
    if (!element->children.empty()) {
        Elements item_lines;
        item_lines.push_back(text(indent_str + marker));

        for (const auto& child : element->children) {
            auto rendered_child = render_element(child, indent + 1);
            item_lines.push_back(hbox({text(indent_str + "  "), rendered_child}));
        }

        return vbox(std::move(item_lines));
    } else {
        // 简单列表项：标记 + 内容
        content_elements.push_back(text(indent_str + marker));
        content_elements.push_back(text(element->content));
        return hbox(std::move(content_elements));
    }
}

ftxui::Element MarkdownRenderer::render_blockquote(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    auto blockquote_color = get_blockquote_color();
    Elements content_elements;

    for (const auto& child : element->children) {
        auto child_element = render_element(child, 1);
        content_elements.push_back(text("│ ") | ftxui::color(blockquote_color));
        content_elements.push_back(child_element);
    }

    if (content_elements.empty()) {
        return text("│ " + element->content) | ftxui::color(blockquote_color);
    }

    return vbox(std::move(content_elements));
}

ftxui::Element MarkdownRenderer::render_horizontal_rule() {
    using namespace ftxui;

    std::string rule(config_.max_width, '-');
    return text(rule) | dim;
}

ftxui::Element MarkdownRenderer::render_text(const std::string& text) {
    using namespace ftxui;
    return wrap_text(text, config_.max_width);
}

ftxui::Element MarkdownRenderer::render_table(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements row_elements;
    // 计算列数与列宽（扫描整个表格）
    table_col_widths_.clear();
    table_num_cols_ = 0;

    // 先统计最大列数
    for (const auto& child : element->children) {
        if (child->type != MarkdownElementType::TABLE_ROW)
            continue;
        int cols = 0;
        for (const auto& cell : child->children) {
            if (cell->type == MarkdownElementType::TABLE_CELL)
                cols++;
        }
        table_num_cols_ = std::max(table_num_cols_, cols);
    }
    if (table_num_cols_ <= 0)
        return vbox({});

    table_col_widths_.assign(table_num_cols_, 0);

    // 计算每列最大宽度（基于 cell->content 长度）
    for (const auto& child : element->children) {
        if (child->type != MarkdownElementType::TABLE_ROW)
            continue;
        int col = 0;
        for (const auto& cell : child->children) {
            if (cell->type != MarkdownElementType::TABLE_CELL)
                continue;
            int len = static_cast<int>(cell->content.length());
            if (col < table_num_cols_)
                table_col_widths_[col] = std::max(table_col_widths_[col], len);
            col++;
        }
    }

    // 渲染行时使用计算好的列宽
    for (const auto& child : element->children) {
        if (child->type == MarkdownElementType::TABLE_ROW) {
            // 渲染行
            row_elements.push_back(render_table_row(child));

            // 如果该行是表头（任何单元格 is_header == true），则在其后插入分隔线
            bool has_header = false;
            for (const auto& cell : child->children) {
                if (cell->type == MarkdownElementType::TABLE_CELL && cell->is_header) {
                    has_header = true;
                    break;
                }
            }
            if (has_header) {
                // 生成分隔线字符串，使用更好的Unicode边框字符
                std::string sep = "├";
                for (int i = 0; i < table_num_cols_; ++i) {
                    if (i > 0) {
                        sep += "┼";
                    }
                    int w = table_col_widths_[i] + 2; // 加上左右 padding
                    for (int k = 0; k < w; ++k) {
                        sep += "─";
                    }
                }
                sep += "┤";
                row_elements.push_back(text(sep) | color(Color::GrayLight));
            }
        }
    }

    return vbox(std::move(row_elements));
}

ftxui::Element MarkdownRenderer::render_table_row(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements cell_elements;
    int col = 0;

    // 添加行开始的边框
    cell_elements.push_back(text("│") | color(Color::GrayLight));

    for (const auto& child : element->children) {
        if (child->type != MarkdownElementType::TABLE_CELL)
            continue;

        std::string cell_text = child->content;
        // 填充至列宽，并添加左右 padding
        if (col < table_num_cols_) {
            int target = table_col_widths_[col];
            if ((int)cell_text.length() < target) {
                cell_text += std::string(target - (int)cell_text.length(), ' ');
            }
        }
        // 左右空格填充以提高可读性
        std::string padded = " " + cell_text + " ";
        cell_elements.push_back(text(padded));
        // 添加列分隔（灰色）
        cell_elements.push_back(text("│") | color(Color::GrayLight));
        col++;
    }

    return hbox(std::move(cell_elements));
}

ftxui::Element MarkdownRenderer::render_table_cell(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    Element content;
    if (content_elements.empty()) {
        content = text(element->content);
    } else {
        content = hbox(std::move(content_elements));
    }

    if (element->is_header) {
        return content | bold;
    }

    return content;
}

ftxui::Element MarkdownRenderer::wrap_text(const std::string& text, int max_width) {
    using namespace ftxui;

    if (max_width <= 0 || text.length() <= size_t(max_width)) {
        return ftxui::text(text);
    }

    Elements lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.length() <= size_t(max_width)) {
            lines.push_back(ftxui::text(line));
        } else {
            // 简单地截断长行（实际应用中可能需要更复杂的换行逻辑）
            lines.push_back(ftxui::text(line.substr(0, max_width)));
        }
    }

    return vbox(std::move(lines));
}

std::string MarkdownRenderer::indent_text(const std::string& text, int indent) {
    std::string indent_str(indent * 2, ' ');
    std::string result;
    std::istringstream iss(text);
    std::string line;
    bool first = true;

    while (std::getline(iss, line)) {
        if (!first) {
            result += "\n";
        }
        result += indent_str + line;
        first = false;
    }

    return result;
}

std::vector<std::string> MarkdownRenderer::split_lines(const std::string& text) {
    std::vector<std::string> lines;
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    return lines;
}

ftxui::Color MarkdownRenderer::get_heading_color(int level) {
    if (!config_.use_color) {
        return ftxui::Color::Default;
    }

    // Glow-inspired color scheme for dark theme
    switch (level) {
        case 1:
            return ftxui::Color::White; // H1: 白色（与反白背景配合）
        case 2:
            return ftxui::Color::Cyan; // H2: 青色
        case 3:
            return ftxui::Color::Blue; // H3: 蓝色
        case 4:
            return ftxui::Color::Green; // H4: 绿色
        case 5:
            return ftxui::Color::Yellow; // H5: 黄色
        case 6:
            return ftxui::Color::Magenta; // H6: 品红
        default:
            return ftxui::Color::GrayLight;
    }
}

ftxui::Color MarkdownRenderer::get_code_color() {
    if (config_.use_color) {
        return ftxui::Color::Green;
    } else {
        return ftxui::Color(); // Default color
    }
}

ftxui::Color MarkdownRenderer::get_link_color() {
    if (config_.use_color) {
        return ftxui::Color::BlueLight;
    } else {
        return ftxui::Color(); // Default color
    }
}

ftxui::Color MarkdownRenderer::get_blockquote_color() {
    if (config_.use_color) {
        return ftxui::Color::GrayLight;
    } else {
        return ftxui::Color(); // Default color
    }
}

ftxui::Decorator MarkdownRenderer::get_bold_decorator() {
    return ftxui::bold;
}

ftxui::Decorator MarkdownRenderer::get_italic_decorator() {
    return ftxui::dim; // 终端中斜体可能显示为暗淡
}

} // namespace features
} // namespace pnana
