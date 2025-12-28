// UI渲染相关实现
#include "core/editor.h"
#include "ui/icons.h"
#include <ftxui/dom/elements.hpp>
#include <sstream>

using namespace ftxui;

namespace pnana {
namespace core {

// UI渲染
Element Editor::renderUI() {
    Element editor_content;
    
    // 如果文件浏览器打开，使用左右分栏布局
    if (file_browser_.isVisible()) {
        editor_content = hbox({
            renderFileBrowser() | size(WIDTH, EQUAL, file_browser_width_),
            separator(),
            renderEditor() | flex
        });
    } else {
        editor_content = renderEditor() | flex;
    }
    
    // 如果终端打开，使用上下分栏布局
    Element main_content;
    if (terminal_.isVisible()) {
        int terminal_height = screen_.dimy() / 3;  // 终端占屏幕高度的1/3
        main_content = vbox({
            editor_content | flex,
            separator(),
            renderTerminal() | size(HEIGHT, EQUAL, terminal_height)
        });
    } else {
        main_content = editor_content;
    }
    
    auto main_ui = vbox({
        renderTabbar(),
        separator(),
        main_content,
        renderStatusbar(),
        renderInputBox(),
        renderHelpbar()
    }) | bgcolor(theme_.getColors().background);
    
    // 如果帮助窗口打开，叠加显示
    if (show_help_) {
        return dbox({
            main_ui,
            renderHelp() | center
        });
    }
    
    // 如果主题菜单打开，叠加显示
    if (show_theme_menu_) {
        return dbox({
            main_ui,
            renderThemeMenu() | center
        });
    }
    
    // 如果创建文件夹对话框打开，叠加显示
    if (show_create_folder_) {
        return dbox({
            main_ui,
            renderCreateFolderDialog() | center
        });
    }
    
    // 如果另存为对话框打开，叠加显示
    if (show_save_as_) {
        return dbox({
            main_ui,
            renderSaveAsDialog() | center
        });
    }
    
    // 如果命令面板打开，叠加显示
    if (command_palette_.isOpen()) {
        return dbox({
            main_ui,
            renderCommandPalette() | center
        });
    }
    
    // 如果对话框打开，叠加显示
    if (dialog_.isVisible()) {
        Elements dialog_elements = {
            main_ui | dim,
            dialog_.render() | center
        };
        return dbox(dialog_elements);
    }
    
    // 如果文件选择器打开，叠加显示
    if (file_picker_.isVisible()) {
        Elements picker_elements = {
            main_ui | dim,
            file_picker_.render() | center
        };
        return dbox(picker_elements);
    }
    
    return main_ui;
}

Element Editor::renderTabbar() {
    auto tabs = document_manager_.getAllTabs();
    // 如果没有文档，显示"Welcome"标签
    if (tabs.empty()) {
        return hbox({
            text(" "),
            text(ui::icons::ROCKET) | color(theme_.getColors().keyword),
            text(" Welcome ") | color(theme_.getColors().foreground) | bold,
            text(" ")
        }) | bgcolor(theme_.getColors().menubar_bg);
    }
    return tabbar_.render(tabs);
}

Element Editor::renderEditor() {
    Document* doc = getCurrentDocument();
    // 如果没有文档，显示欢迎界面
    if (!doc) {
        return renderWelcomeScreen();
    }
    
    // 如果是新文件且内容为空，也显示欢迎界面
    if (doc->getFilePath().empty() && 
        doc->lineCount() == 1 && 
        doc->getLine(0).empty()) {
        return renderWelcomeScreen();
    }
    
    Elements lines;
    
    // 统一计算屏幕高度：减去标签栏(1) + 分隔符(1) + 状态栏(1) + 输入框(1) + 帮助栏(1) + 分隔符(1) = 6行
    int screen_height = screen_.dimy() - 6;
    size_t total_lines = doc->lineCount();
    
    // 只在文件行数少于屏幕高度时，确保从0开始显示（这样最后一行也能显示）
    // 如果文件行数大于屏幕高度，保持当前的视图偏移，让用户自己滚动
    if (total_lines > 0 && total_lines <= static_cast<size_t>(screen_height)) {
        // 文件行数少于屏幕高度，从0开始显示所有行（包括最后一行）
            view_offset_row_ = 0;
        } 
    // 如果文件行数大于屏幕高度，不强制调整视图偏移，保持用户当前的滚动位置
    
    // 计算实际显示的行数范围
    size_t max_lines = std::min(view_offset_row_ + screen_height, total_lines);
    
    // 渲染可见行
    for (size_t i = view_offset_row_; i < max_lines; ++i) {
        lines.push_back(renderLine(i, i == cursor_row_));
    }
    
    // 填充空行
    for (int i = lines.size(); i < screen_height; ++i) {
        Elements empty_line;
        if (show_line_numbers_) {
            empty_line.push_back(text("    ~") | color(theme_.getColors().comment));
        } else {
            empty_line.push_back(text("~") | color(theme_.getColors().comment));
        }
        lines.push_back(hbox(empty_line));
    }
    
    return vbox(lines);
}

Element Editor::renderWelcomeScreen() {
    auto& colors = theme_.getColors();
    
    Elements welcome_content;
    
    // 空行
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));
    
