#include "features/lsp/lsp_formatter.h"
#include "features/lsp/lsp_client.h"
#include "features/lsp/lsp_server_manager.h"
#include "utils/file_type_detector.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <thread>

namespace pnana {
namespace features {

namespace fs = std::filesystem;

LspFormatter::LspFormatter(LspServerManager* lsp_manager) : lsp_manager_(lsp_manager) {}

LspFormatter::~LspFormatter() = default;

std::vector<std::string> LspFormatter::getSupportedFilesInDirectory(
    const std::string& directory_path) {
    std::vector<std::string> supported_files;

    try {
        if (!fs::exists(directory_path) || !fs::is_directory(directory_path)) {
            LOG_WARNING("Directory does not exist or is not a directory: " + directory_path);
            return supported_files;
        }

        // 遍历目录中的所有文件（递归）
        for (const auto& entry : fs::recursive_directory_iterator(
                 directory_path, fs::directory_options::skip_permission_denied)) {
            if (entry.is_regular_file()) {
                std::string file_path = entry.path().string();

                // 跳过不应扫描的目录
                bool should_skip = false;
                for (const auto& part : entry.path()) {
                    std::string part_str = part.string();
                    if (part_str == ".git" || part_str == "build" || part_str == ".vscode" ||
                        part_str == ".idea" || part_str == "node_modules" || part_str == ".next" ||
                        part_str == "target" || part_str == "dist" || part_str == "out") {
                        should_skip = true;
                        break;
                    }
                }

                if (should_skip) {
                    // Skip excluded directories
                    continue;
                }

                std::string extension = entry.path().extension().string();

                // 确保扩展名不包含点号
                if (!extension.empty() && extension[0] == '.') {
                    extension = extension.substr(1);
                }

                std::string file_type =
                    utils::FileTypeDetector::detectFileType(file_path, extension);

                // 检查文件类型是否被 LSP 服务器支持
                if (isFileTypeSupported(file_type)) {
                    supported_files.push_back(file_path);
                }
            }
        }

        // Scan completed

        // 按文件名排序
        std::sort(supported_files.begin(), supported_files.end());

    } catch (const std::exception& e) {
        LOG_ERROR("Error scanning directory for supported files: " + std::string(e.what()));
    }

    return supported_files;
}

bool LspFormatter::isFileTypeSupported(const std::string& file_type) const {
    // 常见的可格式化文件类型
    static const std::vector<std::string> supported_types = {
        "cpp",        "c",     "cxx",        "cc",   "h",     "hpp",  "hxx",  "python", "py",
        "javascript", "js",    "typescript", "ts",   "java",  "go",   "rust", "rs",     "json",
        "xml",        "html",  "css",        "yaml", "yml",   "toml", "lua",  "php",    "ruby",
        "rb",         "swift", "kotlin",     "kt",   "scala", "dart", "r",    "matlab", "shell",
        "bash",       "sh",    "zsh",        "perl", "pl",    "tcl",  "sql"};

    return std::find(supported_types.begin(), supported_types.end(), file_type) !=
           supported_types.end();
}

bool LspFormatter::formatFile(const std::string& file_path) {
    std::string original_content;

    // Test basic file operations
    try {
        std::ifstream test_file(file_path, std::ios::binary);
        if (!test_file.is_open()) {
            LOG_ERROR("Cannot open file for reading: " + file_path);
            return false;
        }
        original_content.assign((std::istreambuf_iterator<char>(test_file)),
                                std::istreambuf_iterator<char>());
        test_file.close();

    } catch (const std::exception& e) {
        LOG_ERROR("File operation test failed: " + std::string(e.what()));
        return false;
    }

    if (!lsp_manager_) {
        LOG_ERROR("LSP manager not available");
        return false;
    }

    // Get LSP client for file
    LspClient* lsp_client = lsp_manager_->getClientForFile(file_path);

    if (!lsp_client) {
        LOG_ERROR("No LSP client available for file: " + file_path + " (server not configured)");
        // Try fallback formatting
        std::string fallback_result = tryCommandLineFormat(file_path, original_content);
        if (!fallback_result.empty() && fallback_result != original_content) {
            std::ofstream out_file(file_path, std::ios::binary);
            if (out_file.is_open()) {
                out_file.write(fallback_result.c_str(), fallback_result.length());
                out_file.close();
                return true;
            } else {
                LOG_ERROR("Failed to write fallback formatted content");
            }
        }
        return false;
    }

    // Ensure client is initialized and connected
    if (!lsp_client->isConnected()) {
        try {
            std::string root_path = std::filesystem::current_path().string();

            // Initialize LSP client (blocking until complete or fail)
            bool init_success = lsp_client->initialize(root_path);

            if (!init_success) {
                LOG_ERROR("LSP client initialization failed for file: " + file_path +
                          ", trying fallback formatting");
                // 尝试备用格式化
                std::string fallback_result = tryCommandLineFormat(file_path, original_content);
                if (!fallback_result.empty() && fallback_result != original_content) {
                    LOG("LspFormatter: Writing fallback formatted content to file...");
                    std::ofstream out_file(file_path, std::ios::binary);
                    if (out_file.is_open()) {
                        out_file.write(fallback_result.c_str(), fallback_result.length());
                        out_file.close();
                        LOG("LspFormatter: Fallback formatting successful");
                        return true;
                    } else {
                        LOG_ERROR("LspFormatter: Failed to write fallback formatted content");
                    }
                }
                return false;
            }

            // 等待客户端连接，最多等待10秒（clangd初始化可能较慢）
            int retry_count = 0;
            const int max_retries = 150; // 150 * 100ms = 15秒
            while (!lsp_client->isConnected() && retry_count < max_retries) {
                LOG("LspFormatter: Waiting for LSP client connection... (attempt " +
                    std::to_string(retry_count + 1) + "/" + std::to_string(max_retries) + ")");
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                retry_count++;
            }

            if (!lsp_client->isConnected()) {
                LOG_ERROR("LSP client failed to connect after 3 seconds for file: " + file_path +
                          ", trying fallback");
                // 尝试备用格式化
                std::string fallback_result = tryCommandLineFormat(file_path, original_content);
                if (!fallback_result.empty() && fallback_result != original_content) {
                    LOG("LspFormatter: Writing fallback formatted content to file...");
                    std::ofstream out_file(file_path, std::ios::binary);
                    if (out_file.is_open()) {
                        out_file.write(fallback_result.c_str(), fallback_result.length());
                        out_file.close();
                        LOG("LspFormatter: Fallback formatting successful");
                        return true;
                    }
                }
                return false;
            }

            LOG("LspFormatter: LSP client is now connected and ready");
        } catch (const std::exception& e) {
            LOG_ERROR("LSP client initialization exception: " + std::string(e.what()) +
                      ", trying fallback");
            // 尝试备用格式化
            std::string fallback_result = tryCommandLineFormat(file_path, original_content);
            if (!fallback_result.empty() && fallback_result != original_content) {
                std::ofstream out_file(file_path, std::ios::binary);
                if (out_file.is_open()) {
                    out_file.write(fallback_result.c_str(), fallback_result.length());
                    out_file.close();
                    LOG("LspFormatter: Fallback formatting successful after LSP exception");
                    return true;
                }
            }
            return false;
        }
    }

    try {
        // 读取原始文件内容
        std::ifstream file(file_path, std::ios::binary);
        if (!file.is_open()) {
            LOG_ERROR("Cannot open file for reading: " + file_path);
            return false;
        }

        std::string original_content((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());
        file.close();

        LOG("LspFormatter: Successfully read file, content length: " +
            std::to_string(original_content.length()));

        // 转换为 URI (使用正确的URL编码)
        std::string uri = filepathToUri(file_path);

        // 确保文件已在 LSP 服务器中打开
        std::string file_type = utils::FileTypeDetector::detectFileType(
            file_path, fs::path(file_path).extension().string());
        LOG("LspFormatter: Detected file type: " + file_type + " for " + file_path);

        // 发送didOpen通知
        try {
            lsp_client->didOpen(uri, file_type, original_content);
            LOG("LspFormatter: Sent didOpen to LSP server");
        } catch (const std::exception& e) {
            LOG_WARNING("LspFormatter: Failed to send didOpen: " + std::string(e.what()));
            // didOpen失败不应该阻止格式化，继续尝试
        }

        // 请求格式化
        LOG("LspFormatter: Sending formatDocument request for: " + uri);
        std::string formatted_content;
        try {
            formatted_content = lsp_client->formatDocument(uri, original_content);
            LOG("LspFormatter: Received formatted content, length: " +
                std::to_string(formatted_content.length()));
        } catch (const std::exception& e) {
            LOG_ERROR("LspFormatter: formatDocument failed: " + std::string(e.what()));
            return false;
        }

        if (formatted_content.empty()) {
            LOG_WARNING("LSP server returned empty content for file: " + file_path +
                        ", trying fallback formatting...");

            // 尝试使用命令行工具作为备选方案
            formatted_content = tryCommandLineFormat(file_path, original_content);
            if (formatted_content.empty() || formatted_content == original_content) {
                LOG_WARNING("All formatting methods failed for file: " + file_path +
                            " (server may not support formatting or file already formatted)");
                return false;
            }

            LOG("LspFormatter: Successfully formatted using command line fallback");
        }

        if (formatted_content == original_content) {
            LOG("LspFormatter: File already properly formatted, no changes needed");
            return true;
        }

        LOG("LspFormatter: Formatting successful, content changed");

        // 写入格式化后的内容
        LOG("LspFormatter: Writing formatted content to file: " + file_path);
        std::ofstream out_file(file_path, std::ios::binary);
        if (!out_file.is_open()) {
            LOG_ERROR("Cannot open file for writing: " + file_path);
            return false;
        }

        out_file.write(formatted_content.c_str(), formatted_content.length());
        out_file.close();
        LOG("LspFormatter: File write completed successfully");

        // 通知LSP服务器文件内容已更改
        try {
            lsp_client->didChange(uri, formatted_content);
            LOG("LspFormatter: Notified LSP server of file change");
        } catch (const std::exception& e) {
            LOG_WARNING("LspFormatter: Failed to notify LSP server of file change: " +
                        std::string(e.what()));
            // 这不是致命错误，继续执行
        }

        LOG("Successfully formatted file: " + file_path);
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Error formatting file " + file_path + ": " + std::string(e.what()));
        return false;
    }
}

bool LspFormatter::formatFiles(const std::vector<std::string>& file_paths) {
    if (!lsp_manager_) {
        LOG_ERROR("LSP manager not available");
        return false;
    }

    bool all_success = true;
    for (const auto& file_path : file_paths) {
        if (!formatFile(file_path)) {
            all_success = false;
        }
    }

    return all_success;
}

std::string LspFormatter::tryCommandLineFormat(const std::string& file_path,
                                               const std::string& /* original_content */) {
    // 检测文件类型并尝试相应的命令行格式化工具
    std::string extension = fs::path(file_path).extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    // 对于C/C++文件，尝试clang-format
    if (extension == ".cpp" || extension == ".hpp" || extension == ".cc" || extension == ".h") {
        return tryClangFormat(file_path);
    }

    // 对于其他文件类型，可以在这里添加更多的命令行工具支持
    // 比如Python的black, JavaScript的prettier等

    return ""; // 没有可用的命令行工具
}

std::string LspFormatter::tryClangFormat(const std::string& file_path) {
    // 检查clang-format是否可用
    int result = system("which clang-format > /dev/null 2>&1");
    if (result != 0) {
        LOG("LspFormatter: clang-format not found, skipping command line fallback");
        return "";
    }

    try {
        // 使用clang-format格式化文件
        std::string command = "clang-format \"" + file_path + "\"";
        LOG("LspFormatter: Running command line formatter: " + command);

        // 执行命令并捕获输出
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            LOG_ERROR("LspFormatter: Failed to run clang-format command");
            return "";
        }

        std::string formatted_content;
        char buffer[4096];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            formatted_content += buffer;
        }

        int status = pclose(pipe);
        if (status != 0) {
            LOG_WARNING("LspFormatter: clang-format command failed with status: " +
                        std::to_string(status));
            return "";
        }

        if (formatted_content.empty()) {
            LOG_WARNING("LspFormatter: clang-format returned empty content");
            return "";
        }

        LOG("LspFormatter: clang-format completed successfully, output length: " +
            std::to_string(formatted_content.length()));
        return formatted_content;

    } catch (const std::exception& e) {
        LOG_ERROR("LspFormatter: Exception in clang-format: " + std::string(e.what()));
        return "";
    }
}

std::string LspFormatter::getFileDisplayName(const std::string& file_path) const {
    try {
        fs::path path(file_path);
        return path.filename().string();
    } catch (...) {
        // 如果路径解析失败，返回完整路径
        return file_path;
    }
}

std::string LspFormatter::getFileRelativePath(const std::string& file_path,
                                              const std::string& base_path) const {
    try {
        fs::path full_path(file_path);
        fs::path base(base_path);

        // 获取相对路径
        fs::path relative_path = fs::relative(full_path, base);
        return relative_path.string();
    } catch (...) {
        // 如果相对路径计算失败，返回文件名
        return getFileDisplayName(file_path);
    }
}

std::string LspFormatter::filepathToUri(const std::string& filepath) const {
    // 使用与lsp_client相同的URI构造逻辑
    std::string uri = "file://";

    try {
        // 使用filesystem库获取绝对路径并规范化
        fs::path abs_path = fs::absolute(filepath);
        std::string path = abs_path.string();

        // 替换反斜杠为正斜杠（Windows兼容性）
        std::replace(path.begin(), path.end(), '\\', '/');

        // URL编码
        for (char c : path) {
            if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':') {
                uri += c;
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
                uri += hex;
            }
        }
    } catch (const std::exception& e) {
        // 如果绝对路径失败，使用原始路径
        LOG_WARNING("Failed to get absolute path for URI conversion: " + std::string(e.what()));
        std::string path = filepath;
        std::replace(path.begin(), path.end(), '\\', '/');

        for (char c : path) {
            if (std::isalnum(c) || c == '/' || c == '-' || c == '_' || c == '.' || c == ':') {
                uri += c;
            } else {
                char hex[4];
                snprintf(hex, sizeof(hex), "%%%02X", static_cast<unsigned char>(c));
                uri += hex;
            }
        }
    }

    return uri;
}

} // namespace features
} // namespace pnana
