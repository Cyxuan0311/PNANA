#include "features/md_render/markdown_renderer.h"
#include "features/SyntaxHighlighter/syntax_highlighter.h"
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

MarkdownRenderer::MarkdownRenderer(const MarkdownRenderConfig& config,
                                   SyntaxHighlighter* syntax_highlighter)
    : config_(config), syntax_highlighter_(syntax_highlighter) {
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

ftxui::Element MarkdownRenderer::render(const std::shared_ptr<MarkdownElement>& root) {
    return render_element(root);
}

ftxui::Element MarkdownRenderer::render_element(const std::shared_ptr<MarkdownElement>& element,
                                                int indent) {
    if (!element) {
        return ftxui::text("");
    }

    using namespace ftxui;

    Element result;
    switch (element->type) {
        case MarkdownElementType::HEADING:
            result = render_heading(element);
            break;
        case MarkdownElementType::PARAGRAPH:
            result = render_paragraph(element);
            break;
        case MarkdownElementType::CODE_BLOCK:
            result = render_code_block(element);
            break;
        case MarkdownElementType::INLINE_CODE:
            result = render_inline_code(element);
            break;
        case MarkdownElementType::BOLD:
            result = render_bold(element);
            break;
        case MarkdownElementType::ITALIC:
            result = render_italic(element);
            break;
        case MarkdownElementType::LINK:
            result = render_link(element);
            break;
        case MarkdownElementType::IMAGE:
            result = render_image(element);
            break;
        case MarkdownElementType::LIST_ITEM:
            result = render_list_item(element, indent);
            break;
        case MarkdownElementType::BLOCKQUOTE:
            result = render_blockquote(element);
            break;
        case MarkdownElementType::HORIZONTAL_RULE:
            result = render_horizontal_rule();
            break;
        case MarkdownElementType::TABLE:
            result = render_table(element);
            break;
        case MarkdownElementType::TABLE_ROW:
            result = render_table_row(element);
            break;
        case MarkdownElementType::TABLE_CELL:
            result = render_table_cell(element);
            break;
        case MarkdownElementType::TEXT:
        default: {
            Elements children_elements;
            for (const auto& child : element->children) {
                children_elements.push_back(render_element(child, indent));
            }
            if (children_elements.empty()) {
                result = render_text(element->content);
            } else {
                result = vbox(std::move(children_elements));
            }
        }
    }

    // Add blank line after block-level elements for vertical spacing
    switch (element->type) {
        case MarkdownElementType::HEADING:
        case MarkdownElementType::PARAGRAPH:
        case MarkdownElementType::CODE_BLOCK:
        case MarkdownElementType::BLOCKQUOTE:
        case MarkdownElementType::LIST_ITEM:
        case MarkdownElementType::HORIZONTAL_RULE:
        case MarkdownElementType::TABLE:
            result = vbox({std::move(result), text("")});
            break;
        default:
            break;
    }

    return result;
}

ftxui::Element MarkdownRenderer::render_heading(const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    std::string text_content = element->content;
    while (!text_content.empty() && (text_content.back() == '\n' || text_content.back() == '\r'))
        text_content.pop_back();

    auto fg = get_heading_fg_color(element->level);
    auto bg = get_heading_bg_color(element->level);

    Elements inline_elems;
    if (!element->children.empty()) {
        if (!text_content.empty())
            inline_elems.push_back(text(text_content) | get_bold_decorator() | ftxui::color(fg) |
                                   ftxui::bgcolor(bg));
        for (const auto& child : element->children) {
            inline_elems.push_back(render_element(child) | get_bold_decorator() | ftxui::color(fg) |
                                   ftxui::bgcolor(bg));
        }
    } else {
        inline_elems.push_back(text(text_content) | get_bold_decorator() | ftxui::color(fg) |
                               ftxui::bgcolor(bg));
    }

    Elements block;

    if (element->level == 1) {
        // H1: centered + full-width underline
        auto heading_line = hbox(std::move(inline_elems));
        block.push_back(ftxui::hcenter(heading_line));
        if (config_.max_width > 0) {
            int w = std::min(config_.max_width, 60);
            block.push_back(text(std::string(w, '=')) | ftxui::color(fg) | ftxui::dim);
        }
    } else if (element->level == 2) {
        // H2: left accent bar
        block.push_back(hbox({
            text(" ") | ftxui::color(fg) | ftxui::bgcolor(fg) | size(ftxui::WIDTH, ftxui::EQUAL, 2),
            text(" ") | size(ftxui::WIDTH, ftxui::EQUAL, 1),
            hbox(std::move(inline_elems)),
        }));
    } else {
        // H3+: indent progressively
        int indent = (element->level - 3) * 2;
        std::string pad(indent, ' ');
        block.push_back(hbox({
            text(pad),
            hbox(std::move(inline_elems)),
        }));
    }

    return vbox(std::move(block));
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

static std::string mapCodeLangToFileType(const std::string& lang) {
    if (lang == "py" || lang == "python3")
        return "python";
    if (lang == "js" || lang == "javascript" || lang == "node")
        return "javascript";
    if (lang == "ts" || lang == "typescript")
        return "typescript";
    if (lang == "rs" || lang == "rust")
        return "rust";
    if (lang == "sh" || lang == "bash" || lang == "zsh" || lang == "shell")
        return "bash";
    if (lang == "yml" || lang == "yaml")
        return "yaml";
    if (lang == "md" || lang == "markdown")
        return "markdown";
    if (lang == "c++" || lang == "cxx" || lang == "cc")
        return "cpp";
    if (lang == "h" || lang == "hpp")
        return "cpp";
    if (lang == "jsx")
        return "javascript";
    if (lang == "tsx")
        return "typescript";
    if (lang == "kt" || lang == "kotlin")
        return "kotlin";
    if (lang == "swift")
        return "swift";
    if (lang == "rb" || lang == "ruby")
        return "ruby";
    if (lang == "pl" || lang == "perl")
        return "perl";
    if (lang == "php")
        return "php";
    if (lang == "scala")
        return "scala";
    if (lang == "lua")
        return "lua";
    if (lang == "go" || lang == "golang")
        return "go";
    if (lang == "dockerfile" || lang == "docker")
        return "dockerfile";
    if (lang == "makefile" || lang == "make")
        return "makefile";
    if (lang == "cmake" || lang == "cmakelists")
        return "cmake";
    if (lang == "sql")
        return "sql";
    return lang;
}

ftxui::Element MarkdownRenderer::render_code_block(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;

    Elements rendered;
    std::string lang = element->lang;
    auto bg = get_code_bg_color();
    auto fg = get_code_color();

    // Language tag (right-aligned, dim)
    if (!lang.empty()) {
        std::string tag = " " + lang + " ";
        rendered.push_back(hbox({filler(), text(tag) | dim | color(fg) | bgcolor(bg)}) |
                           bgcolor(bg));
    }

    // Body: code lines with dark background
    Elements body_lines;
    auto raw_lines = split_lines(element->content);

    if (syntax_highlighter_ && !lang.empty()) {
        // Use syntax highlighting
        std::string file_type = mapCodeLangToFileType(lang);
        syntax_highlighter_->setFileType(file_type);
        for (auto& ln : raw_lines) {
            body_lines.push_back(syntax_highlighter_->highlightLine(ln) | bgcolor(bg));
        }
    } else {
        // Plain text fallback
        for (auto& ln : raw_lines) {
            body_lines.push_back(text(ln) | color(fg) | bgcolor(bg));
        }
    }

    if (body_lines.empty())
        body_lines.push_back(text("") | bgcolor(bg));
    rendered.push_back(vbox(std::move(body_lines)));

    // Bottom padding
    rendered.push_back(text("") | bgcolor(bg));

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
    auto link_color = get_link_color();
    std::string display_text = element->content;
    if (display_text.empty())
        display_text = element->url;
    if (display_text.empty())
        display_text = "[Link]";

    // Show as [text](url)
    std::string full_text;
    if (!element->url.empty() && element->url != display_text) {
        full_text = "[" + display_text + "](" + element->url + ")";
    } else {
        full_text = display_text;
    }

    return text(full_text) | ftxui::color(link_color) | ftxui::underlined | ftxui::dim;
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

    // Task list or regular bullet
    std::string marker;
    Color marker_color = theme_colors_.foreground;
    if (element->is_task) {
        marker = element->task_checked ? "[✓] " : "[ ] ";
        marker_color = element->task_checked ? theme_colors_.success : theme_colors_.comment;
    } else {
        marker = "• ";
        marker_color = theme_colors_.keyword;
    }

    int available_width =
        config_.max_width - static_cast<int>(indent_str.size()) - static_cast<int>(marker.size());
    if (available_width <= 0)
        available_width = config_.max_width;

    if (!element->children.empty()) {
        Elements elems;
        elems.push_back(text(indent_str + marker) | color(marker_color));
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
            oss << "\n" << indent_str << std::string(marker.size() - 1, ' ') << " " << wrapped[i];
        }
    } else {
        oss << indent_str << marker;
    }
    return text(oss.str()) | color(marker_color);
}

ftxui::Element MarkdownRenderer::render_blockquote(
    const std::shared_ptr<MarkdownElement>& element) {
    using namespace ftxui;
    auto bq_color = get_blockquote_color();
    Elements lines_el;

    if (!element->children.empty()) {
        for (const auto& child : element->children) {
            auto child_el = render_element(child);
            lines_el.push_back(hbox({
                text("▎") | ftxui::color(bq_color) | bold,
                text(" ") | size(WIDTH, EQUAL, 1),
                child_el | ftxui::dim,
            }));
        }
    } else {
        auto lines = wrap_into_lines(element->content, config_.max_width - 4);
        for (const auto& ln : lines) {
            lines_el.push_back(hbox({
                text("▎") | ftxui::color(bq_color) | bold,
                text(" ") | size(WIDTH, EQUAL, 1),
                text(ln) | ftxui::dim,
            }));
        }
    }

    return vbox(std::move(lines_el));
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