// 文件操作相关实现
#include "core/editor.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include "utils/text_analyzer.h"
#include <filesystem>
#include <iostream>

namespace pnana {
namespace core {

// 文件操作
bool Editor::openFile(const std::string& filepath) {
    LOG("=== openFile() START ===");
    LOG("Opening file: " + filepath);

    try {
        LOG("Step 1: Opening document...");
        document_manager_.openDocument(filepath);
        LOG("Document opened successfully");

        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0; // 打开文件时，总是从文件头部开始显示
        view_offset_col_ = 0;

        LOG("Step 2: Getting current document...");
        Document* doc = getCurrentDocument();
        if (!doc) {
            LOG_ERROR("getCurrentDocument() returned null!");
            setStatusMessage(std::string(pnana::ui::icons::ERROR) + " Failed to get document");
            return false;
        }

        LOG("Document obtained, line count: " + std::to_string(doc->lineCount()));
        LOG("Step 3: Detecting Chinese content (limited check)...");

        // 检测文件是否包含大量中文字符（排除注释）
        // 限制检测行数和字符数，避免大文件卡住
        bool has_chinese = false;
        try {
            std::string file_type = getFileType();
            LOG("File type: " + file_type);

            // 只检测前50行，避免大文件卡住
            size_t max_check_lines = std::min(doc->lineCount(), static_cast<size_t>(50));
            LOG("Checking first " + std::to_string(max_check_lines) + " lines");

            std::vector<std::string> lines;
            for (size_t i = 0; i < max_check_lines; ++i) {
                lines.push_back(doc->getLine(i));
            }

            LOG("Lines collected, checking Chinese content...");
            // 限制检测字符数为500，加快处理速度
            has_chinese = utils::TextAnalyzer::hasChineseContent(lines, file_type, 500, 10);
            LOG("Chinese detection completed, result: " +
                std::string(has_chinese ? "true" : "false"));
        } catch (const std::exception& e) {
            LOG_WARNING("Chinese detection exception: " + std::string(e.what()));
            has_chinese = false; // 检测失败，默认不禁用语法高亮
        } catch (...) {
            LOG_WARNING("Chinese detection unknown exception");
            has_chinese = false;
        }

        LOG("Step 4: Setting syntax highlighter...");
        // 更新语法高亮器（中文文件禁用语法高亮）
        try {
            if (has_chinese) {
                syntax_highlighting_ = false;
                syntax_highlighter_.setFileType("text");
                LOG("Syntax highlighting disabled (Chinese file)");
            } else {
                std::string file_type = getFileType();
                syntax_highlighter_.setFileType(file_type);
                syntax_highlighting_ = true; // 确保语法高亮开启
                LOG("Syntax highlighting enabled, file type: " + file_type);
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Syntax highlighter exception: " + std::string(e.what()));
        } catch (...) {
            LOG_WARNING("Syntax highlighter unknown exception");
        }

#ifdef BUILD_LSP_SUPPORT
        LOG("Step 5: Updating LSP document...");
        // 通知 LSP 服务器文件已打开
        // 注意：如果 LSP 初始化失败或文件类型不支持，不应该阻塞文件打开
        try {
            updateLspDocument();
            LOG("LSP document updated");
        } catch (const std::exception& e) {
            LOG_WARNING("LSP update failed: " + std::string(e.what()) +
                        " (file will open without LSP features)");
            // LSP 更新失败不影响文件打开
        } catch (...) {
            LOG_WARNING(
                "LSP update failed: Unknown exception (file will open without LSP features)");
            // LSP 更新失败不影响文件打开
        }
#else
        LOG("Step 5: LSP support not compiled");
#endif

        LOG("Step 6: Setting status message...");
        // 使用之前已获取的doc变量
        if (doc) {
            setStatusMessage(std::string(pnana::ui::icons::OPEN) +
                             " Opened: " + doc->getFileName());
            LOG("Status message set: " + doc->getFileName());
        } else {
            LOG_ERROR("getCurrentDocument() returned null after openFile()!");
            setStatusMessage(std::string(pnana::ui::icons::OPEN) + " Opened: " + filepath);
        }

        LOG("=== openFile() SUCCESS ===");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("=== openFile() EXCEPTION ===");
        LOG_ERROR("Exception type: std::exception");
        LOG_ERROR("Exception message: " + std::string(e.what()));
        setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                         " Failed to open file: " + std::string(e.what()));
        return false;
    } catch (...) {
        LOG_ERROR("=== openFile() UNKNOWN EXCEPTION ===");
        setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                         " Failed to open file: Unknown error");
        return false;
    }
}

bool Editor::saveFile() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return false;

    // 如果是新文件，需要先指定文件名
    if (doc->getFilePath().empty()) {
        setStatusMessage(std::string(pnana::ui::icons::WARNING) +
                         " No file name. Use Alt+A to save as");
        return false;
    }

