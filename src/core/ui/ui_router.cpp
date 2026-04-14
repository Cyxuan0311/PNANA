#include "core/ui/ui_router.h"
#include "core/editor.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace ui {

// 创建 tmux 风格的分屏线：激活侧高亮
// is_vertical: true 为竖直分屏线，false 为横向分屏线
// left_or_top_active: 左侧/上方区域是否激活
// right_or_bottom_active: 右侧/下方区域是否激活
// separator_default: 默认颜色
// separator_active: 激活侧高亮色
static Element createTmuxStyleSeparator(bool is_vertical, bool left_or_top_active,
                                        bool right_or_bottom_active,
                                        const ftxui::Color& separator_default,
                                        const ftxui::Color& separator_active) {
    if (is_vertical) {
        // 竖直分屏线：上半部分和下半部分根据激活状态设置颜色
        // 获取屏幕高度（这里使用一个合理的默认值，实际高度由 FTXUI 布局决定）
        int height = 30; // 默认高度，会被布局自动调整
        Elements line_chars;
        int half_h = height / 2;

        for (int i = 0; i < height; ++i) {
            bool use_active = (i < half_h) ? left_or_top_active : right_or_bottom_active;
            line_chars.push_back(text("│") |
                                 color(use_active ? separator_active : separator_default));
        }
        return vbox(line_chars) | size(WIDTH, EQUAL, 1);
    } else {
        // 横向分屏线：左半部分和右半部分根据激活状态设置颜色
        int width = 80; // 默认宽度，会被布局自动调整
        Elements line_chars;
        int half_w = width / 2;

        for (int i = 0; i < width; ++i) {
            bool use_active = (i < half_w) ? left_or_top_active : right_or_bottom_active;
            line_chars.push_back(text("─") |
                                 color(use_active ? separator_active : separator_default));
        }
        return hbox(line_chars) | size(HEIGHT, EQUAL, 1);
    }
}

UIRouter::UIRouter() : initialized_(false) {
    initializeRegionRenderers();
    initialized_ = true;
}

UIRouter::~UIRouter() = default;

void UIRouter::initializeRegionRenderers() {
    // 区域渲染器将在后续阶段实现
    // 这里先留空，后续添加各个区域渲染器的初始化
}

Element UIRouter::render(Editor* editor) {
    // 构建主 UI 结构：主内容区使用 flex 占满中间空间，保证状态栏/帮助栏始终贴底
    // 边框高亮由 RegionManager 的当前区域决定（含 CODE_AREA / FILE_BROWSER / TERMINAL /
    // GIT_PANEL / AI_ASSISTANT_PANEL）；AI 面板在 overlay 中单独渲染并随当前区域高亮边框
    Element main_ui = vbox({renderTabbar(editor), separator(), renderMainContent(editor) | flex,
                            renderStatusAndHelp(editor)}) |
                      bgcolor(editor->getTheme().getColors().background);

    return overlayDialogs(main_ui, editor);
}

Element UIRouter::renderTabbar(Editor* editor) {
    // 获取当前区域
    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();
    bool is_tab_active = (current_region == EditorRegion::TAB_AREA);

    // 渲染标签栏（这里需要调用 Editor 的 renderTabbar 方法）
    // 暂时使用简单的实现，后续会迁移到 TabAreaRenderer
    Element tabbar_content = editor->renderTabbar();

    // 如果标签栏激活，应用边框
    if (is_tab_active) {
        return border_manager_.applyBorder(tabbar_content, EditorRegion::TAB_AREA, true,
                                           editor->getTheme());
    }

    return tabbar_content;
}

