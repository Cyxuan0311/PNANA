// 文件操作相关实现
#include "core/editor.h"
#include "features/ssh/ssh_client.h"
#include "ui/icons.h"
#include "utils/logger.h"
#include "utils/text_analyzer.h"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

// 前向声明辅助函数（在editor_ssh.cpp中定义）
namespace pnana {
namespace core {
bool parseSSHPath(const std::string& path, pnana::ui::SSHConfig& config);
}
} // namespace pnana

namespace {
// 将字节数格式化为 KB/MB/GB，用于大文件确认对话框
std::string formatSize(std::uintmax_t bytes) {
    if (bytes >= 1024ull * 1024 * 1024) {
        unsigned long long gb = static_cast<unsigned long long>(bytes / (1024ull * 1024 * 1024));
        unsigned int rem =
            static_cast<unsigned int>((bytes % (1024ull * 1024 * 1024)) / (1024 * 1024));
        if (rem >= 100)
            return std::to_string(gb) + "." + std::to_string(rem / 100) + " GB";
        return std::to_string(gb) + " GB";
    }
    if (bytes >= 1024 * 1024) {
        unsigned long mb = static_cast<unsigned long>(bytes / (1024 * 1024));
        unsigned int rem = static_cast<unsigned int>((bytes % (1024 * 1024)) / 1024);
        if (rem >= 100)
            return std::to_string(mb) + "." + std::to_string(rem / 100) + " MB";
        return std::to_string(mb) + " MB";
    }
    if (bytes >= 1024)
        return std::to_string(static_cast<unsigned long>(bytes / 1024)) + " KB";
    return std::to_string(static_cast<unsigned long>(bytes)) + " B";
}
} // namespace

namespace pnana {
namespace core {

// 文件操作
bool Editor::openFile(const std::string& filepath) {
    if (filepath.size() >= 6 && filepath.compare(0, 6, "ssh://") == 0) {
        pnana::ui::SSHConfig config;
        if (!parseSSHPath(filepath, config)) {
            setStatusMessage("Invalid SSH path");
            return false;
        }
        if (current_ssh_config_.host == config.host && current_ssh_config_.user == config.user) {
            config.password = current_ssh_config_.password;
            config.key_path = current_ssh_config_.key_path;
        }
        features::ssh::Client ssh_client;
        features::ssh::Result result = ssh_client.readFile(config);
        if (!result.success) {
            setStatusMessage("SSH Error: " + result.error);
            return false;
        }
        size_t doc_index = document_manager_.createNewDocument();
        Document* doc = document_manager_.getDocument(doc_index);
        if (!doc)
            return false;
        doc->setFilePath(filepath);
        std::istringstream iss(result.content);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(iss, line))
            lines.push_back(line);
        if (lines.empty())
            lines.push_back("");
        doc->getLines() = lines;
        doc->setModified(false);
        document_ssh_configs_[doc_index] = config;
        document_manager_.switchToDocument(doc_index);
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;
        syntax_highlighter_.setFileType(getFileType());
        recent_files_manager_.addFile(filepath);
        setStatusMessage(std::string(pnana::ui::icons::OPEN) + " Opened: " + doc->getFileName());
        return true;
    }

    // 本地文件：检查大小，超过阈值时弹出确认对话框
    std::uintmax_t size_bytes = 0;
    try {
        if (std::filesystem::exists(filepath) && !std::filesystem::is_directory(filepath)) {
            size_bytes = std::filesystem::file_size(filepath);
        }
    } catch (...) {
        // 无法获取大小时继续尝试打开
    }

    int limit_mb = config_manager_.getConfig().files.max_file_size_before_prompt_mb;
    if (limit_mb > 0 && size_bytes > static_cast<std::uintmax_t>(limit_mb) * 1024ull * 1024ull) {
        std::string msg = "File is about " + formatSize(size_bytes) +
                          ". Opening may be slow and use a lot of memory. Open anyway?";
        dialog_.showConfirm(
            "Large File", msg,
            [this, filepath]() {
                openFileInternal(filepath);
                file_browser_.setVisible(false);
                region_manager_.setRegion(EditorRegion::CODE_AREA);
            },
            [this]() {
                setStatusMessage("Canceled opening large file");
            });
        return true;
    }