    size_t line_count = doc->lineCount();
    size_t byte_count = 0;
    for (size_t i = 0; i < line_count; ++i) {
        byte_count += doc->getLine(i).length() + 1; // +1 for newline
    }

    if (doc->save()) {
        // nano风格：显示写入的行数
        std::string msg = std::string(pnana::ui::icons::SAVED) + " Wrote " +
                          std::to_string(line_count) + " lines (" + std::to_string(byte_count) +
                          " bytes) to " + doc->getFileName();
        setStatusMessage(msg);
        return true;
    }

    // 显示详细错误信息
    std::string error_msg = std::string(pnana::ui::icons::ERROR) + " Error: " + doc->getLastError();
    if (error_msg.empty() || doc->getLastError().empty()) {
        error_msg = std::string(pnana::ui::icons::ERROR) + " Failed to save file";
    }
    setStatusMessage(error_msg);
    return false;
}

bool Editor::saveFileAs(const std::string& filepath) {
    Document* doc = getCurrentDocument();
    if (!doc)
        return false;

    size_t line_count = doc->lineCount();
    size_t byte_count = 0;
    for (size_t i = 0; i < line_count; ++i) {
        byte_count += doc->getLine(i).length() + 1; // +1 for newline
    }

    if (doc->saveAs(filepath)) {
        // 更新语法高亮器（文件类型可能改变）
        syntax_highlighter_.setFileType(getFileType());

        // 刷新文件浏览器，显示新创建的文件
        file_browser_.refresh();

        // 如果保存的文件在文件浏览器的当前目录中，自动选中它
        try {
            std::filesystem::path saved_path(filepath);
            std::filesystem::path current_dir(file_browser_.getCurrentDirectory());
            std::filesystem::path saved_dir = saved_path.parent_path();

            // 检查保存的文件是否在当前目录中
            if (std::filesystem::equivalent(saved_dir, current_dir)) {
                std::string filename = saved_path.filename().string();
                file_browser_.selectItemByName(filename);
            }
        } catch (...) {
            // 如果路径比较失败，忽略错误，不影响保存操作
        }

        // nano风格：显示写入的行数
        std::string msg = std::string(pnana::ui::icons::SAVED) + " Wrote " +
                          std::to_string(line_count) + " lines (" + std::to_string(byte_count) +
                          " bytes) to " + filepath;
        setStatusMessage(msg);
        return true;
    }

    // 显示详细错误信息
    std::string error_msg = std::string(pnana::ui::icons::ERROR) + " Error: " + doc->getLastError();
    if (error_msg.empty() || doc->getLastError().empty()) {
        error_msg = std::string(pnana::ui::icons::ERROR) + " Failed to save file";
    }
    setStatusMessage(error_msg);
    return false;
}

bool Editor::closeFile() {
    Document* doc = getCurrentDocument();
    if (!doc)
        return false;

#ifdef BUILD_LSP_SUPPORT
    // 通知 LSP 服务器文件已关闭
    if (lsp_enabled_ && lsp_manager_) {
        std::string filepath = doc->getFilePath();
        if (!filepath.empty()) {
            features::LspClient* client = lsp_manager_->getClientForFile(filepath);
            if (client && client->isConnected()) {
                std::string uri = filepathToUri(filepath);
                if (!uri.empty()) {
                    client->didClose(uri);
                    file_language_map_.erase(uri);
                }
            }
        }
    }
    completion_popup_.hide();
#endif

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
    setStatusMessage(std::string(pnana::ui::icons::NEW) + " New file created");
}

void Editor::createFolder() {
    // 显示创建文件夹对话框
    show_create_folder_ = true;
    create_folder_dialog_.setCurrentDirectory(file_browser_.getCurrentDirectory());
    create_folder_dialog_.setInput("");
    setStatusMessage(
        "Enter folder name (in current directory: " + file_browser_.getCurrentDirectory() + ")");
}

void Editor::startSaveAs() {
    // 显示另存为对话框
    Document* doc = getCurrentDocument();
    if (!doc) {
        setStatusMessage("No document to save");
        return;
    }

    show_save_as_ = true;
    save_as_dialog_.setCurrentFileName(doc->getFileName());
    // 输入框只显示文件名或空白，不显示完整路径
    if (!doc->getFilePath().empty()) {
        // 只显示文件名，不显示完整路径
        std::filesystem::path file_path(doc->getFilePath());
        save_as_dialog_.setInput(file_path.filename().string());
        setStatusMessage("Enter file name to save as (current: " + file_path.filename().string() +
                         ")");
    } else {
        // 未命名文件，输入框为空
        save_as_dialog_.setInput("");
        setStatusMessage("Enter file name to save (in: " + file_browser_.getCurrentDirectory() +
                         ")");
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
        setStatusMessage(std::string(pnana::ui::icons::CLOSE) + " Tab closed");
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
        setStatusMessage(std::string(pnana::ui::icons::FILE) + " " + doc->getFileName());
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
        setStatusMessage(std::string(pnana::ui::icons::FILE) + " " + doc->getFileName());
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
