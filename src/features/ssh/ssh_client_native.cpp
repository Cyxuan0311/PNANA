#include "features/ssh/ssh_client_native.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <future>
#include <libssh2_sftp.h>
#include <sstream>
#include <thread>
#include <vector>

namespace pnana {
namespace features {
namespace ssh {

// 优化配置常量
constexpr size_t SFTP_BUFFER_SIZE = 32768;       // 32KB 缓冲区 (原 4KB)
constexpr size_t BATCH_WRITE_SIZE = 8;           // 批量写入：累积 8 个缓冲区后写入
constexpr size_t PARALLEL_CHUNK_SIZE = 10485760; // 并行传输分片大小：10MB
constexpr int MAX_PARALLEL_CONNECTIONS = 4;      // 最大并行连接数

std::once_flag SSHClientNative::init_flag_;

SSHClientNative::SSHClientNative() {
    initLibSSH2();
}

SSHClientNative::~SSHClientNative() {
    // libssh2 会在程序退出时自动清理
}

void SSHClientNative::initLibSSH2() {
    std::call_once(init_flag_, []() {
        libssh2_init(0);
    });
}

// ==================== 文件操作 ====================

SSHResult SSHClientNative::readFile(const SSHConfig& config) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    std::string content;

    SSHResult result = sftpReadFile(session, config.remote_path, content);
    if (result.success) {
        result.content = content;
    }

    return result;
}

SSHResult SSHClientNative::writeFile(const SSHConfig& config, const std::string& content) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    return sftpWriteFile(session, config.remote_path, content);
}

SSHResult SSHClientNative::uploadFile(const SSHConfig& config, const std::string& local_path,
                                      const std::string& remote_path, ProgressCallback callback) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    return sftpUploadFile(session, local_path, remote_path, callback);
}

SSHResult SSHClientNative::downloadFile(const SSHConfig& config, const std::string& remote_path,
                                        const std::string& local_path, ProgressCallback callback) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    return sftpDownloadFile(session, remote_path, local_path, callback);
}

// ==================== 目录操作 ====================

SSHResult SSHClientNative::listDir(const SSHConfig& config) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();

    // 使用 SFTP 列出目录
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    LIBSSH2_SFTP_HANDLE* dir_handle =
        libssh2_sftp_opendir(sftp_session, config.remote_path.c_str());
    if (!dir_handle) {
        libssh2_sftp_shutdown(sftp_session);
        return SSHResult::fail("Failed to open directory");
    }

    std::ostringstream result_content;
    char name_buffer[512];
    LIBSSH2_SFTP_ATTRIBUTES attrs;

    while (libssh2_sftp_readdir_ex(dir_handle, name_buffer, sizeof(name_buffer), nullptr, 0,
                                   &attrs) > 0) {
        std::string name(name_buffer);
        if (name == "." || name == "..")
            continue;

        char type = (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) ? 'd' : 'f';
        result_content << type << "\t" << name << "\n";
    }

    libssh2_sftp_closedir(dir_handle);
    libssh2_sftp_shutdown(sftp_session);

    return SSHResult::ok(result_content.str());
}

std::vector<RemoteDirEntry> SSHClientNative::parseListDirContent(const std::string& content) {
    std::vector<RemoteDirEntry> entries;
    std::istringstream iss(content);
    std::string line;

    while (std::getline(iss, line)) {
        if (line.empty())
            continue;

        size_t tab_pos = line.find('\t');
        if (tab_pos == std::string::npos)
            continue;

        RemoteDirEntry entry;
        entry.is_directory = (line[0] == 'd');
        entry.name = line.substr(tab_pos + 1);
        entries.push_back(entry);
    }

    return entries;
}

SSHResult SSHClientNative::getPathType(const SSHConfig& config) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int ret = libssh2_sftp_stat(sftp_session, config.remote_path.c_str(), &attrs);

    libssh2_sftp_shutdown(sftp_session);

    if (ret == 0) {
        if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) {
            return SSHResult::ok("dir");
        } else if (attrs.permissions & LIBSSH2_SFTP_S_IFREG) {
            return SSHResult::ok("file");
        }
    }

    return SSHResult::ok("unknown");
}

