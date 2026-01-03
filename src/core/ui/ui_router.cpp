#include "core/ui/ui_router.h"
#include "core/editor.h"
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

namespace pnana {
namespace core {
namespace ui {

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
    // 构建主UI结构
    Element main_ui = vbox({renderTabbar(editor), separator(), renderMainContent(editor),
                            renderStatusAndHelp(editor)}) |
                      bgcolor(editor->getTheme().getColors().background);

    // 叠加对话框
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

    // 如果文件浏览器打开，使用左右分栏布局
    if (editor->isFileBrowserVisible()) {
        Element file_browser = editor->renderFileBrowser();
        bool is_browser_active = (current_region == EditorRegion::FILE_BROWSER);
        file_browser = border_manager_.applyBorder(file_browser, EditorRegion::FILE_BROWSER,
                                                   is_browser_active, editor->getTheme());

        Element code_area = editor->renderEditor();
        bool is_code_active = (current_region == EditorRegion::CODE_AREA);
        code_area = border_manager_.applyBorder(code_area, EditorRegion::CODE_AREA, is_code_active,
                                                editor->getTheme());

        editor_content = hbox({file_browser | size(WIDTH, EQUAL, editor->getFileBrowserWidth()),
                               separator(), code_area | flex});
    } else {
        editor_content = editor->renderEditor();
        bool is_code_active = (current_region == EditorRegion::CODE_AREA);
        editor_content = border_manager_.applyBorder(editor_content, EditorRegion::CODE_AREA,
                                                     is_code_active, editor->getTheme());
        editor_content = editor_content | flex;
    }

    // 如果终端打开，使用上下分栏布局
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
