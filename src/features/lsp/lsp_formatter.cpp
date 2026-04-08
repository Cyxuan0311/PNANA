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
    const auto format_start = std::chrono::steady_clock::now();

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
                const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - format_start);
                (void)elapsed_ms;
                return true;
            } else {
                LOG_ERROR("Failed to write fallback formatted content");
            }
        }
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - format_start);
        LOG_WARNING("[FormatFlow] formatFile failed(no-client), elapsed_ms=" +
                    std::to_string(elapsed_ms.count()));
        return false;
    }

    // Ensure client is initialized and connected
    if (!lsp_client->isConnected()) {
        LOG_DEBUG("[FormatFlow] client not connected, initializing...");
        try {
            std::string root_path = std::filesystem::current_path().string();

            // Initialize LSP client (blocking until complete or fail)
            bool init_success = lsp_manager_->initializeClientForFile(file_path, root_path);
            LOG_DEBUG(std::string("[FormatFlow] initializeClientForFile result=") +
                      (init_success ? "true" : "false"));

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
            LOG_DEBUG("[FormatFlow] client connected after init wait");
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
        LOG_DEBUG("[FormatFlow] uri=" + uri);

        // 确保文件已在 LSP 服务器中打开
        std::string language_id = detectLanguageIdForLsp(file_path);
        LOG("LspFormatter: Detected LSP language id: " + language_id + " for " + file_path);

        // 发送didOpen通知
        try {
            lsp_client->didOpen(uri, language_id, original_content);
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
            LOG_DEBUG("[FormatFlow] formatDocument completed, output_bytes=" +
                      std::to_string(formatted_content.size()));
        } catch (const std::exception& e) {
            LOG_ERROR("LspFormatter: formatDocument failed: " + std::string(e.what()));
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - format_start);
            LOG_ERROR("[FormatFlow] formatFile failed at formatDocument, elapsed_ms=" +
                      std::to_string(elapsed_ms.count()));
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
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - format_start);
            LOG_DEBUG("[FormatFlow] formatFile no-op, elapsed_ms=" +
                      std::to_string(elapsed_ms.count()));
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
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - format_start);
        LOG_DEBUG("[FormatFlow] formatFile success(lsp), elapsed_ms=" +
                  std::to_string(elapsed_ms.count()));
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("Error formatting file " + file_path + ": " + std::string(e.what()));
        return false;
    }
}

bool LspFormatter::formatFiles(const std::vector<std::string>& file_paths) {
    const auto batch_start = std::chrono::steady_clock::now();
    LOG_DEBUG("[FormatFlow] formatFiles start, count=" + std::to_string(file_paths.size()));

    if (!lsp_manager_) {
        LOG_ERROR("LSP manager not available");
        return false;
    }

    bool all_success = true;
    size_t success_count = 0;
    size_t failed_count = 0;
    for (const auto& file_path : file_paths) {
        LOG_DEBUG("[FormatFlow] formatFiles item begin: " + file_path);
        if (!formatFile(file_path)) {
            all_success = false;
            failed_count++;
            LOG_WARNING("[FormatFlow] formatFiles item failed: " + file_path);
        } else {
            success_count++;
            LOG_DEBUG("[FormatFlow] formatFiles item success: " + file_path);
        }
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - batch_start);
    LOG_DEBUG("[FormatFlow] formatFiles end, success=" + std::to_string(success_count) +
              ", failed=" + std::to_string(failed_count) +
              ", elapsed_ms=" + std::to_string(elapsed_ms.count()));

    return all_success;
}

std::string LspFormatter::tryCommandLineFormat(const std::string& file_path,
                                               const std::string& original_content) {
    const std::string extension = normalizeExtension(fs::path(file_path).extension().string());
    LOG_DEBUG("[FormatFlow] tryCommandLineFormat: ext='" + extension + "', file='" + file_path +
              "'");

    if (extension == "c" || extension == "cpp" || extension == "cxx" || extension == "cc" ||
        extension == "h" || extension == "hpp" || extension == "hxx" || extension == "hh") {
        return tryClangFormat(file_path, original_content);
    }

    if (extension == "py" || extension == "pyw" || extension == "pyi") {
        return tryBlackFormat(file_path, original_content);
    }

    if (extension == "js" || extension == "jsx" || extension == "mjs" || extension == "cjs" ||
        extension == "ts" || extension == "tsx" || extension == "json" || extension == "jsonc" ||
        extension == "css" || extension == "scss" || extension == "sass" || extension == "md" ||
        extension == "markdown") {
        return tryPrettierFormat(file_path, original_content);
    }

    if (extension == "go") {
        return tryGoFmt(file_path, original_content);
    }

    if (extension == "rs") {
        return tryRustFmt(file_path, original_content);
    }

    if (extension == "sh" || extension == "bash" || extension == "zsh" || extension == "shell") {
        return tryShFmt(file_path, original_content);
    }

    LOG_DEBUG("[FormatFlow] no command-line fallback registered for extension='" + extension + "'");
    return "";
}

std::string LspFormatter::normalizeExtension(const std::string& extension) const {
    std::string normalized = extension;
    if (!normalized.empty() && normalized.front() == '.') {
        normalized.erase(0, 1);
    }
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    return normalized;
}