SSHResult SSHClientNative::mkdir(const SSHConfig& config, const std::string& path) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    // 递归创建目录
    std::string current_path;
    std::istringstream iss(path);
    std::string component;

    while (std::getline(iss, component, '/')) {
        if (component.empty())
            continue;

        current_path += "/" + component;

        LIBSSH2_SFTP_ATTRIBUTES attrs;
        if (libssh2_sftp_stat(sftp_session, current_path.c_str(), &attrs) != 0) {
            // 目录不存在，创建它
            int ret = libssh2_sftp_mkdir_ex(sftp_session, current_path.c_str(),
                                            strlen(current_path.c_str()), 0755);
            if (ret != 0 && ret != LIBSSH2_ERROR_EAGAIN) {
                libssh2_sftp_shutdown(sftp_session);
                return SSHResult::fail("Failed to create directory: " + current_path);
            }
        }
    }

    libssh2_sftp_shutdown(sftp_session);
    return SSHResult::ok("Directory created: " + path);
}

SSHResult SSHClientNative::removeFile(const SSHConfig& config, const std::string& path) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    int ret = libssh2_sftp_unlink(sftp_session, path.c_str());
    libssh2_sftp_shutdown(sftp_session);

    if (ret == 0) {
        return SSHResult::ok("File removed: " + path);
    } else {
        return SSHResult::fail("Failed to remove file: " + path);
    }
}

SSHResult SSHClientNative::removeDir(const SSHConfig& config, const std::string& path) {
    // 先列出目录内容
    SSHConfig list_config = config;
    list_config.remote_path = path;

    SSHResult list_result = listDir(list_config);
    if (!list_result.success) {
        return list_result;
    }

    auto entries = parseListDirContent(list_result.content);

    // 递归删除所有子项
    for (const auto& entry : entries) {
        std::string full_path = path + "/" + entry.name;
        SSHResult result;

        if (entry.is_directory) {
            result = removeDir(config, full_path);
        } else {
            SSHConfig rm_config = config;
            rm_config.remote_path = full_path;
            result = removeFile(rm_config, full_path);
        }

        if (!result.success) {
            return result;
        }
    }

    // 删除空目录
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    int ret = libssh2_sftp_rmdir(sftp_session, path.c_str());
    libssh2_sftp_shutdown(sftp_session);

    if (ret == 0) {
        return SSHResult::ok("Directory removed: " + path);
    } else {
        return SSHResult::fail("Failed to remove directory: " + path);
    }
}

SSHResult SSHClientNative::rename(const SSHConfig& config, const std::string& old_path,
                                  const std::string& new_path) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    int ret = libssh2_sftp_rename(sftp_session, old_path.c_str(), new_path.c_str());
    libssh2_sftp_shutdown(sftp_session);

    if (ret == 0) {
        return SSHResult::ok("Renamed: " + old_path + " -> " + new_path);
    } else {
        return SSHResult::fail("Failed to rename: " + old_path);
    }
}

bool SSHClientNative::exists(const SSHConfig& config, const std::string& path) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return false;
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int ret = libssh2_sftp_stat(sftp_session, path.c_str(), &attrs);
    libssh2_sftp_shutdown(sftp_session);

    return (ret == 0);
}

