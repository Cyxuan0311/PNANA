#include "features/md_render/markdown_renderer.h"
#include "ui/theme.h"
#include <algorithm>
#include <ftxui/dom/elements.hpp>
#include <regex>
#include <sstream>

namespace pnana {
namespace features {

// Helper: wrap a single paragraph into lines by words, respecting max_width.
static std::vector<std::string> wrap_into_lines(const std::string& text, int max_width) {
    std::vector<std::string> out_lines;
    if (max_width <= 0) {
        out_lines.push_back(text);
        return out_lines;
    }

    std::istringstream iss(text);
    std::string paragraph_line;
    // Split original text by newline and wrap each separately
    while (std::getline(iss, paragraph_line)) {
        if (paragraph_line.empty()) {
            out_lines.emplace_back("");
            continue;
        }
        std::istringstream wss(paragraph_line);
        std::string word;
        std::string current;
        while (wss >> word) {
            if (current.empty()) {
                current = word;
            } else {
                if ((int)current.length() + 1 + (int)word.length() <= max_width) {
                    current += " " + word;
                } else {
                    out_lines.push_back(current);
                    current = word;
                }
            }
        }
        if (!current.empty()) {
            out_lines.push_back(current);
        }
    }

    return out_lines;
}

// 前向声明：在构造函数中调用，定义位于文件较下方
namespace {
pnana::ui::ThemeColors loadThemeColors(const std::string& theme_name);
}

MarkdownRenderer::MarkdownRenderer(const MarkdownRenderConfig& config) : config_(config) {
    // 加载并缓存当前主题颜色
    try {
        theme_colors_ = loadThemeColors(config_.theme);
    } catch (...) {
        // 失败时使用默认主题颜色
        theme_colors_ = pnana::ui::Theme::Monokai();
    }
}

// 初始化主题颜色缓存
namespace {
pnana::ui::ThemeColors loadThemeColors(const std::string& theme_name) {
    pnana::ui::Theme t;
    t.setTheme(theme_name);
    return t.getColors();
}
} // namespace

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

    // Prepare inline elements: if there are child spans, render each and apply
    // fg/bg/bold to each child so background only covers text width.
    std::string text_content = element->content;
    // Trim trailing newlines
    while (!text_content.empty() && (text_content.back() == '\n' || text_content.back() == '\r'))
        text_content.pop_back();

    auto fg = get_heading_fg_color(element->level);
    auto bg = get_heading_bg_color(element->level);

    Elements inline_elems;
    if (!element->children.empty()) {
        // If there's leading plain content, add it first
        if (!text_content.empty())
            inline_elems.push_back(text(text_content) | get_bold_decorator() | ftxui::color(fg) |
                                   ftxui::bgcolor(bg));

        for (const auto& child : element->children) {
            // Render child then apply fg/bg and bold to ensure bg only covers child width
            auto child_el = render_element(child);
            inline_elems.push_back(child_el | get_bold_decorator() | ftxui::color(fg) |
                                   ftxui::bgcolor(bg));
        }
    } else {
        // No child spans: simple text element
        inline_elems.push_back(text(text_content) | get_bold_decorator() | ftxui::color(fg) |
                               ftxui::bgcolor(bg));
    }