Element UIRouter::renderMainContent(Editor* editor) {
    // 获取当前区域
    EditorRegion current_region = editor->getRegionManager().getCurrentRegion();

    Element editor_content;

    // 如果 markdown 预览激活，使用分屏布局
    if (editor->isMarkdownPreviewActive()) {
        Element code_area = editor->renderEditor();
        bool is_code_active = (current_region == EditorRegion::CODE_AREA);

        Element preview_area = editor->renderMarkdownPreview();
        bool is_preview_active =
            (current_region == EditorRegion::CODE_AREA); // 预览区域也属于代码区域

        // 强制左右均分宽度（不包括边框），确保代码区与预览区等宽
        int screen_width = editor->getScreenWidth();
        int half_width = std::max(10, (screen_width - 1) / 2); // 留出 1 列给分隔符

        // 使用 tmux 风格的分屏线：激活侧高亮
        const auto& tc = editor->getTheme().getColors();
        Element separator_line = createTmuxStyleSeparator(true, is_code_active, is_preview_active,
                                                          tc.dialog_border, tc.line_number_current);

        // 先将两个区域合并，然后再应用单个边框
        editor_content = hbox({code_area | size(WIDTH, EQUAL, half_width), separator_line,
                               preview_area | size(WIDTH, EQUAL, half_width)});
        editor_content =
            border_manager_.applyBorder(editor_content, EditorRegion::CODE_AREA,
                                        is_code_active || is_preview_active, editor->getTheme());
    }
    // 如果文件浏览器打开，使用左右分栏布局，位置由 display.file_browser_side 配置决定
    else if (editor->isFileBrowserVisible()) {
        Element file_browser_panel = editor->renderFileBrowser();
        bool is_browser_active = (current_region == EditorRegion::FILE_BROWSER);
        file_browser_panel = border_manager_.applyBorder(
            file_browser_panel, EditorRegion::FILE_BROWSER, is_browser_active, editor->getTheme());
        file_browser_panel = file_browser_panel | size(WIDTH, EQUAL, editor->getFileBrowserWidth());

        Element code_area = editor->renderEditor();
        bool is_code_active = (current_region == EditorRegion::CODE_AREA);
        code_area = border_manager_.applyBorder(code_area, EditorRegion::CODE_AREA, is_code_active,
                                                editor->getTheme());

        bool file_browser_on_left =
            editor->getConfigManager().getConfig().display.file_browser_side != "right";
        if (file_browser_on_left) {
            editor_content = hbox({file_browser_panel, separator(), code_area | flex});
        } else {
            editor_content = hbox({code_area | flex, separator(), file_browser_panel});
        }
    } else {
        editor_content = editor->renderEditor();
        bool is_code_active = (current_region == EditorRegion::CODE_AREA);
        editor_content = border_manager_.applyBorder(editor_content, EditorRegion::CODE_AREA,
                                                     is_code_active, editor->getTheme());
        editor_content = editor_content | flex;
    }

    // 如果终端打开，使用上下分栏布局，位置由 display.terminal_side 决定
    if (editor->isTerminalVisible()) {
        int terminal_height = editor->getTerminalHeight();
        if (terminal_height <= 0) {
            // 使用默认高度（屏幕高度的1/3）
            terminal_height = editor->getScreenHeight() / 3;
        }
        Element terminal = editor->renderTerminal();
        bool is_terminal_active = (current_region == EditorRegion::TERMINAL);
        terminal = border_manager_.applyBorder(terminal, EditorRegion::TERMINAL, is_terminal_active,
                                               editor->getTheme());

        bool terminal_on_top =
            editor->getConfigManager().getConfig().display.terminal_side == "top";
        if (terminal_on_top) {
            return vbox({terminal | size(HEIGHT, EQUAL, terminal_height), separator(),
                         editor_content | flex});
        }
        return vbox(
            {editor_content | flex, separator(), terminal | size(HEIGHT, EQUAL, terminal_height)});
    }

    return editor_content;
}

Element UIRouter::renderStatusAndHelp(Editor* editor) {
    // 渲染状态栏、输入框、帮助栏
    // 这里需要调用 Editor 的相应方法
    return vbox({editor->renderStatusbar(), editor->renderInputBox(), editor->renderHelpbar()});
}

Element UIRouter::overlayDialogs(Element main_ui, Editor* editor) {
    // 使用 Editor 的 overlayDialogs 方法
    return editor->overlayDialogs(main_ui);
}

} // namespace ui
} // namespace core
} // namespace pnana
