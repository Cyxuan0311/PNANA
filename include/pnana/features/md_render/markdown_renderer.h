#ifndef PNANA_FEATURES_MARKDOWN_RENDERER_H
#define PNANA_FEATURES_MARKDOWN_RENDERER_H

#include "features/md_render/markdown_parser.h"
#include <ftxui/dom/elements.hpp>
#include <string>

namespace pnana {
namespace features {

// Markdown渲染器配置
struct MarkdownRenderConfig {
    int max_width = 80;
    bool use_color = true;
    std::string theme = "dark"; // "dark" 或 "light"
};

// Markdown渲染器
class MarkdownRenderer {
  public:
    explicit MarkdownRenderer(const MarkdownRenderConfig& config = MarkdownRenderConfig());

    // 渲染markdown文本
    ftxui::Element render(const std::string& markdown);

    // 渲染解析后的markdown元素
    ftxui::Element render_element(const std::shared_ptr<MarkdownElement>& element, int indent = 0);

  private:
    MarkdownRenderConfig config_;
    // 临时表格列宽（在渲染单个表格时使用）
    std::vector<int> table_col_widths_;
    int table_num_cols_ = 0;

    // 渲染不同类型的元素
    ftxui::Element render_heading(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_paragraph(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_code_block(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_inline_code(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_bold(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_italic(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_link(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_image(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_list_item(const std::shared_ptr<MarkdownElement>& element,
                                    int indent = 0);
    ftxui::Element render_blockquote(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_horizontal_rule();
    ftxui::Element render_text(const std::string& text);
    ftxui::Element render_table(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_table_row(const std::shared_ptr<MarkdownElement>& element);
    ftxui::Element render_table_cell(const std::shared_ptr<MarkdownElement>& element);

    // 辅助方法
    ftxui::Element wrap_text(const std::string& text, int max_width = 0);
    std::string indent_text(const std::string& text, int indent);
    std::vector<std::string> split_lines(const std::string& text);

    // 颜色和样式
    ftxui::Color get_heading_color(int level);
    ftxui::Color get_code_color();
    ftxui::Color get_link_color();
    ftxui::Color get_blockquote_color();
    ftxui::Decorator get_bold_decorator();
    ftxui::Decorator get_italic_decorator();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_MARKDOWN_RENDERER_H