    auto row = hbox(std::move(inline_elems));
    if (element->level == 1 && config_.max_width > 0) {
        return ftxui::hcenter(row);
    }
    return row;
}

ftxui::Element MarkdownRenderer::render_paragraph(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements content_elements;
    for (const auto& child : element->children) {
        content_elements.push_back(render_element(child));
    }

    if (content_elements.empty()) {
        // 处理多行文本：按源换行拆分并按单词换行，保留空行，使用单个 text 元素
        auto wrapped_lines = wrap_into_lines(element->content, config_.max_width);
        Elements lines;
        for (const auto& ln : wrapped_lines) {
            lines.push_back(text(ln));
        }
        return vbox(std::move(lines));
    }

    // 如果有子元素，水平排列它们以保留内联样式，必要时回退到文本内容
    if (!content_elements.empty()) {
        return hbox(std::move(content_elements));
    }
    return wrap_text(element->content, config_.max_width);
}

ftxui::Element MarkdownRenderer::render_code_block(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    // Render code block with dark background
    Elements rendered;
    int total_width = std::max(0, config_.max_width);
    std::string lang = element->lang;

    // Header: language label with dark background
    std::string header;
    if (!lang.empty())
        header = "-----" + lang + "-----";
    else
        header = std::string(5, '-') + "code" + std::string(5, '-');
    if ((int)header.length() < total_width)
        header += std::string(total_width - (int)header.length(), '-');
    if ((int)header.length() > total_width)
        header = header.substr(0, total_width);

    Element header_el = text(header) | ftxui::color(ftxui::Color::GrayLight) |
                        get_bold_decorator() | ftxui::bgcolor(get_code_bg_color());
    rendered.push_back(header_el);

    // Body: code lines with dark background
    Elements body_lines;
    auto raw_lines = split_lines(element->content);
    // Wrap each source line by words to avoid mid-word truncation
    for (auto& ln : raw_lines) {
        auto wrapped = wrap_into_lines(ln, total_width);
        if (wrapped.empty()) {
            body_lines.push_back(text("") | ftxui::color(get_code_color()) |
                                 ftxui::bgcolor(get_code_bg_color()));
        } else {
            for (const auto& wln : wrapped) {
                body_lines.push_back(text(wln) | ftxui::color(get_code_color()) |
                                     ftxui::bgcolor(get_code_bg_color()));
            }
        }
    }
    if (body_lines.empty())
        body_lines.push_back(text("") | ftxui::bgcolor(get_code_bg_color()));
    rendered.push_back(vbox(std::move(body_lines)));

    // Footer: short separator with dark background
    std::string footer = std::string(10, '-');
    if ((int)footer.length() < total_width) {
        // center footer
        int pad = (total_width - (int)footer.length()) / 2;
        footer = std::string(pad, ' ') + footer;
    }
    Element footer_el =
        text(footer) | ftxui::color(ftxui::Color::GrayLight) | ftxui::bgcolor(get_code_bg_color());
    rendered.push_back(footer_el);

    return vbox(std::move(rendered));
}

ftxui::Element MarkdownRenderer::render_inline_code(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    // Inline code: green text on dark background, underlined
    std::string content = element->content;
    Element el = text(content) | ftxui::color(get_code_color()) | ftxui::underlined;
    return el;
}

ftxui::Element MarkdownRenderer::render_bold(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    return text(element->content) | get_bold_decorator();
}

ftxui::Element MarkdownRenderer::render_italic(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    return text(element->content) | get_italic_decorator();
}

ftxui::Element MarkdownRenderer::render_link(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    std::string display_text = element->content;
    if (display_text.empty())
        display_text = element->url;
    if (display_text.empty())
        display_text = "[Link]";
    Decorator d = ftxui::color(get_link_color()) | ftxui::underlined;
    return text(display_text) | d;
}

ftxui::Element MarkdownRenderer::render_image(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    std::string display_text = element->title.empty() ? element->content : element->title;
    if (display_text.empty())
        display_text = "[Image]";
    return text(display_text) | ftxui::dim;
}

ftxui::Element MarkdownRenderer::render_list_item(const std::shared_ptr<MarkdownElement>& element,
                                                  int indent) {
    using namespace ftxui;
    std::string indent_str(indent * 2, ' ');
    std::string marker = "• ";
    int available_width =
        config_.max_width - static_cast<int>(indent_str.size()) - static_cast<int>(marker.size());
    if (available_width <= 0)
        available_width = config_.max_width;
    // Prefer rendering children if present (inline formatting). Otherwise wrap textual content.
    if (!element->children.empty()) {
        Elements elems;
        elems.push_back(text(indent_str + marker));
        for (const auto& child : element->children) {
            elems.push_back(render_element(child, indent + 1));
        }
        return hbox(std::move(elems));
    }
    auto wrapped = wrap_into_lines(element->content, available_width);
    std::ostringstream oss;
    if (!wrapped.empty()) {
        oss << indent_str << marker << wrapped[0];
        for (size_t i = 1; i < wrapped.size(); ++i) {
            oss << "\n" << indent_str << std::string(marker.size(), ' ') << wrapped[i];
        }
    } else {
        oss << indent_str << marker;
    }
    return text(oss.str());
}

ftxui::Element MarkdownRenderer::render_blockquote(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    auto lines = wrap_into_lines(element->content, config_.max_width);
    Elements lines_el;
    for (const auto& ln : lines) {
        lines_el.push_back(text(std::string("│ ") + ln));
    }
    return vbox(std::move(lines_el)) | ftxui::color(get_blockquote_color());
}

ftxui::Element MarkdownRenderer::render_horizontal_rule() {
    using namespace ftxui;
    std::string rule;
    rule.reserve(config_.max_width * 3); // Reserve space for UTF-8 encoded characters
    for (size_t i = 0; i < static_cast<size_t>(config_.max_width); ++i) {
        rule += "─";
    }
    if (config_.use_color)
        rule = std::string("\033[90m") + rule + "\033[0m";
    return text(rule);
}

ftxui::Element MarkdownRenderer::render_text(const std::string& text) {
    using namespace ftxui;
    return wrap_text(text, config_.max_width);
}

ftxui::Element MarkdownRenderer::render_table(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    // Build a textual table using Unicode box drawing, return as single text element.
    table_col_widths_.clear();
    table_num_cols_ = 0;
    // count cols
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
        return text("");
    table_col_widths_.assign(table_num_cols_, 0);
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

    std::ostringstream oss;
    bool header_emitted = false;
    for (const auto& child : element->children) {
        if (child->type != MarkdownElementType::TABLE_ROW)
            continue;
        // render row
        // build line
        oss << "│";
        int col = 0;
        for (const auto& cell : child->children) {
            if (cell->type != MarkdownElementType::TABLE_CELL)
                continue;
            std::string cell_text = cell->content;
            int target = table_col_widths_[col];
            if ((int)cell_text.length() < target)
                cell_text += std::string(target - (int)cell_text.length(), ' ');
            oss << " " << cell_text << " │";
            col++;
        }
        oss << "\n";
        // check header separator
        bool has_header = false;
        for (const auto& cell : child->children) {
            if (cell->type == MarkdownElementType::TABLE_CELL && cell->is_header) {
                has_header = true;
                break;
            }
        }
        if (has_header && !header_emitted) {
            // separator line
            oss << "├";
            for (int i = 0; i < table_num_cols_; ++i) {
                if (i > 0)
                    oss << "┼";
                int w = table_col_widths_[i] + 2;
                for (int k = 0; k < w; ++k)
                    oss << "─";
            }
            oss << "┤\n";
            header_emitted = true;
        }
    }
    std::string out = oss.str();
    if (config_.use_color)
        out = std::string("\033[90m") + out + "\033[0m";
    return text(out);
}

ftxui::Element MarkdownRenderer::render_table_row(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    // Not used anymore: table rows rendered by render_table
    std::ostringstream oss;
    oss << "│";
    int col = 0;
    for (const auto& child : element->children) {
        if (child->type != MarkdownElementType::TABLE_CELL)
            continue;
        std::string cell_text = child->content;
        if (col < table_num_cols_) {
            int target = table_col_widths_[col];
            if ((int)cell_text.length() < target)
                cell_text += std::string(target - (int)cell_text.length(), ' ');
        }
        oss << " " << cell_text << " │";
        col++;
    }
    return text(oss.str());
}

ftxui::Element MarkdownRenderer::render_table_cell(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    auto el = text(element->content);
    if (element->is_header)
        el = el | get_bold_decorator();
    return el;
}

ftxui::Element MarkdownRenderer::wrap_text(const std::string& text, int max_width) {
    using namespace ftxui;
    if (max_width <= 0 || text.length() <= size_t(max_width)) {
        return ftxui::text(text);
    }
    auto wrapped = wrap_into_lines(text, max_width);
    std::ostringstream oss;
    for (size_t i = 0; i < wrapped.size(); ++i) {
        oss << wrapped[i];
        if (i + 1 < wrapped.size())
            oss << "\n";
    }
    return ftxui::text(oss.str());
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
    // Deprecated: use get_heading_fg_color/bg instead. Keep for backward compatibility.
    return get_heading_fg_color(level);
}

ftxui::Color MarkdownRenderer::get_heading_bg_color(int level) {
    if (!config_.use_color)
        return ftxui::Color::Default;

    // Map heading levels to theme accent colors (prefer
    // function/keyword/type/number/string/operator)
    switch (level) {
        case 1:
            return theme_colors_.function;
        case 2:
            return theme_colors_.keyword;
        case 3:
            return theme_colors_.type;
        case 4:
            return theme_colors_.number;
        case 5:
            return theme_colors_.string;
        case 6:
            return theme_colors_.operator_color;
        default:
            return theme_colors_.function;
    }
}

ftxui::Color MarkdownRenderer::get_heading_fg_color(int level) {
    (void)level; // 避免未使用参数警告
    if (!config_.use_color)
        return ftxui::Color::Default;
    // For contrast, use dialog title foreground if available, otherwise general foreground
    if (theme_colors_.dialog_title_fg != ftxui::Color::Default)
        return theme_colors_.dialog_title_fg;
    return theme_colors_.foreground;
}

ftxui::Color MarkdownRenderer::get_code_color() {
    if (config_.use_color) {
        return theme_colors_.string;
    } else {
        return ftxui::Color(); // Default color
    }
}

ftxui::Color MarkdownRenderer::get_code_bg_color() {
    if (!config_.use_color) {
        return ftxui::Color::Default;
    }
    // Use a dark background for code blocks: prefer theme background or a dark gray
    if (theme_colors_.background != ftxui::Color::Default) {
        return theme_colors_.background;
    }
    // Fallback to a dark gray color (#282828 similar to Monokai background)
    return ftxui::Color::RGB(0x28, 0x28, 0x28);
}

ftxui::Color MarkdownRenderer::get_link_color() {
    if (config_.use_color) {
        return theme_colors_.type;
    } else {
        return ftxui::Color(); // Default color
    }
}

ftxui::Color MarkdownRenderer::get_blockquote_color() {
    if (config_.use_color) {
        return theme_colors_.comment;
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