    // Logo和标题
    welcome_content.push_back(
        text("  ██████╗ ███╗   ██╗ █████╗ ███╗   ██╗ █████╗ ") 
        | color(colors.success) | bold | center
    );
    welcome_content.push_back(
        text("  ██╔══██╗████╗  ██║██╔══██╗████╗  ██║██╔══██╗") 
        | color(colors.success) | bold | center
    );
    welcome_content.push_back(
        text("  ██████╔╝██╔██╗ ██║███████║██╔██╗ ██║███████║") 
        | color(colors.success) | bold | center
    );
    welcome_content.push_back(
        text("  ██╔═══╝ ██║╚██╗██║██╔══██║██║╚██╗██║██╔══██║") 
        | color(colors.success) | bold | center
    );
    welcome_content.push_back(
        text("  ██║     ██║ ╚████║██║  ██║██║ ╚████║██║  ██║") 
        | color(colors.success) | bold | center
    );
    welcome_content.push_back(
        text("  ╚═╝     ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝") 
        | color(colors.success) | bold | center
    );
    
    welcome_content.push_back(text(""));
    welcome_content.push_back(
        text("Modern Terminal Text Editor") 
        | color(colors.foreground) | center
    );
    welcome_content.push_back(
        text("Version 0.0.3") 
        | color(colors.comment) | dim | center
    );
    
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));
    
    // Start editing hint (highlighted)
    welcome_content.push_back(
        hbox({
            text(" "),
            text(ui::icons::BULB) | color(colors.warning),
            text(" Press "),
            text(" i ") | bgcolor(colors.keyword) | color(colors.background) | bold,
            text(" to start editing a new document ")
        }) | color(colors.foreground) | center
    );
    
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));
    
    // Quick Start section
    welcome_content.push_back(
        hbox({
            text(ui::icons::ROCKET),
            text(" Quick Start")
        }) | color(colors.keyword) | bold | center
    );
    welcome_content.push_back(text(""));
    
    welcome_content.push_back(
        hbox({
            text("  "),
            text("Ctrl+O") | color(colors.function) | bold,
            text("  Open file    "),
            text("Ctrl+N") | color(colors.function) | bold,
            text("  New file")
        }) | center
    );
    
    welcome_content.push_back(
        hbox({
            text("  "),
            text("Ctrl+S") | color(colors.function) | bold,
            text("  Save file    "),
            text("Ctrl+Q") | color(colors.function) | bold,
            text("  Quit editor")
        }) | center
    );
    
    welcome_content.push_back(text(""));
    
    // Features section
    welcome_content.push_back(
        hbox({
            text(ui::icons::STAR),
            text(" Features")
        }) | color(colors.keyword) | bold | center
    );
    welcome_content.push_back(text(""));
    
    welcome_content.push_back(
        hbox({
            text("  "),
            text("Ctrl+F") | color(colors.function) | bold,
            text("  Search       "),
            text("Ctrl+G") | color(colors.function) | bold,
            text("  Go to line")
        }) | center
    );
    
    welcome_content.push_back(
        hbox({
            text("  "),
            text("Ctrl+T") | color(colors.function) | bold,
            text("  Themes       "),
            text("Ctrl+Z/Y") | color(colors.function) | bold,
            text("  Undo/Redo")
        }) | center
    );
    
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));
    
    // 提示信息
    welcome_content.push_back(
        hbox({
            text(ui::icons::BULB),
            text(" Tip: Just start typing to begin editing!")
        }) | color(colors.success) | center
    );
    
    welcome_content.push_back(text(""));
    
    welcome_content.push_back(
        text("Press Ctrl+T to choose from 6 beautiful themes") 
        | color(colors.comment) | dim | center
    );
    
    welcome_content.push_back(text(""));
    welcome_content.push_back(text(""));
    
    // 底部信息
    welcome_content.push_back(
        text("─────────────────────────────────────────────────") 
        | color(colors.comment) | dim | center
    );
    welcome_content.push_back(
        text("Check the bottom bar for more shortcuts") 
        | color(colors.comment) | dim | center
    );
    
    return vbox(welcome_content) | center | flex;
}