SSHResult SSHClientNative::getFileSize(const SSHConfig& config, const std::string& path) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    if (!sftp_session) {
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int ret = libssh2_sftp_stat(sftp_session, path.c_str(), &attrs);
    libssh2_sftp_shutdown(sftp_session);

    if (ret == 0 && (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
        return SSHResult::ok(std::to_string(attrs.filesize));
    } else {
        return SSHResult::fail("Failed to get file size");
    }
}

// ==================== 命令执行 ====================

SSHResult SSHClientNative::runCommand(const SSHConfig& config, const std::string& command,
                                      const std::string& working_dir, bool pty) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();

    // 构建完整命令（包含 cd 到工作目录）
    std::string full_command = buildRemoteCommand(working_dir, command);

    // 打开 SSH 通道
    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(session);
    if (!channel) {
        return SSHResult::fail("Failed to open SSH channel");
    }

    // 如果请求 PTY，分配伪终端
    if (pty) {
        libssh2_channel_request_pty(channel, "xterm");
    }

    // 执行命令
    int ret = libssh2_channel_exec(channel, full_command.c_str());
    if (ret != 0) {
        libssh2_channel_close(channel);
        return SSHResult::fail("Failed to execute command");
    }

    // 读取输出
    std::string output;
    std::string error;
    char buffer[4096];

    // 读取 stdout
    ssize_t n;
    while ((n = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        output.append(buffer, n);
    }

    // 读取 stderr
    while ((n = libssh2_channel_read_ex(channel, SSH_EXTENDED_DATA_STDERR, buffer,
                                        sizeof(buffer))) > 0) {
        error.append(buffer, n);
    }

    // 等待命令完成
    libssh2_channel_wait_closed(channel);

    // 获取退出码
    int exit_code = 0;
    libssh2_channel_get_exit_status(channel);

    libssh2_channel_close(channel);

    SSHResult result;
    result.content = output;

    if (exit_code == 0) {
        result.success = true;
    } else {
        result.success = false;
        result.error =
            error.empty() ? "Command exited with code " + std::to_string(exit_code) : error;
    }

    return result;
}

SSHResult SSHClientNative::runCommandStreaming(
    const SSHConfig& config, const std::string& command,
    std::function<void(const std::string&)> stdout_callback,
    std::function<void(const std::string&)> stderr_callback, const std::string& working_dir) {
    auto conn = SSHPool::getInstance().acquire(config);
    if (!conn || !conn->isConnected()) {
        return SSHResult::fail("Failed to connect to SSH server");
    }

    SSHConnectionGuard guard(conn);

    LIBSSH2_SESSION* session = conn->getSession();

    std::string full_command = buildRemoteCommand(working_dir, command);

    LIBSSH2_CHANNEL* channel = libssh2_channel_open_session(session);
    if (!channel) {
        return SSHResult::fail("Failed to open SSH channel");
    }

    int ret = libssh2_channel_exec(channel, full_command.c_str());
    if (ret != 0) {
        libssh2_channel_close(channel);
        return SSHResult::fail("Failed to execute command");
    }

    char buffer[4096];
    ssize_t n;

    // 流式读取 stdout
    while ((n = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        if (stdout_callback) {
            stdout_callback(std::string(buffer, n));
        }
    }

    // 流式读取 stderr
    while ((n = libssh2_channel_read_ex(channel, SSH_EXTENDED_DATA_STDERR, buffer,
                                        sizeof(buffer))) > 0) {
        if (stderr_callback) {
            stderr_callback(std::string(buffer, n));
        }
    }

    libssh2_channel_wait_closed(channel);
    int exit_code = libssh2_channel_get_exit_status(channel);

    libssh2_channel_close(channel);

    return (exit_code == 0)
               ? SSHResult::ok("Command completed")
               : SSHResult::fail("Command failed with exit code " + std::to_string(exit_code));
}

// ==================== SFTP 辅助函数 ====================

SSHResult SSHClientNative::sftpReadFile(LIBSSH2_SESSION* session, const std::string& remote_path,
                                        std::string& content) {
    auto start_time = std::chrono::steady_clock::now();

    utils::Logger::getInstance().startTiming("sftp_init_" + remote_path);
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    utils::Logger::getInstance().endTiming("sftp_init_" + remote_path, "SFTP initialization");

    if (!sftp_session) {
        utils::Logger::getInstance().logError("SFTP initialization failed: " + remote_path);
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    utils::Logger::getInstance().startTiming("sftp_open_" + remote_path);
    LIBSSH2_SFTP_HANDLE* file =
        libssh2_sftp_open(sftp_session, remote_path.c_str(), LIBSSH2_FXF_READ, 0);
    utils::Logger::getInstance().endTiming("sftp_open_" + remote_path, "Open remote file");

    if (!file) {
        libssh2_sftp_shutdown(sftp_session);
        utils::Logger::getInstance().logError("Failed to open remote file: " + remote_path);
        return SSHResult::fail("Failed to open remote file: " + remote_path);
    }

    // Get file size
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    if (libssh2_sftp_fstat(file, &attrs) == 0 && (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
        content.reserve(attrs.filesize);
        utils::Logger::getInstance().log("File size: " + std::to_string(attrs.filesize) + " bytes");
    }

    // Read file content with optimized buffer
    char buffer[SFTP_BUFFER_SIZE];
    ssize_t n;
    size_t total_read = 0;

    while ((n = libssh2_sftp_read(file, buffer, sizeof(buffer))) > 0) {
        content.append(buffer, n);
        total_read += n;
    }

    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_time)
                              .count();

    utils::Logger::getInstance().log(
        "File read completed: total=" + std::to_string(total_read) +
        ", duration=" + std::to_string(total_duration) + "ms" + ", avg_speed=" +
        (total_duration > 0 ? std::to_string((total_read * 1000) / total_duration) : "N/A") +
        " B/s");

    libssh2_sftp_close(file);
    libssh2_sftp_shutdown(sftp_session);

    return SSHResult::ok("File read successfully");
}

SSHResult SSHClientNative::sftpWriteFile(LIBSSH2_SESSION* session, const std::string& remote_path,
                                         const std::string& content) {
    auto start_time = std::chrono::steady_clock::now();

    utils::Logger::getInstance().startTiming("sftp_init_write_" + remote_path);
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    utils::Logger::getInstance().endTiming("sftp_init_write_" + remote_path,
                                           "SFTP initialization (write)");

    if (!sftp_session) {
        utils::Logger::getInstance().logError("SFTP initialization failed (write): " + remote_path);
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    utils::Logger::getInstance().startTiming("sftp_open_write_" + remote_path);
    LIBSSH2_SFTP_HANDLE* file = libssh2_sftp_open(
        sftp_session, remote_path.c_str(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    utils::Logger::getInstance().endTiming("sftp_open_write_" + remote_path,
                                           "Open remote file (write)");

    if (!file) {
        libssh2_sftp_shutdown(sftp_session);
        utils::Logger::getInstance().logError("Failed to open remote file for writing: " +
                                              remote_path);
        return SSHResult::fail("Failed to open remote file for writing: " + remote_path);
    }

    // Write content with batch optimization
    size_t total_written = 0;

    utils::Logger::getInstance().log(
        "Starting file write, total size: " + std::to_string(content.size()) + " bytes");

    // Batch buffer for optimized writes
    std::vector<char> batch_buffer;
    batch_buffer.reserve(SFTP_BUFFER_SIZE * BATCH_WRITE_SIZE);

    while (total_written < content.size()) {
        size_t chunk_size = std::min(SFTP_BUFFER_SIZE, content.size() - total_written);

        // Add to batch buffer
        batch_buffer.insert(batch_buffer.end(), content.c_str() + total_written,
                            content.c_str() + total_written + chunk_size);
        total_written += chunk_size;

        // Write when batch is full or at the end
        if (batch_buffer.size() >= SFTP_BUFFER_SIZE * BATCH_WRITE_SIZE ||
            total_written >= content.size()) {
            // Write all data from batch buffer (handle partial writes)
            size_t offset = 0;
            while (offset < batch_buffer.size()) {
                ssize_t n = libssh2_sftp_write(file, batch_buffer.data() + offset,
                                               batch_buffer.size() - offset);
                if (n < 0) {
                    libssh2_sftp_close(file);
                    libssh2_sftp_shutdown(sftp_session);
                    utils::Logger::getInstance().logError("Failed to write to remote file: " +
                                                          remote_path);
                    return SSHResult::fail("Failed to write to remote file");
                }
                offset += n;
            }
            batch_buffer.clear();
        }
    }

    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_time)
                              .count();

    utils::Logger::getInstance().log(
        "File write completed: total=" + std::to_string(total_written) +
        ", duration=" + std::to_string(total_duration) + "ms" + ", avg_speed=" +
        (total_duration > 0 ? std::to_string((total_written * 1000) / total_duration) : "N/A") +
        " B/s");

    libssh2_sftp_close(file);
    libssh2_sftp_shutdown(sftp_session);

    return SSHResult::ok("File written successfully");
}

SSHResult SSHClientNative::sftpUploadFile(LIBSSH2_SESSION* session, const std::string& local_path,
                                          const std::string& remote_path,
                                          ProgressCallback callback) {
    auto start_time = std::chrono::steady_clock::now();

    utils::Logger::getInstance().log("Starting file upload: " + local_path + " -> " + remote_path);

    // Open local file
    utils::Logger::getInstance().startTiming("upload_local_open_" + local_path);
    std::ifstream local_file(local_path, std::ios::binary);
    if (!local_file) {
        utils::Logger::getInstance().logError("Failed to open local file: " + local_path);
        return SSHResult::fail("Failed to open local file: " + local_path);
    }
    utils::Logger::getInstance().endTiming("upload_local_open_" + local_path, "Open local file");

    // Get local file size
    utils::Logger::getInstance().startTiming("upload_get_size_" + local_path);
    local_file.seekg(0, std::ios::end);
    size_t file_size = local_file.tellg();
    local_file.seekg(0, std::ios::beg);
    utils::Logger::getInstance().endTiming("upload_get_size_" + local_path, "Get file size");

    utils::Logger::getInstance().log("Local file size: " + std::to_string(file_size) + " bytes");

    // Initialize SFTP
    utils::Logger::getInstance().startTiming("upload_sftp_init_" + remote_path);
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    utils::Logger::getInstance().endTiming("upload_sftp_init_" + remote_path,
                                           "SFTP initialization (upload)");

    if (!sftp_session) {
        utils::Logger::getInstance().logError("SFTP initialization failed (upload): " +
                                              remote_path);
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    // Open remote file
    utils::Logger::getInstance().startTiming("upload_sftp_open_" + remote_path);
    LIBSSH2_SFTP_HANDLE* file = libssh2_sftp_open(
        sftp_session, remote_path.c_str(),
        LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC,
        LIBSSH2_SFTP_S_IRUSR | LIBSSH2_SFTP_S_IWUSR | LIBSSH2_SFTP_S_IRGRP | LIBSSH2_SFTP_S_IROTH);
    utils::Logger::getInstance().endTiming("upload_sftp_open_" + remote_path,
                                           "Open remote file (upload)");

    if (!file) {
        libssh2_sftp_shutdown(sftp_session);
        utils::Logger::getInstance().logError("Failed to open remote file for upload: " +
                                              remote_path);
        return SSHResult::fail("Failed to open remote file for writing: " + remote_path);
    }

    // Upload file with optimized buffer and batch write
    char buffer[SFTP_BUFFER_SIZE];
    size_t total_transferred = 0;

    // Batch buffer for optimized writes
    std::vector<char> batch_buffer;
    batch_buffer.reserve(SFTP_BUFFER_SIZE * BATCH_WRITE_SIZE);

    utils::Logger::getInstance().log("Starting data transfer...");

    while (local_file.read(buffer, sizeof(buffer)) || local_file.gcount() > 0) {
        size_t bytes_read = local_file.gcount();

        // Add to batch buffer
        batch_buffer.insert(batch_buffer.end(), buffer, buffer + bytes_read);
        total_transferred += bytes_read;

        // Write when batch is full or at the end
        bool is_final = (total_transferred >= file_size);
        if (batch_buffer.size() >= SFTP_BUFFER_SIZE * BATCH_WRITE_SIZE || is_final) {
            // Write all data from batch buffer (handle partial writes)
            size_t offset = 0;
            while (offset < batch_buffer.size()) {
                ssize_t written = libssh2_sftp_write(file, batch_buffer.data() + offset,
                                                     batch_buffer.size() - offset);
                if (written < 0) {
                    libssh2_sftp_close(file);
                    libssh2_sftp_shutdown(sftp_session);
                    utils::Logger::getInstance().logError(
                        "Failed to write to remote file (upload): " + remote_path);
                    return SSHResult::fail("Failed to write to remote file");
                }
                offset += written;
            }
            batch_buffer.clear();
        }

        if (callback) {
            callback(total_transferred, file_size);
        }
    }

    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_time)
                              .count();

    utils::Logger::getInstance().log(
        "File upload completed: total=" + std::to_string(total_transferred) +
        ", duration=" + std::to_string(total_duration) + "ms" + ", avg_speed=" +
        (total_duration > 0 ? std::to_string((total_transferred * 1000) / total_duration) : "N/A") +
        " B/s");

    libssh2_sftp_close(file);
    libssh2_sftp_shutdown(sftp_session);

    return SSHResult::ok("File uploaded successfully");
}

SSHResult SSHClientNative::sftpDownloadFile(LIBSSH2_SESSION* session,
                                            const std::string& remote_path,
                                            const std::string& local_path,
                                            ProgressCallback callback) {
    auto start_time = std::chrono::steady_clock::now();

    utils::Logger::getInstance().log("Starting file download: " + remote_path + " -> " +
                                     local_path);

    // Initialize SFTP
    utils::Logger::getInstance().startTiming("download_sftp_init_" + remote_path);
    LIBSSH2_SFTP* sftp_session = libssh2_sftp_init(session);
    utils::Logger::getInstance().endTiming("download_sftp_init_" + remote_path,
                                           "SFTP initialization (download)");

    if (!sftp_session) {
        utils::Logger::getInstance().logError("SFTP initialization failed (download): " +
                                              remote_path);
        return SSHResult::fail("Failed to initialize SFTP session");
    }

    // Open remote file
    utils::Logger::getInstance().startTiming("download_sftp_open_" + remote_path);
    LIBSSH2_SFTP_HANDLE* file =
        libssh2_sftp_open(sftp_session, remote_path.c_str(), LIBSSH2_FXF_READ, 0);
    utils::Logger::getInstance().endTiming("download_sftp_open_" + remote_path,
                                           "Open remote file (download)");

    if (!file) {
        libssh2_sftp_close(file);
        libssh2_sftp_shutdown(sftp_session);
        utils::Logger::getInstance().logError("Failed to open remote file for download: " +
                                              remote_path);
        return SSHResult::fail("Failed to open remote file: " + remote_path);
    }

    // Get file size
    utils::Logger::getInstance().startTiming("download_get_size_" + remote_path);
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    size_t file_size = 0;
    if (libssh2_sftp_fstat(file, &attrs) == 0 && (attrs.flags & LIBSSH2_SFTP_ATTR_SIZE)) {
        file_size = attrs.filesize;
    }
    utils::Logger::getInstance().endTiming("download_get_size_" + remote_path, "Get file size");

    utils::Logger::getInstance().log("Remote file size: " + std::to_string(file_size) + " bytes");

    // Open local file
    utils::Logger::getInstance().startTiming("download_local_open_" + local_path);
    std::ofstream local_file(local_path, std::ios::binary);
    if (!local_file) {
        libssh2_sftp_close(file);
        libssh2_sftp_shutdown(sftp_session);
        utils::Logger::getInstance().logError("Failed to open local file for writing: " +
                                              local_path);
        return SSHResult::fail("Failed to open local file for writing: " + local_path);
    }
    utils::Logger::getInstance().endTiming("download_local_open_" + local_path, "Open local file");

    // Download file with optimized buffer
    char buffer[SFTP_BUFFER_SIZE];
    ssize_t n;
    size_t total_transferred = 0;

    utils::Logger::getInstance().log("Starting data transfer...");

    while ((n = libssh2_sftp_read(file, buffer, sizeof(buffer))) > 0) {
        local_file.write(buffer, n);
        total_transferred += n;

        if (callback) {
            callback(total_transferred, file_size);
        }
    }

    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                              std::chrono::steady_clock::now() - start_time)
                              .count();

    utils::Logger::getInstance().log(
        "File download completed: total=" + std::to_string(total_transferred) +
        ", duration=" + std::to_string(total_duration) + "ms" + ", avg_speed=" +
        (total_duration > 0 ? std::to_string((total_transferred * 1000) / total_duration) : "N/A") +
        " B/s");

    libssh2_sftp_close(file);
    libssh2_sftp_shutdown(sftp_session);

    return SSHResult::ok("File downloaded successfully");
}

// ==================== 辅助函数 ====================

std::string SSHClientNative::escapeShellPath(const std::string& path) {
    std::string escaped = "'";
    for (char c : path) {
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    escaped += "'";
    return escaped;
}

std::string SSHClientNative::escapeShellArg(const std::string& arg) {
    return escapeShellPath(arg);
}

std::string SSHClientNative::buildRemoteCommand(const std::string& working_dir,
                                                const std::string& command) {
    if (working_dir.empty()) {
        return command;
    }

    return "cd " + escapeShellPath(working_dir) + " && " + command;
}

} // namespace ssh
} // namespace features
} // namespace pnana
