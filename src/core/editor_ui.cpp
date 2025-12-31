// UI渲染相关实现
#include "core/editor.h"
#include "core/ui/ui_router.h"
#include "ui/icons.h"
#include "ui/terminal_ui.h"
#include "ui/welcome_screen.h"
#include "ui/theme_menu.h"
#include "ui/create_folder_dialog.h"
#include "ui/save_as_dialog.h"
#include "ui/cursor_config_dialog.h"
#include "ui/binary_file_view.h"
#include "features/image_preview.h"
#include "utils/logger.h"
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <map>
#include <algorithm>
#include <climits>
#include <chrono>

using namespace ftxui;

namespace pnana {
namespace core {

// UI渲染
Element Editor::renderUI() {
    // 使用 UIRouter 进行渲染（如果已初始化）
    // 注意：目前 UIRouter 只处理基本的布局和边框，对话框等仍使用原有逻辑
    if (ui_router_) {
        Element main_ui = ui_router_->render(this);
        
        // 叠加对话框（如果打开）- 这部分仍使用原有逻辑
        return overlayDialogs(main_ui);
    }
    
    // 如果 UIRouter 未初始化，使用原有逻辑
    return renderUILegacy();
}

// 原有的 UI 渲染逻辑（保留作为后备）
Element Editor::renderUILegacy() {
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
        int terminal_height = terminal_height_;
        if (terminal_height <= 0) {
            // 使用默认高度（屏幕高度的1/3）
            terminal_height = screen_.dimy() / 3;
        }
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
    
    return overlayDialogs(main_ui);
}

// 叠加对话框
Element Editor::overlayDialogs(Element main_ui) {
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
            theme_menu_.render() | center
        });
    }
    
    // 如果创建文件夹对话框打开，叠加显示
    if (show_create_folder_) {
        return dbox({
            main_ui,
            create_folder_dialog_.render() | center
        });
    }
    
    // 如果另存为对话框打开，叠加显示
    if (show_save_as_) {
        return dbox({
            main_ui,
            save_as_dialog_.render() | center
        });
    }
    
    // 光标配置对话框
    if (cursor_config_dialog_.isVisible()) {
        Elements dialog_elements = {
            main_ui,
            cursor_config_dialog_.render() | center
        };
        return dbox(dialog_elements);
    }
    
#ifdef BUILD_LUA_SUPPORT
    // 插件管理对话框
    if (plugin_manager_dialog_.isVisible()) {
        Elements dialog_elements = {
            main_ui,
            plugin_manager_dialog_.render() | center
        };
        return dbox(dialog_elements);
    }
#endif
    
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
    
#ifdef BUILD_LSP_SUPPORT
    // 如果补全弹窗打开，叠加显示
    if (completion_popup_.isVisible()) {
        // 计算补全弹窗的位置（在光标下方）
        int popup_x = completion_popup_.getPopupX();
        int popup_y = completion_popup_.getPopupY();
        
        // 计算相对于编辑器内容区域的Y位置
        // 编辑器内容区域从第2行开始（标签栏+分隔符）
        int editor_start_y = 2;
        int actual_popup_y = popup_y + editor_start_y;
        
        // 使用dbox叠加显示弹窗
        Element popup = renderCompletionPopup();
        
        // 创建定位容器：左侧空白 + 弹窗 + 右侧空白
        Element horizontal_layout = hbox({
            filler() | size(WIDTH, EQUAL, popup_x),
            popup,
            filler()
        });
        
        // 创建垂直布局：上方空白 + 弹窗 + 下方空白
        Element vertical_layout = vbox({
            filler() | size(HEIGHT, EQUAL, actual_popup_y),
            horizontal_layout,
            filler()
        });
        
        Elements completion_elements = {
            main_ui,
            vertical_layout
        };
        return dbox(completion_elements);
    }
#endif
    
    // 如果文件选择器打开，叠加显示
    if (file_picker_.isVisible()) {
        Elements picker_elements = {
            main_ui | dim,
            file_picker_.render() | center
        };
        return dbox(picker_elements);
    }
    
    // 如果分屏对话框打开，叠加显示
    if (split_dialog_.isVisible()) {
        Elements split_elements = {
            main_ui | dim,
            split_dialog_.render() | center
        };
        return dbox(split_elements);
    }
    
    // 如果 SSH 对话框打开，叠加显示
    if (ssh_dialog_.isVisible()) {
        Elements ssh_elements = {
            main_ui | dim,
            ssh_dialog_.render() | center
        };
        return dbox(ssh_elements);
    }
    
    // 没有对话框打开，返回主UI
    return main_ui;
}