Element Editor::renderLine(size_t line_num, bool is_current) {
    Elements line_elements;
    
    // 行号
    if (show_line_numbers_) {
        line_elements.push_back(renderLineNumber(line_num, is_current));
        line_elements.push_back(text(" "));
    }
    
    // 行内容
    std::string content = getCurrentDocument()->getLine(line_num);
    
    Element content_elem;
    
    // 使用语法高亮
    if (syntax_highlighting_) {
        if (is_current && cursor_col_ <= content.length()) {
            // 当前行有光标，使用块状光标覆盖模式
            std::string before = content.substr(0, cursor_col_);
            std::string cursor_char = cursor_col_ < content.length() ? 
                                      content.substr(cursor_col_, 1) : " ";
            std::string after = cursor_col_ < content.length() ? 
                                content.substr(cursor_col_ + 1) : "";
            
            Elements parts;
            if (!before.empty()) {
                parts.push_back(syntax_highlighter_.highlightLine(before));
            }
            // 块状光标：使用明确的背景色块覆盖，文字颜色反转以保持可见
            auto& colors = theme_.getColors();
            if (!content.empty() && cursor_col_ < content.length()) {
                // 光标位置有字符：使用前景色作为背景，背景色作为文字颜色，创建块状覆盖
                // 这样文字仍然可见，但被块状背景覆盖
                parts.push_back(
                    text(cursor_char) | 
                    bgcolor(colors.foreground) | 
                    color(colors.background) | 
                    bold
                );
            } else {
                // 光标在行尾：显示空格块
                parts.push_back(
                    text(" ") | 
                    bgcolor(colors.foreground) | 
                    color(colors.background) | 
                    bold
                );
            }
            if (!after.empty()) {
                parts.push_back(syntax_highlighter_.highlightLine(after));
            }
            content_elem = hbox(parts);
        } else {
            content_elem = syntax_highlighter_.highlightLine(content);
        }
    } else {
        // 不使用语法高亮
        if (is_current && cursor_col_ <= content.length()) {
            // 块状光标覆盖模式：保持文字完整，光标位置使用背景色块覆盖
            std::string before = content.substr(0, cursor_col_);
            std::string cursor_char = cursor_col_ < content.length() ? 
                                      content.substr(cursor_col_, 1) : " ";
            std::string after = cursor_col_ < content.length() ? 
                                content.substr(cursor_col_ + 1) : "";
            
            Elements parts;
            parts.push_back(text(before) | color(theme_.getColors().foreground));
            // 块状光标：使用明确的背景色块覆盖，文字颜色反转以保持可见
            auto& colors = theme_.getColors();
            parts.push_back(
                text(cursor_char) | 
                bgcolor(colors.foreground) | 
                color(colors.background) | 
                bold
            );
            parts.push_back(text(after) | color(theme_.getColors().foreground));
            content_elem = hbox(parts);
        } else {
            content_elem = text(content) | color(theme_.getColors().foreground);
        }
    }
    
    line_elements.push_back(content_elem);
    
    Element line_elem = hbox(line_elements);
    
    // 高亮当前行背景
    if (is_current) {
        line_elem = line_elem | bgcolor(theme_.getColors().current_line);
    }
    
    return line_elem;
}

Element Editor::renderLineNumber(size_t line_num, bool is_current) {
    std::string line_str;
    
    if (relative_line_numbers_ && !is_current) {
        size_t diff = (line_num > cursor_row_) ? 
                     (line_num - cursor_row_) : (cursor_row_ - line_num);
        line_str = std::to_string(diff);
    } else {
        line_str = std::to_string(line_num + 1);
    }
    
    // 右对齐
    while (line_str.length() < 4) {
        line_str = " " + line_str;
    }
    
    return text(line_str) | 
           (is_current ? color(theme_.getColors().line_number_current) | bold 
                       : color(theme_.getColors().line_number));
}

