// 文件操作相关实现
#include "core/editor.h"
#include "ui/icons.h"

namespace pnana {
namespace core {

// 文件操作
bool Editor::openFile(const std::string& filepath) {
    document_manager_.openDocument(filepath);
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;  // 打开文件时，总是从文件头部开始显示
    view_offset_col_ = 0;
    
    // 更新语法高亮器
    syntax_highlighter_.setFileType(getFileType());
    
    // 打开文件时，总是从文件头部开始显示
    // 如果文件行数少于屏幕高度，所有行（包括最后一行）都会显示
    // 如果文件行数大于屏幕高度，用户可以通过滚动查看所有内容，包括最后一行
    
    setStatusMessage(std::string(ui::icons::OPEN) + " Opened: " + filepath);
    return true;
}

bool Editor::saveFile() {
    Document* doc = getCurrentDocument();
    if (!doc) return false;
    
    // 如果是新文件，需要先指定文件名
    if (doc->getFilePath().empty()) {
        setStatusMessage(std::string(ui::icons::WARNING) + " No file name. Use Alt+A to save as");
        return false;
    }
    
    size_t line_count = doc->lineCount();
    size_t byte_count = 0;
    for (size_t i = 0; i < line_count; ++i) {
        byte_count += doc->getLine(i).length() + 1; // +1 for newline
    }
    
    if (doc->save()) {
        // nano风格：显示写入的行数
        std::string msg = std::string(ui::icons::SAVED) + " Wrote " + 
                         std::to_string(line_count) + " lines (" +
                         std::to_string(byte_count) + " bytes) to " + 
                         doc->getFileName();
        setStatusMessage(msg);
        return true;
    }
    
    // 显示详细错误信息
    std::string error_msg = std::string(ui::icons::ERROR) + " Error: " + doc->getLastError();
    if (error_msg.empty() || doc->getLastError().empty()) {
        error_msg = std::string(ui::icons::ERROR) + " Failed to save file";
    }
    setStatusMessage(error_msg);
    return false;
}

bool Editor::saveFileAs(const std::string& filepath) {
    Document* doc = getCurrentDocument();
    if (!doc) return false;
    
    size_t line_count = doc->lineCount();
    size_t byte_count = 0;
    for (size_t i = 0; i < line_count; ++i) {
        byte_count += doc->getLine(i).length() + 1; // +1 for newline
    }
    
    if (doc->saveAs(filepath)) {
        // 更新语法高亮器（文件类型可能改变）
        syntax_highlighter_.setFileType(getFileType());
        
        // nano风格：显示写入的行数
        std::string msg = std::string(ui::icons::SAVED) + " Wrote " + 
                         std::to_string(line_count) + " lines (" +
                         std::to_string(byte_count) + " bytes) to " + 
                         filepath;
        setStatusMessage(msg);
        return true;
    }
    
    // 显示详细错误信息
    std::string error_msg = std::string(ui::icons::ERROR) + " Error: " + doc->getLastError();
    if (error_msg.empty() || doc->getLastError().empty()) {
        error_msg = std::string(ui::icons::ERROR) + " Failed to save file";
    }
    setStatusMessage(error_msg);
    return false;
}

bool Editor::closeFile() {
    Document* doc = getCurrentDocument();
    if (!doc) return false;
    
    if (doc->isModified()) {
        setStatusMessage("File has unsaved changes. Save first (Ctrl+S)");
        return false;
    }
    closeCurrentTab();
    return true;
}

void Editor::newFile() {
    document_manager_.createNewDocument();
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
    setStatusMessage(std::string(ui::icons::NEW) + " New file created");
}

void Editor::createFolder() {
    // 显示创建文件夹对话框
    show_create_folder_ = true;
    folder_name_input_ = "";
    setStatusMessage("Enter folder name (in current directory: " + file_browser_.getCurrentDirectory() + ")");
}

void Editor::startSaveAs() {
    // 显示另存为对话框
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document to save");
        return;
    }
    
    show_save_as_ = true;
    // 如果有当前文件路径，使用它作为默认值
    if (!doc->getFilePath().empty()) {
        save_as_input_ = doc->getFilePath();
        setStatusMessage("Enter file path to save as");
    } else {
        // 如果没有路径（未命名文件），使用文件浏览器的当前目录
        save_as_input_ = file_browser_.getCurrentDirectory() + "/";
        setStatusMessage("Enter file name to save (new file)");
    }
}

void Editor::quit() {
    Document* doc = getCurrentDocument();
    if (doc && doc->isModified()) {
        setStatusMessage("File modified. Save first (Ctrl+S) or force quit");
        return;
    }
    should_quit_ = true;
    // 立即退出循环，不需要等待下一个事件
    screen_.ExitLoopClosure()();
}

// 标签页管理
void Editor::closeCurrentTab() {
    if (document_manager_.closeCurrentDocument()) {
        setStatusMessage(std::string(ui::icons::CLOSE) + " Tab closed");
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;
    } else {
        setStatusMessage("Cannot close: unsaved changes");
    }
}

void Editor::switchToNextTab() {
    document_manager_.switchToNextDocument();
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
    Document* doc = getCurrentDocument();
    if (doc) {
        setStatusMessage(std::string(ui::icons::FILE) + " " + doc->getFileName());
    }
}

void Editor::switchToPreviousTab() {
    document_manager_.switchToPreviousDocument();
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
    Document* doc = getCurrentDocument();
    if (doc) {
        setStatusMessage(std::string(ui::icons::FILE) + " " + doc->getFileName());
    }
}

void Editor::switchToTab(size_t index) {
    document_manager_.switchToDocument(index);
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
}

} // namespace core
} // namespace pnana