Element Editor::renderTabbar() {
    auto tabs = document_manager_.getAllTabs();
    
    // 如果没有文档，显示"Welcome"标签
    if (tabs.empty()) {
        return hbox({
            text(" "),
            text(pnana::ui::icons::ROCKET) | color(theme_.getColors().keyword),
            text(" Welcome ") | color(theme_.getColors().foreground) | bold,
            text(" ")
        }) | bgcolor(theme_.getColors().menubar_bg);
    }
    
    return tabbar_.render(tabs);
}

Element Editor::renderEditor() {
    // 如果启用了分屏（区域数量 > 1），使用分屏渲染
    if (split_view_manager_.hasSplits()) {
        return renderSplitEditor();
    }
    
    // 单视图渲染（没有分屏）
    Document* doc = getCurrentDocument();
    
    // 检查文件浏览器中是否选中了图片文件
    if (file_browser_.isVisible()) {
        std::string selected_path = file_browser_.getSelectedPath();
        if (!selected_path.empty() && features::ImagePreview::isImageFile(selected_path)) {
            // 检查是否支持图片预览（需要 FFmpeg）
            if (!features::ImagePreview::isSupported()) {
                // 如果没有 FFmpeg 支持，清空预览并跳过
                if (image_preview_.isLoaded()) {
                    image_preview_.clear();
                }
            } else {
                // 计算代码区的实际可用尺寸
                int code_area_width = screen_.dimx();
                int code_area_height = screen_.dimy() - 6; // 减去标签栏、状态栏等
                
                // 如果文件浏览器打开，减去文件浏览器的宽度
                if (file_browser_.isVisible()) {
                    code_area_width -= (file_browser_width_ + 1); // +1 是分隔符
                }
                
                // 预留一些边距和图片信息空间（标题、尺寸、分隔符 = 3行）
                code_area_width -= 4;
                int available_height = code_area_height - 3 - 4; // 减去图片信息行和边距
                
                // 确保最小尺寸
                if (code_area_width < 40) code_area_width = 40;
                if (available_height < 10) available_height = 10;
                
                // 根据代码区尺寸计算预览尺寸（确保不截断）
                // 字符高度约为宽度的0.6倍，所以预览高度 = 可用高度
                // 预览宽度 = 代码区宽度
                int preview_width = code_area_width;
                int preview_height = available_height;
                
                // 如果是图片文件，显示预览
                if (!image_preview_.isLoaded() || image_preview_.getImagePath() != selected_path ||
                    image_preview_.getRenderWidth() != preview_width || image_preview_.getRenderHeight() != preview_height) {
                    // 传入宽度和高度，让 loadImage 根据这两个值计算合适的预览尺寸
                    image_preview_.loadImage(selected_path, preview_width, preview_height);
                }
            }
            
            if (image_preview_.isLoaded()) {
                Elements preview_lines;
                auto& colors = theme_.getColors();
                
                // 添加图片信息
                preview_lines.push_back(
                    hbox({
                        text("🖼️  Image Preview: ") | color(colors.function) | bold,
                        text(image_preview_.getImagePath()) | color(colors.foreground)
                    })
                );
                preview_lines.push_back(
                    hbox({
                        text("  Size: ") | color(colors.comment),
                        text(std::to_string(image_preview_.getImageWidth()) + "x" + 
                             std::to_string(image_preview_.getImageHeight())) | color(colors.foreground)
                    })
                );
                preview_lines.push_back(separator());
                
                // 使用像素数据直接渲染，使用 FTXUI 颜色 API（确保颜色正确显示）
                auto preview_pixels = image_preview_.getPreviewPixels();
                if (!preview_pixels.empty()) {
                    // 渲染所有行（因为已经在 loadImage 时根据代码区尺寸计算好了）
                    for (size_t i = 0; i < preview_pixels.size(); ++i) {
                        Elements pixel_elements;
                        const auto& row = preview_pixels[i];
                        
                        // 渲染所有像素（因为已经在 loadImage 时根据代码区宽度计算好了）
                        for (size_t j = 0; j < row.size(); ++j) {
                            const auto& pixel = row[j];
                            // 使用 FTXUI 的颜色 API 直接设置颜色，不受主题影响
                            ftxui::Color pixel_color = Color::RGB(pixel.r, pixel.g, pixel.b);
                            pixel_elements.push_back(text(pixel.ch) | color(pixel_color));
                        }
                        
                        preview_lines.push_back(hbox(pixel_elements));
                    }
                } else {
                    preview_lines.push_back(text("Failed to load image preview") | color(colors.error));
                }
                
                // 使用黑色背景以确保图片颜色正确显示，不受主题影响
                return vbox(preview_lines) | bgcolor(Color::Black);
            }
        } else {
            // 如果不是图片，清空预览
            if (image_preview_.isLoaded()) {
                image_preview_.clear();
            }
        }
    }
    
    // 如果没有文档，显示欢迎界面
    if (!doc) {
        return welcome_screen_.render();
    }
    
    // 如果是二进制文件，显示二进制文件视图
    if (doc->isBinary()) {
        binary_file_view_.setFilePath(doc->getFilePath());
        return binary_file_view_.render();
    }
    
    // 如果是新文件且内容为空，也显示欢迎界面
    if (doc->getFilePath().empty() && 
        doc->lineCount() == 1 && 
        doc->getLine(0).empty()) {
        return welcome_screen_.render();
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
    // 限制渲染的行数，避免大文件卡住
    const size_t MAX_RENDER_LINES = 200;  // 最多渲染200行
    size_t render_count = std::min(max_lines - view_offset_row_, MAX_RENDER_LINES);
    
    try {
        for (size_t i = view_offset_row_; i < view_offset_row_ + render_count; ++i) {
            try {
                // 性能优化：对于超长行，跳过语法高亮
                std::string line_content = doc->getLine(i);
                if (line_content.length() > 5000) {
                    // 超长行，使用简单渲染
                    Elements simple_line;
                    if (show_line_numbers_) {
                        simple_line.push_back(renderLineNumber(i, i == cursor_row_));
                    }
                    simple_line.push_back(text(line_content.substr(0, 5000) + "...") | 
                                         color(theme_.getColors().foreground));
                    lines.push_back(hbox(simple_line));
                } else {
                    lines.push_back(renderLine(i, i == cursor_row_));
                }
            } catch (const std::exception& e) {
                // 如果渲染某一行失败，使用空行替代
                Elements error_line;
                if (show_line_numbers_) {
                    error_line.push_back(text("    ~") | color(theme_.getColors().comment));
                } else {
                    error_line.push_back(text("~") | color(theme_.getColors().comment));
                }
                lines.push_back(hbox(error_line));
            } catch (...) {
                // 如果渲染某一行失败，使用空行替代
                Elements error_line;
                if (show_line_numbers_) {
                    error_line.push_back(text("    ~") | color(theme_.getColors().comment));
                } else {
                    error_line.push_back(text("~") | color(theme_.getColors().comment));
                }
                lines.push_back(hbox(error_line));
            }
        }
    } catch (const std::exception& e) {
        // 如果整个渲染循环失败，返回错误信息
        return vbox({
            text("Error rendering file: " + std::string(e.what())) | color(Color::Red)
        });
    } catch (...) {
        return vbox({
            text("Unknown error rendering file") | color(Color::Red)
        });
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

Element Editor::renderSplitEditor() {
    int screen_width = screen_.dimx();
    int screen_height = screen_.dimy() - 6;  // 减去标签栏、状态栏等
    
    // 更新分屏视图的尺寸
    split_view_manager_.updateRegionSizes(screen_width, screen_height);
    
    // 获取所有区域
    const auto& regions = split_view_manager_.getRegions();
    
    if (regions.empty()) {
        return renderEditor();  // 如果没有区域，回退到单视图
    }
    
    using namespace ftxui;
    
    // 如果只有一个区域，检查是否需要重置
    if (regions.size() == 1) {
        const auto& region = regions[0];
        // 如果区域尺寸无效，重置分屏管理器
        if (region.width == 0 || region.height == 0) {
            split_view_manager_.reset();
            // 回退到正常渲染
            Document* doc = getCurrentDocument();
            if (!doc) {
                return welcome_screen_.render();
            }
            // 继续正常渲染流程（会回到 renderEditor，但 hasSplits() 会返回 false）
        } else {
            // 区域有效，正常渲染该区域
            Document* doc = nullptr;
            if (region.document_index < document_manager_.getDocumentCount()) {
                doc = document_manager_.getDocument(region.document_index);
            }
            if (region.is_active && doc) {
                document_manager_.switchToDocument(region.document_index);
            }
            return renderEditorRegion(region, doc) | size(WIDTH, EQUAL, region.width) 
                                                  | size(HEIGHT, EQUAL, region.height);
        }
    }
    
    // 多个区域：构建布局
    // 找到所有区域的边界
    int min_x = INT_MAX, min_y = INT_MAX;
    int max_x = 0, max_y = 0;
    for (const auto& region : regions) {
        min_x = std::min(min_x, region.x);
        min_y = std::min(min_y, region.y);
        max_x = std::max(max_x, region.x + region.width);
        max_y = std::max(max_y, region.y + region.height);
    }
    
    // 创建布局网格（简化：使用固定布局）
    // 按 y 坐标分组（行）
    std::map<int, std::vector<const features::ViewRegion*>> rows;
    for (const auto& region : regions) {
        rows[region.y].push_back(&region);
    }
    
    Elements row_elements;
    for (auto& [y, row_regions] : rows) {
        // 按 x 坐标排序
        std::sort(row_regions.begin(), row_regions.end(), 
                  [](const features::ViewRegion* a, const features::ViewRegion* b) {
                      return a->x < b->x;
                  });
        
        Elements col_elements;
        for (size_t i = 0; i < row_regions.size(); ++i) {
            const auto* region = row_regions[i];
            
            // 获取该区域关联的文档
            Document* doc = nullptr;
            if (region->document_index < document_manager_.getDocumentCount()) {
                doc = document_manager_.getDocument(region->document_index);
            }
            
            // 如果区域是激活的，更新当前文档
            if (region->is_active && doc) {
                document_manager_.switchToDocument(region->document_index);
            }
            
            // 渲染该区域的编辑器内容
            Element region_content = renderEditorRegion(*region, doc);
            region_content = region_content | size(WIDTH, EQUAL, region->width) 
                                          | size(HEIGHT, EQUAL, region->height);
            
            col_elements.push_back(region_content);
            
            // 如果不是最后一个，添加竖直分屏线
            if (i < row_regions.size() - 1) {
                Elements line_chars;
                for (int j = 0; j < region->height; ++j) {
                    line_chars.push_back(text("│") | color(Color::GrayDark));
                }
                col_elements.push_back(vbox(line_chars) | size(WIDTH, EQUAL, 1));
            }
        }
        
        row_elements.push_back(hbox(col_elements));
        
        // 如果不是最后一行，添加横向分屏线
        auto next_row = rows.upper_bound(y);
        if (next_row != rows.end()) {
            Elements line_chars;
            int line_width = max_x - min_x;
            for (int j = 0; j < line_width; ++j) {
                line_chars.push_back(text("─") | color(Color::GrayDark));
            }
            row_elements.push_back(hbox(line_chars) | size(HEIGHT, EQUAL, 1));
        }
    }
    
    return vbox(row_elements);
}

Element Editor::renderEditorRegion(const features::ViewRegion& region, Document* doc) {
    // 如果没有文档，显示空区域
    if (!doc) {
        Elements empty_lines;
        for (int i = 0; i < region.height; ++i) {
            empty_lines.push_back(text("~") | color(theme_.getColors().comment));
        }
        return vbox(empty_lines);
    }
    
    // 如果是二进制文件，显示二进制文件视图
    if (doc->isBinary()) {
        binary_file_view_.setFilePath(doc->getFilePath());
        return binary_file_view_.render();
    }
    
    Elements lines;
    
    // 计算该区域应该显示的行数
    size_t total_lines = doc->lineCount();
    int region_height = region.height;
    
    // 如果区域是激活的，使用当前的视图偏移
    // 否则，每个区域可以有自己的视图偏移（简化实现：所有区域共享视图偏移）
    size_t start_line = view_offset_row_;
    size_t max_lines = std::min(start_line + region_height, total_lines);
    
    // 渲染可见行
    for (size_t i = start_line; i < max_lines && i < start_line + region_height; ++i) {
        bool is_current = (region.is_active && i == cursor_row_);
        lines.push_back(renderLine(i, is_current));
    }
    
    // 填充空行
    for (int i = lines.size(); i < region_height; ++i) {
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


// 渲染光标元素的辅助函数
Element Editor::renderCursorElement(const std::string& cursor_char, size_t cursor_pos, size_t line_length) const {
    auto& colors = theme_.getColors();
    ::pnana::ui::CursorStyle style = getCursorStyle();
    ftxui::Color cursor_color = getCursorColor();
    bool smooth = getCursorSmooth();
    
    // 根据样式渲染光标
    Element cursor_elem;
    
    switch (style) {
        case ::pnana::ui::CursorStyle::BLOCK: {
            // 块状光标：背景色填充
            if (cursor_pos < line_length) {
                cursor_elem = text(cursor_char) | bgcolor(cursor_color) | color(colors.background) | bold;
            } else {
                cursor_elem = text(" ") | bgcolor(cursor_color) | color(colors.background) | bold;
            }
            break;
        }
        case ::pnana::ui::CursorStyle::UNDERLINE: {
            // 下划线光标：使用反转颜色，但使用稍微暗的背景来模拟下划线效果
            // 在终端中，我们使用反转颜色来模拟下划线
            if (cursor_pos < line_length) {
                // 使用反转颜色（前景色作为背景）
                cursor_elem = text(cursor_char) | bgcolor(cursor_color) | color(colors.background);
            } else {
                // 行尾：显示下划线字符
                cursor_elem = text("▁") | color(cursor_color) | bold;
            }
            break;
        }
        case ::pnana::ui::CursorStyle::BAR: {
            // 竖线光标：字符前显示竖线
            if (cursor_pos < line_length) {
                cursor_elem = hbox({
                    text("│") | color(cursor_color) | bold,
                    text(cursor_char) | color(colors.foreground)
                });
            } else {
                cursor_elem = text("│") | color(cursor_color) | bold;
            }
            break;
        }
        case ::pnana::ui::CursorStyle::HOLLOW: {
            // 空心块光标：使用反转颜色（前景色作为边框效果）
            if (cursor_pos < line_length) {
                // 使用反转颜色模拟空心效果
                cursor_elem = text(cursor_char) | color(cursor_color) | bold | 
                             bgcolor(colors.background);
            } else {
                // 行尾：显示一个带颜色的空格
                cursor_elem = text("▯") | color(cursor_color) | bold;
            }
            break;
        }
        default: {
            // 默认块状
            if (cursor_pos < line_length) {
                cursor_elem = text(cursor_char) | bgcolor(cursor_color) | color(colors.background) | bold;
            } else {
                cursor_elem = text(" ") | bgcolor(cursor_color) | color(colors.background) | bold;
            }
            break;
        }
    }
    
    // 如果启用流动效果，可以添加额外的视觉效果
    // 注意：FTXUI 不支持动画，流动效果可以通过其他方式实现（如渐变颜色）
    if (smooth) {
        // 流动效果：使用稍微不同的颜色或样式
        // 这里简化处理，使用稍微亮一点的颜色
        // 实际流动效果需要时间相关的状态，这里先实现基础版本
    }
    
    return cursor_elem;
}

Element Editor::renderLine(size_t line_num, bool is_current) {
    Elements line_elements;
    
    // 行号
    if (show_line_numbers_) {
        line_elements.push_back(renderLineNumber(line_num, is_current));
        line_elements.push_back(text(" "));
    }
    
    // 行内容
    Document* doc = getCurrentDocument();
    if (!doc) {
        return hbox({text("~") | color(theme_.getColors().comment)});
    }
    
    if (line_num >= doc->lineCount()) {
        return hbox({text("~") | color(theme_.getColors().comment)});
    }
    
    std::string content;
    try {
        content = doc->getLine(line_num);
    } catch (const std::exception& e) {
        content = "";
    } catch (...) {
        content = "";
    }
    
    // 获取当前行的搜索匹配
    std::vector<features::SearchMatch> line_matches;
    if (search_engine_.hasMatches()) {
        const auto& all_matches = search_engine_.getAllMatches();
        for (const auto& match : all_matches) {
            if (match.line == line_num) {
                line_matches.push_back(match);
            }
        }
    }
    
    Element content_elem;
    
    // 渲染带搜索高亮的行内容
    auto renderLineWithHighlights = [&](const std::string& line_content, size_t cursor_pos, bool has_cursor) -> Element {
            Elements parts;
        auto& colors = theme_.getColors();
        
        // 性能优化：如果行太长，限制语法高亮处理
        const size_t MAX_HIGHLIGHT_LENGTH = 5000;  // 最多处理5000字符
        bool line_too_long = line_content.length() > MAX_HIGHLIGHT_LENGTH;
        
        if (line_matches.empty()) {
            // 没有搜索匹配，正常渲染
            if (has_cursor && cursor_pos <= line_content.length()) {
                std::string before = line_content.substr(0, cursor_pos);
                std::string cursor_char = cursor_pos < line_content.length() ? 
                                          line_content.substr(cursor_pos, 1) : " ";
                std::string after = cursor_pos < line_content.length() ? 
                                    line_content.substr(cursor_pos + 1) : "";
                
                if (syntax_highlighting_ && !line_too_long) {
                    // 启用语法高亮（仅当行不太长时）
                    try {
                        if (!before.empty()) {
                            parts.push_back(syntax_highlighter_.highlightLine(before));
                        }
                    } catch (...) {
                        parts.push_back(text(before) | color(colors.foreground));
                    }
                    // 使用配置的光标样式渲染
                    parts.push_back(renderCursorElement(cursor_char, cursor_pos, line_content.length()));
                    try {
                        if (!after.empty()) {
                            parts.push_back(syntax_highlighter_.highlightLine(after));
                        }
                    } catch (...) {
                        parts.push_back(text(after) | color(colors.foreground));
                    }
                } else {
                    parts.push_back(text(before) | color(colors.foreground));
                    // 使用配置的光标样式渲染
                    parts.push_back(renderCursorElement(cursor_char, cursor_pos, line_content.length()));
                    parts.push_back(text(after) | color(colors.foreground));
                }
            } else {
                // 没有光标，渲染整行
                if (syntax_highlighting_ && !line_too_long) {
                    // 启用语法高亮（仅当行不太长时）
                    try {
                        parts.push_back(syntax_highlighter_.highlightLine(line_content));
                    } catch (...) {
                        // 语法高亮失败，使用简单文本
                        parts.push_back(text(line_content) | color(colors.foreground));
                    }
                } else {
                    parts.push_back(text(line_content) | color(colors.foreground));
                }
            }
        } else {
            // 有搜索匹配，需要高亮显示
            size_t pos = 0;
            size_t match_idx = 0;
            
            while (pos < line_content.length()) {
                // 检查是否有匹配从当前位置开始
                bool found_match = false;
                for (size_t i = match_idx; i < line_matches.size(); ++i) {
                    if (line_matches[i].column == pos) {
                        // 找到匹配，高亮显示
                        size_t match_len = line_matches[i].length;
                        std::string match_text = line_content.substr(pos, match_len);
                        
                        // 检查光标是否在匹配范围内
                        bool cursor_in_match = has_cursor && cursor_pos >= pos && cursor_pos < pos + match_len;
                        
                        if (cursor_in_match) {
                            // 光标在匹配内，需要分割匹配文本
                            size_t before_cursor = cursor_pos - pos;
                            size_t after_cursor = pos + match_len - cursor_pos;
                            
                            if (before_cursor > 0) {
                                std::string before = match_text.substr(0, before_cursor);
                                parts.push_back(
                                    text(before) | 
                                    bgcolor(Color::GrayDark) | 
                                    color(colors.foreground)
                                );
                            }
                            
                            // 光标位置的字符
                            std::string cursor_char = match_text.substr(before_cursor, 1);
                            // 使用配置的光标样式渲染
                            parts.push_back(renderCursorElement(cursor_char, pos + before_cursor, line_content.length()));
                            
                            if (after_cursor > 1) {
                                std::string after = match_text.substr(before_cursor + 1);
                                parts.push_back(
                                    text(after) | 
                                    bgcolor(Color::GrayDark) | 
                                    color(colors.foreground)
                                );
                            }
        } else {
                            // 光标不在匹配内，正常高亮匹配
                            parts.push_back(
                                text(match_text) | 
                                bgcolor(Color::GrayDark) | 
                                color(colors.foreground)
                            );
                        }
                        
                        pos += match_len;
                        match_idx = i + 1;
                        found_match = true;
                        break;
                    }
                }
                
                if (!found_match) {
                    // 没有匹配，找到下一个匹配的位置
                    size_t next_match_pos = line_content.length();
                    for (size_t i = match_idx; i < line_matches.size(); ++i) {
                        if (line_matches[i].column > pos && line_matches[i].column < next_match_pos) {
                            next_match_pos = line_matches[i].column;
                        }
                    }
                    
                    std::string segment = line_content.substr(pos, next_match_pos - pos);
                    
                    // 检查光标是否在这个段内
                    if (has_cursor && cursor_pos >= pos && cursor_pos < next_match_pos) {
                        size_t before_cursor = cursor_pos - pos;
                        std::string before = segment.substr(0, before_cursor);
                        std::string cursor_char = before_cursor < segment.length() ? 
                                                  segment.substr(before_cursor, 1) : " ";
                        std::string after = before_cursor < segment.length() ? 
                                            segment.substr(before_cursor + 1) : "";
                        
                        if (syntax_highlighting_ && !line_too_long) {
                            // 启用语法高亮（仅当行不太长时）
                            try {
                                if (!before.empty()) {
                                    parts.push_back(syntax_highlighter_.highlightLine(before));
                                }
                            } catch (...) {
                                parts.push_back(text(before) | color(colors.foreground));
                            }
                            // 使用配置的光标样式渲染
                            parts.push_back(renderCursorElement(cursor_char, cursor_pos, line_content.length()));
                            try {
                                if (!after.empty()) {
                                    parts.push_back(syntax_highlighter_.highlightLine(after));
                                }
                            } catch (...) {
                                parts.push_back(text(after) | color(colors.foreground));
                            }
                        } else {
                            parts.push_back(text(before) | color(colors.foreground));
                            // 使用配置的光标样式渲染
                            parts.push_back(renderCursorElement(cursor_char, cursor_pos, line_content.length()));
                            parts.push_back(text(after) | color(colors.foreground));
        }
                    } else {
                        // 没有光标，正常渲染
                        if (syntax_highlighting_ && !line_too_long) {
                            // 启用语法高亮（仅当行不太长时）
                            try {
                                parts.push_back(syntax_highlighter_.highlightLine(segment));
                            } catch (...) {
                                parts.push_back(text(segment) | color(colors.foreground));
                            }
                        } else {
                            parts.push_back(text(segment) | color(colors.foreground));
                        }
                    }
                    
                    pos = next_match_pos;
                }
            }
        }
        
        return hbox(parts);
    };
    
    try {
        content_elem = renderLineWithHighlights(content, cursor_col_, is_current);
    } catch (const std::exception& e) {
        // 如果高亮失败，使用简单文本
        content_elem = text(content) | color(theme_.getColors().foreground);
    } catch (...) {
        // 如果高亮失败，使用简单文本
        content_elem = text(content) | color(theme_.getColors().foreground);
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
    return helpbar_.render(pnana::ui::Helpbar::getDefaultHelp());
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


Element Editor::renderHelp() {
    int width = screen_.dimx();
    int height = screen_.dimy();
    return help_.render(width, height);
}



Element Editor::renderCommandPalette() {
    return command_palette_.render();
}

Element Editor::renderTerminal() {
    int height = terminal_height_;
    if (height <= 0) {
        // 使用默认高度（屏幕高度的1/3）
        height = screen_.dimy() / 3;
    }
    return pnana::ui::renderTerminal(terminal_, height);
}

Element Editor::renderFilePicker() {
    return file_picker_.render();
}

} // namespace core
} // namespace pnana