Element Editor::renderStatusbar() {
    // If no document, show welcome status
    if (getCurrentDocument() == nullptr) {
        return statusbar_.render(
            "Welcome",
            false,  // not modified
            false,  // not readonly
            0,      // line
            0,      // col
            0,      // total lines
            "UTF-8",
            "LF",
            "text",
            status_message_.empty() ? "Press i to start editing" : status_message_,
            region_manager_.getRegionName(),
            false,  // syntax highlighting
            false,  // has selection
            0       // selection length
        );
    }
    
    // 获取行尾类型
    std::string line_ending;
    switch (getCurrentDocument()->getLineEnding()) {
        case Document::LineEnding::LF:
            line_ending = "LF";
            break;
        case Document::LineEnding::CRLF:
            line_ending = "CRLF";
            break;
        case Document::LineEnding::CR:
            line_ending = "CR";
            break;
    }
    
    return statusbar_.render(
        getCurrentDocument()->getFileName(),
        getCurrentDocument()->isModified(),
        getCurrentDocument()->isReadOnly(),
        cursor_row_,
        cursor_col_,
        getCurrentDocument()->lineCount(),
        getCurrentDocument()->getEncoding(),
        line_ending,
        getFileType(),
        status_message_,
        region_manager_.getRegionName(),
        syntax_highlighting_,
        selection_active_,
        selection_active_ ? 
            (cursor_row_ != selection_start_row_ || cursor_col_ != selection_start_col_ ? 1 : 0) : 0
    );
}

Element Editor::renderHelpbar() {
    return helpbar_.render(ui::Helpbar::getDefaultHelp());
}

Element Editor::renderInputBox() {
    if (mode_ == EditorMode::SEARCH || 
        mode_ == EditorMode::REPLACE || 
        mode_ == EditorMode::GOTO_LINE) {
        return text(status_message_ + input_buffer_) 
            | bgcolor(theme_.getColors().menubar_bg)
            | color(theme_.getColors().menubar_fg);
    }
    return text("");
}

Element Editor::renderFileBrowser() {
    int height = screen_.dimy() - 4;  // 减去状态栏等高度
    return file_browser_.render(height);
}

Element Editor::renderThemeMenu() {
    auto& current_colors = theme_.getColors();
    Elements theme_items;
    
    // 标题栏
    theme_items.push_back(
        hbox({
            text(" "),
            text(ui::icons::THEME) | color(Color::Cyan),
            text(" Select Theme ") | bold | color(current_colors.foreground),
            filler()
        }) | bgcolor(current_colors.menubar_bg)
    );
    theme_items.push_back(separator());
    
    // 主题列表
    for (size_t i = 0; i < available_themes_.size(); ++i) {
        std::string theme_name = available_themes_[i];
        
        // 获取主题颜色预览
        ui::Theme temp_theme;
        temp_theme.setTheme(theme_name);
        auto& theme_colors = temp_theme.getColors();
        
        // 创建颜色预览块（显示主要颜色）
        Elements color_preview_elements = {
            text("█") | color(theme_colors.background) | bgcolor(theme_colors.background),
            text("█") | color(theme_colors.keyword) | bgcolor(theme_colors.keyword),
            text("█") | color(theme_colors.string) | bgcolor(theme_colors.string),
            text("█") | color(theme_colors.function) | bgcolor(theme_colors.function),
            text("█") | color(theme_colors.type) | bgcolor(theme_colors.type),
            text(" ")
        };
        auto color_preview = hbox(color_preview_elements);
        
        // 主题名称
        std::string display_name = theme_name;
        if (theme_name == theme_.getCurrentThemeName()) {
            display_name += " " + std::string(ui::icons::SUCCESS);
        }
        
        auto name_text = text(display_name);
        
        // 选中状态样式
        if (i == selected_theme_index_) {
            name_text = name_text | bold | color(current_colors.function);
            color_preview = color_preview | bgcolor(current_colors.selection);
        } else {
            name_text = name_text | color(current_colors.foreground);
        }
        
        // 组合行
        Elements row_elements = {
            text(" "),
            (i == selected_theme_index_ ? text("►") | color(current_colors.function) : text(" ")),
            text(" "),
            color_preview,
            text(" "),
            name_text,
            filler()
        };
        theme_items.push_back(
            hbox(row_elements) | (i == selected_theme_index_ ? bgcolor(current_colors.selection) : bgcolor(current_colors.background))
        );
    }
    
    theme_items.push_back(separator());
    
    // 底部提示
    theme_items.push_back(
        hbox({
            text(" "),
            text("↑↓: Navigate") | color(current_colors.comment),
            text("  "),
            text("Enter: Apply") | color(current_colors.comment),
            text("  "),
            text("Esc: Cancel") | color(current_colors.comment),
            filler()
        }) | bgcolor(current_colors.menubar_bg)
    );
    
    return vbox(theme_items) 
        | border
        | bgcolor(current_colors.background)
        | size(WIDTH, GREATER_THAN, 50)
        | size(HEIGHT, GREATER_THAN, 16);
}