std::string LspFormatter::detectLanguageIdForLsp(const std::string& file_path) const {
    const std::string filename = fs::path(file_path).filename().string();
    const std::string extension = normalizeExtension(fs::path(file_path).extension().string());
    const std::string detected = utils::FileTypeDetector::detectFileType(filename, extension);

    if (detected == "cpp" || detected == "cxx" || detected == "cc" || detected == "hpp" ||
        detected == "hxx" || detected == "hh") {
        return "cpp";
    }
    if (detected == "c") {
        return "c";
    }
    if (detected == "python" || detected == "py") {
        return "python";
    }
    if (detected == "javascript" || detected == "js") {
        return "javascript";
    }
    if (detected == "typescript" || detected == "ts") {
        return "typescript";
    }

    return detected;
}

std::string LspFormatter::runFormatterCommand(const std::string& command,
                                              const std::string& original_content,
                                              const std::string& formatter_name,
                                              const std::string& file_path) {
    const auto fallback_start = std::chrono::steady_clock::now();
    LOG_DEBUG("[FormatFlow] runFormatterCommand start, formatter='" + formatter_name + "', file='" +
              file_path + "'");

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        LOG_ERROR("[FormatFlow] failed to run formatter='" + formatter_name + "'");
        return "";
    }

    std::string formatted_content;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        formatted_content += buffer;
    }

    int status = pclose(pipe);
    if (status != 0) {
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - fallback_start);
        LOG_WARNING("[FormatFlow] formatter failed, formatter='" + formatter_name + "', status=" +
                    std::to_string(status) + ", elapsed_ms=" + std::to_string(elapsed_ms.count()));
        return "";
    }

    if (formatted_content.empty()) {
        const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - fallback_start);
        LOG_WARNING("[FormatFlow] formatter returned empty output, formatter='" + formatter_name +
                    "', elapsed_ms=" + std::to_string(elapsed_ms.count()));
        return "";
    }

    if (formatted_content == original_content) {
        LOG_DEBUG("[FormatFlow] formatter no-op output, formatter='" + formatter_name + "'");
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - fallback_start);
    LOG_DEBUG("[FormatFlow] formatter success, formatter='" + formatter_name +
              "', output_bytes=" + std::to_string(formatted_content.size()) +
              ", elapsed_ms=" + std::to_string(elapsed_ms.count()));
    return formatted_content;
}

std::string LspFormatter::tryClangFormat(const std::string& file_path,
                                         const std::string& original_content) {
    if (!isToolAvailable("clang-format")) {
        LOG_WARNING("[FormatFlow] tryClangFormat unavailable: clang-format not found");
        return "";
    }

    std::string command = "clang-format \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "clang-format", file_path);
}

std::string LspFormatter::tryBlackFormat(const std::string& file_path,
                                         const std::string& original_content) {
    if (!isToolAvailable("black")) {
        LOG_WARNING("[FormatFlow] tryBlackFormat unavailable: black not found");
        return "";
    }

    std::string command = "black --quiet - < \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "black", file_path);
}

std::string LspFormatter::tryPrettierFormat(const std::string& file_path,
                                            const std::string& original_content) {
    if (!isToolAvailable("prettier")) {
        LOG_WARNING("[FormatFlow] tryPrettierFormat unavailable: prettier not found");
        return "";
    }

    std::string command = "prettier --stdin-filepath \"" + file_path + "\" < \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "prettier", file_path);
}

std::string LspFormatter::tryGoFmt(const std::string& file_path,
                                   const std::string& original_content) {
    if (!isToolAvailable("gofmt")) {
        LOG_WARNING("[FormatFlow] tryGoFmt unavailable: gofmt not found");
        return "";
    }

    std::string command = "gofmt \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "gofmt", file_path);
}

std::string LspFormatter::tryRustFmt(const std::string& file_path,
                                     const std::string& original_content) {
    if (!isToolAvailable("rustfmt")) {
        LOG_WARNING("[FormatFlow] tryRustFmt unavailable: rustfmt not found");
        return "";
    }

    std::string command = "rustfmt --emit stdout \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "rustfmt", file_path);
}

std::string LspFormatter::tryShFmt(const std::string& file_path,
                                   const std::string& original_content) {
    if (!isToolAvailable("shfmt")) {
        LOG_WARNING("[FormatFlow] tryShFmt unavailable: shfmt not found");
        return "";
    }

    std::string command = "shfmt \"" + file_path + "\"";
    return runFormatterCommand(command, original_content, "shfmt", file_path);
}

bool LspFormatter::isToolAvailable(const std::string& tool_name) {
    const auto it = tool_availability_cache_.find(tool_name);
    if (it != tool_availability_cache_.end()) {
        LOG_DEBUG("[FormatFlow] tool cache hit: " + tool_name + "=" +
                  (it->second ? std::string("true") : std::string("false")));
        return it->second;
    }

    const std::string command = "which " + tool_name + " > /dev/null 2>&1";
    const bool available = (system(command.c_str()) == 0);
    tool_availability_cache_[tool_name] = available;

    LOG_DEBUG("[FormatFlow] tool cache miss: " + tool_name + " -> " +
              (available ? std::string("available") : std::string("missing")));
    return available;
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