    return openFileInternal(filepath);
}

bool Editor::openFileInternal(const std::string& filepath) {
    auto open_t0 = std::chrono::high_resolution_clock::now();

    try {
        document_manager_.openDocument(filepath);

        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - open_t0)
                          .count();
            LOG("[perf] openFileInternal openDocument us=" + std::to_string(us));
        }

        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;

        Document* doc = getCurrentDocument();
        if (!doc) {
            LOG_ERROR("getCurrentDocument() returned null!");
            setStatusMessage(std::string(pnana::ui::icons::ERROR) + " Failed to get document");
            return false;
        }

        // 大文件策略：根据文件大小禁用语法高亮与 LSP，减轻首帧卡顿（阶段二）
        std::uintmax_t size_bytes = 0;
        try {
            if (std::filesystem::exists(filepath) && !std::filesystem::is_directory(filepath)) {
                size_bytes = std::filesystem::file_size(filepath);
            }
        } catch (...) {
        }
        const std::uintmax_t large_threshold = 50ull * 1024 * 1024; // 50 MB
        bool is_large_file = (size_bytes > large_threshold);

        bool has_chinese = false;
        std::string file_type;
        if (!is_large_file) {
            try {
                file_type = getFileType();
                size_t max_check_lines = std::min(doc->lineCount(), static_cast<size_t>(50));
                std::vector<std::string> lines;
                for (size_t i = 0; i < max_check_lines; ++i) {
                    lines.push_back(doc->getLine(i));
                }
                has_chinese = utils::TextAnalyzer::hasChineseContent(lines, file_type, 500, 10);
            } catch (const std::exception& e) {
                LOG_WARNING("Chinese detection exception: " + std::string(e.what()));
                has_chinese = false;
            } catch (...) {
                LOG_WARNING("Chinese detection unknown exception");
                has_chinese = false;
            }
        }

        try {
            if (is_large_file) {
                syntax_highlighting_ = false;
                syntax_highlighter_.setFileType("text");
            } else if (has_chinese && file_type != "markdown") {
                syntax_highlighting_ = false;
                syntax_highlighter_.setFileType("text");
            } else {
                if (file_type.empty())
                    file_type = getFileType();
                syntax_highlighter_.setFileType(file_type);
                syntax_highlighting_ = true;
            }
        } catch (const std::exception& e) {
            LOG_WARNING("Syntax highlighter exception: " + std::string(e.what()));
        } catch (...) {
            LOG_WARNING("Syntax highlighter unknown exception");
        }

        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - open_t0)
                          .count();
            LOG("[perf] openFileInternal after_highlight us=" + std::to_string(us));
        }

#ifdef BUILD_LUA_SUPPORT
        triggerPluginEvent("FileOpened", {filepath});
        triggerPluginEvent("BufEnter", {filepath});
#endif

#ifdef BUILD_LSP_SUPPORT
        if (!is_large_file) {
            try {
                static int file_open_count = 0;
                if (++file_open_count % 10 == 0) {
                    cleanupExpiredCaches();
                }
                updateCurrentFileDiagnostics();
                updateCurrentFileFolding();
                updateLspDocument();
            } catch (const std::exception& e) {
                LOG_WARNING("LSP update failed: " + std::string(e.what()) +
                            " (file will open without LSP features)");
            } catch (...) {
                LOG_WARNING(
                    "LSP update failed: Unknown exception (file will open without LSP features)");
            }
        }