Element Editor::renderHelp() {
    int width = screen_.dimx();
    int height = screen_.dimy();
    return help_.render(width, height);
}

Element Editor::renderCreateFolderDialog() {
    auto& colors = theme_.getColors();
    
    Elements dialog_content;
    
    // 标题
    dialog_content.push_back(
        hbox({
            text(" "),
            text(ui::icons::FOLDER) | color(colors.keyword),
            text(" Create New Folder "),
            text(" ")
        }) | bold | bgcolor(colors.menubar_bg) | center
    );
    
    dialog_content.push_back(separator());
    
    // 当前目录
    dialog_content.push_back(text(""));
    std::string curr_dir = file_browser_.getCurrentDirectory();
    std::string display_dir = curr_dir.length() > 40 ? 
        "..." + curr_dir.substr(curr_dir.length() - 37) : curr_dir;
    dialog_content.push_back(
        hbox({
            text("  Location: "),
            text(display_dir) | color(colors.comment)
        })
    );
    
    dialog_content.push_back(text(""));
    
    // 输入框 - 使用背景色突出显示输入区域，实时显示用户输入
    std::string folder_input_display = folder_name_input_.empty() ? "_" : folder_name_input_ + "_";
    dialog_content.push_back(
        hbox({
            text("  Folder name: "),
            text(folder_input_display) | bold | color(colors.foreground) | bgcolor(colors.selection)
        })
    );
    
    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());
    
    // 提示
    dialog_content.push_back(
        hbox({
            text("  "),
            text("Enter") | color(colors.function) | bold,
            text(": Create  "),
            text("Esc") | color(colors.function) | bold,
            text(": Cancel  ")
        }) | dim
    );
    
    return window(
        text(""),
        vbox(dialog_content)
    ) | size(WIDTH, EQUAL, 50) 
      | size(HEIGHT, EQUAL, 12)
      | bgcolor(colors.background)
      | border;
}

Element Editor::renderSaveAsDialog() {
    auto& colors = theme_.getColors();
    
    Elements dialog_content;
    
    // 检查是否为未命名文件
    Document* doc = getCurrentDocument();
    bool is_untitled = doc && doc->getFilePath().empty();
    
    // 标题 - 根据是否为未命名文件显示不同标题
    dialog_content.push_back(
        hbox({
            text(" "),
            text(ui::icons::SAVE) | color(colors.keyword),
            text(is_untitled ? " Save New File " : " Save As "),
            text(" ")
        }) | bold | bgcolor(colors.menubar_bg) | center
    );
    
    dialog_content.push_back(separator());
    
    // 当前文件信息
    dialog_content.push_back(text(""));
    if (doc) {
        std::string curr_file = doc->getFileName();
        if (is_untitled) {
            // 未命名文件：显示提示信息
            dialog_content.push_back(
                hbox({
                    text("  Status: "),
                    text("[Untitled] - Enter file name") | color(colors.warning)
                })
            );
        } else {
            // 已命名文件：显示当前文件名
            dialog_content.push_back(
                hbox({
                    text("  Current: "),
                    text(curr_file) | color(colors.comment)
                })
            );
        }
    }
    
    dialog_content.push_back(text(""));
    
    // 输入框 - 根据是否为未命名文件显示不同标签
    // 使用背景色突出显示输入区域，实时显示用户输入
    std::string input_display = save_as_input_.empty() ? "_" : save_as_input_ + "_";
    dialog_content.push_back(
        hbox({
            text(is_untitled ? "  File name: " : "  File path: "),
            text(input_display) | bold | color(colors.foreground) | bgcolor(colors.selection)
        })
    );
    
    dialog_content.push_back(text(""));
    dialog_content.push_back(separator());
    
    // 提示
    dialog_content.push_back(
        hbox({
            text("  "),
            text("Enter") | color(colors.function) | bold,
            text(": Save  "),
            text("Esc") | color(colors.function) | bold,
            text(": Cancel  ")
        }) | dim
    );
    
    return window(
        text(""),
        vbox(dialog_content)
    ) | size(WIDTH, EQUAL, 60) 
      | size(HEIGHT, EQUAL, 12)
      | bgcolor(colors.background)
      | border;
}

Element Editor::renderCommandPalette() {
    return command_palette_.render();
}

Element Editor::renderTerminal() {
    int height = screen_.dimy() / 3;
    return terminal_.render(height);
}

Element Editor::renderFilePicker() {
    return file_picker_.render();
}

} // namespace core
} // namespace pnana