#endif

        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                          std::chrono::high_resolution_clock::now() - open_t0)
                          .count();
            LOG("[perf] openFileInternal after_lsp us=" + std::to_string(us));
        }

        if (doc) {
            setStatusMessage(std::string(pnana::ui::icons::OPEN) +
                             " Opened: " + doc->getFileName());
        } else {
            setStatusMessage(std::string(pnana::ui::icons::OPEN) + " Opened: " + filepath);
        }

        recent_files_manager_.addFile(filepath);

        if (split_view_manager_.hasSplits()) {
            size_t new_doc_index = document_manager_.getCurrentIndex();
            size_t active_region_index = split_view_manager_.getActiveRegionIndex();
            const auto* active_region = split_view_manager_.getActiveRegion();

            if (active_region && active_region->current_document_index == SIZE_MAX) {
                split_view_manager_.setDocumentIndexForRegion(active_region_index, new_doc_index);
                if (region_states_.size() <= active_region_index) {
                    region_states_.resize(active_region_index + 1);
                }
                region_states_[active_region_index].cursor_row = 0;
                region_states_[active_region_index].cursor_col = 0;
                region_states_[active_region_index].view_offset_row = 0;
                region_states_[active_region_index].view_offset_col = 0;
            } else {
                split_view_manager_.addDocumentIndexToRegion(active_region_index, new_doc_index);
                split_view_manager_.setDocumentIndexForRegion(active_region_index, new_doc_index);
            }
        }

        if (pnana::utils::Logger::getInstance().isEnabled()) {
            auto total_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                std::chrono::high_resolution_clock::now() - open_t0)
                                .count();
            LOG("[perf] openFileInternal total us=" + std::to_string(total_us));
        }
        return true;
    } catch (const std::exception& e) {
        setStatusMessage(std::string(pnana::ui::icons::ERROR) +
                         " Failed to open file: " + std::string(e.what()));
        return false;
    } catch (...) {
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

    // 检查是否为SSH文件
    std::string filepath = doc->getFilePath();
    if (filepath.find("ssh://") == 0) {
        // SSH文件保存
        size_t doc_index = document_manager_.getCurrentIndex();
        auto it = document_ssh_configs_.find(doc_index);
        if (it != document_ssh_configs_.end()) {
            // 使用关联的SSH配置保存
            return saveSSHFile(doc, it->second);
        } else {
            // 尝试从路径解析SSH配置
            pnana::ui::SSHConfig config;
            if (parseSSHPath(filepath, config)) {
                // 需要密码或密钥，使用当前配置
                if (current_ssh_config_.host == config.host &&
                    current_ssh_config_.user == config.user) {
                    config.password = current_ssh_config_.password;
                    config.key_path = current_ssh_config_.key_path;
                    return saveSSHFile(doc, config);
                } else {
                    setStatusMessage("SSH: Authentication required. Please reconnect.");
                    return false;
                }
            } else {
                setStatusMessage("SSH: Invalid SSH path format");
                return false;
            }
        }
    }

    // 普通文件保存
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

#ifdef BUILD_LUA_SUPPORT
        // 触发文件保存事件
        triggerPluginEvent("FileSaved", {filepath});
        triggerPluginEvent("BufWrite", {filepath});
#endif

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

    // 检查是否为SSH路径
    if (filepath.find("ssh://") == 0) {
        // SSH文件保存
        pnana::ui::SSHConfig config;
        if (parseSSHPath(filepath, config)) {
            // 使用当前SSH配置的认证信息（如果可用）
            size_t doc_index = document_manager_.getCurrentIndex();
            auto it = document_ssh_configs_.find(doc_index);
            if (it != document_ssh_configs_.end() && it->second.host == config.host &&
                it->second.user == config.user) {
                config.password = it->second.password;
                config.key_path = it->second.key_path;
            } else if (current_ssh_config_.host == config.host &&
                       current_ssh_config_.user == config.user) {
                config.password = current_ssh_config_.password;
                config.key_path = current_ssh_config_.key_path;
            } else {
                setStatusMessage("SSH: Authentication required. Please reconnect.");
                return false;
            }
            bool result = saveSSHFile(doc, config, filepath);
            if (result) {
                // 更新SSH配置映射
                document_ssh_configs_[doc_index] = config;
            }
            return result;
        } else {
            setStatusMessage("SSH: Invalid SSH path format");
            return false;
        }
    }

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

    // 如果在分屏模式下，将新文档设置为当前激活区域的文档
    if (split_view_manager_.hasSplits()) {
        size_t new_doc_index = document_manager_.getCurrentIndex();
        size_t active_region_index = split_view_manager_.getActiveRegionIndex();
        const auto* active_region = split_view_manager_.getActiveRegion();

        if (active_region && active_region->current_document_index == SIZE_MAX) {
            // 如果当前区域显示欢迎页面，直接设置新文档为该区域的当前文档
            split_view_manager_.setDocumentIndexForRegion(active_region_index, new_doc_index);

            // 确保该区域有正确的状态
            if (region_states_.size() <= active_region_index) {
                region_states_.resize(active_region_index + 1);
            }
            // 初始化新区域的状态：新打开的文件从开头开始
            region_states_[active_region_index].cursor_row = 0;
            region_states_[active_region_index].cursor_col = 0;
            region_states_[active_region_index].view_offset_row = 0;
            region_states_[active_region_index].view_offset_col = 0;
        } else {
            // 否则添加到文档列表中
            split_view_manager_.addDocumentIndexToRegion(active_region_index, new_doc_index);
            // 更新当前激活区域显示这个新文档
            split_view_manager_.setDocumentIndexForRegion(active_region_index, new_doc_index);
        }
    }

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

void Editor::startMoveFile() {
    // 显示移动文件对话框
    if (!file_browser_.hasSelection()) {
        setStatusMessage("No file or folder selected");
        return;
    }

    std::string selected_path = file_browser_.getSelectedPath();
    std::string current_dir = file_browser_.getCurrentDirectory();

    show_move_file_ = true;
    move_file_dialog_.setSourcePath(selected_path);
    move_file_dialog_.setTargetDirectory(current_dir);
    move_file_dialog_.setInput(""); // 初始为空，用户输入目标路径
    setStatusMessage("Enter target path to move: " + file_browser_.getSelectedName());
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
    // 在分屏模式下，只在当前激活区域的文档列表中切换
    if (split_view_manager_.hasSplits()) {
        const auto* active_region = split_view_manager_.getActiveRegion();
        if (active_region) {
            const auto& region_docs = active_region->document_indices;
            if (!region_docs.empty()) {
                // 找到当前文档在区域文档列表中的位置
                auto it = std::find(region_docs.begin(), region_docs.end(),
                                    active_region->current_document_index);
                size_t current_pos =
                    (it != region_docs.end()) ? std::distance(region_docs.begin(), it) : 0;

                // 计算下一个文档的位置
                size_t next_pos = (current_pos + 1) % region_docs.size();
                size_t next_doc_index = region_docs[next_pos];

                // 更新当前区域的文档索引（这会改变该区域显示的文档）
                split_view_manager_.setDocumentIndexForRegion(
                    split_view_manager_.getActiveRegionIndex(), next_doc_index);

                // 更新全局文档管理器到新文档（因为这是激活区域）
                document_manager_.switchToDocument(next_doc_index);
            }
        }
    } else {
        // 单视图模式下的传统行为
        document_manager_.switchToNextDocument();
    }

    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
    Document* doc = getCurrentDocument();
    if (doc) {
        setStatusMessage(std::string(pnana::ui::icons::FILE) + " " + doc->getFileName());
    }

    // 强制UI更新，确保标签栏立即刷新
    force_ui_update_ = true;
}

void Editor::switchToPreviousTab() {
    // 在分屏模式下，只在当前激活区域的文档列表中切换
    if (split_view_manager_.hasSplits()) {
        const auto* active_region = split_view_manager_.getActiveRegion();
        if (active_region) {
            const auto& region_docs = active_region->document_indices;
            if (!region_docs.empty()) {
                // 找到当前文档在区域文档列表中的位置
                auto it = std::find(region_docs.begin(), region_docs.end(),
                                    active_region->current_document_index);
                size_t current_pos =
                    (it != region_docs.end()) ? std::distance(region_docs.begin(), it) : 0;

                // 计算上一个文档的位置
                size_t prev_pos = (current_pos == 0) ? (region_docs.size() - 1) : (current_pos - 1);
                size_t prev_doc_index = region_docs[prev_pos];

                // 更新当前区域的文档索引（这会改变该区域显示的文档）
                split_view_manager_.setDocumentIndexForRegion(
                    split_view_manager_.getActiveRegionIndex(), prev_doc_index);

                // 更新全局文档管理器到新文档（因为这是激活区域）
                document_manager_.switchToDocument(prev_doc_index);
            }
        }
    } else {
        // 单视图模式下的传统行为
        document_manager_.switchToPreviousDocument();
    }

    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;
    Document* doc = getCurrentDocument();
    if (doc) {
        setStatusMessage(std::string(pnana::ui::icons::FILE) + " " + doc->getFileName());
    }

    // 强制UI更新，确保标签栏立即刷新
    force_ui_update_ = true;
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
