#include "features/lsp/lsp_stdio_connector.h"
#include "utils/logger.h"
#include "jsonrpccxx/common.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <regex>
#include <cstring>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdio>
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>
#include <chrono>

#ifdef USE_BOOST_PROCESS
#include <boost/process.hpp>
namespace bp = boost::process;
#endif

namespace pnana {
namespace features {

// 检查命令是否存在（在 PATH 中）
static bool checkCommandExists(const std::string& command) {
    // 如果命令包含路径分隔符，直接检查文件是否存在
    if (command.find('/') != std::string::npos) {
        struct stat st;
        return stat(command.c_str(), &st) == 0 && (st.st_mode & S_IXUSR);
    }
    
    // 否则，在 PATH 中查找
    const char* path_env = getenv("PATH");
    if (!path_env || path_env[0] == '\0') {
        return false;
    }
    
    std::string path_str(path_env);
    std::istringstream path_stream(path_str);
    std::string path_dir;
    while (std::getline(path_stream, path_dir, ':')) {
        if (path_dir.empty()) {
            continue;
        }
        std::string full_path = path_dir + "/" + command;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && (st.st_mode & S_IXUSR)) {
            return true;
        }
    }
    
    return false;
}

LspStdioConnector::LspStdioConnector(const std::string& server_command)
    : server_command_(server_command), env_vars_({})
#ifdef USE_BOOST_PROCESS
    , server_process_(nullptr), stdout_stream_(nullptr), stdin_stream_(nullptr)
#else
    , server_pid_(-1), stdin_file_(nullptr), stdout_file_(nullptr),
      stdin_fd_(-1), stdout_fd_(-1)
#endif
    , running_(false)
{
}

LspStdioConnector::LspStdioConnector(const std::string& server_command,
                                   const std::map<std::string, std::string>& env_vars)
    : server_command_(server_command), env_vars_(env_vars)
#ifdef USE_BOOST_PROCESS
    , server_process_(nullptr), stdout_stream_(nullptr), stdin_stream_(nullptr)
#else
    , server_pid_(-1), stdin_file_(nullptr), stdout_file_(nullptr),
      stdin_fd_(-1), stdout_fd_(-1)
#endif
    , running_(false)
{
}

LspStdioConnector::~LspStdioConnector() {
    stop();
}

bool LspStdioConnector::start() {
    LOG("LspStdioConnector::start() called, server_command: " + server_command_);
    
    if (running_) {
        LOG("LSP connector already running");
        return true;
    }
    
    // 检查服务器命令是否存在
    if (!checkCommandExists(server_command_)) {
        LOG_WARNING("LSP server command not found: " + server_command_ + ", skipping LSP initialization");
        return false;
    }
    LOG("LSP server command found: " + server_command_);
    
    try {
#ifdef USE_BOOST_PROCESS
        // 使用 boost::process
        stdout_stream_ = std::make_unique<bp::ipstream>();
        stdin_stream_ = std::make_unique<bp::opstream>();
        
        server_process_ = std::make_unique<bp::child>(
            server_command_,
            bp::std_out > *stdout_stream_,
            bp::std_in < *stdin_stream_,
            bp::std_err > bp::null
        );
        
        running_ = true;
            // 不在start()中启动通知监听线程，而是在initialize()完成后启动
            // startNotificationListener();
        return true;
#else
        // 使用 POSIX API
        int stdin_pipe[2], stdout_pipe[2];
        
        if (pipe(stdin_pipe) != 0 || pipe(stdout_pipe) != 0) {
            std::cerr << "Failed to create pipes" << std::endl;
            return false;
        }

        LOG("Forking process for LSP server...");
        server_pid_ = fork();
        if (server_pid_ < 0) {
            LOG_ERROR("Failed to fork process: " + std::string(strerror(errno)));
            return false;
        }

        if (server_pid_ == 0) {
            LOG("Child process: executing LSP server: " + server_command_);
            // 子进程：语言服务器
            close(stdin_pipe[1]);   // 关闭写端
            close(stdout_pipe[0]);  // 关闭读端

            dup2(stdin_pipe[0], STDIN_FILENO);
            dup2(stdout_pipe[1], STDOUT_FILENO);

            // 重定向 stderr 到 /dev/null，抑制 clangd 的日志输出
            int stderr_fd = open("/dev/null", O_WRONLY);
            if (stderr_fd >= 0) {
                dup2(stderr_fd, STDERR_FILENO);
                close(stderr_fd);
            }

            close(stdin_pipe[0]);
            close(stdout_pipe[1]);

            // 设置环境变量
            for (const auto& [key, value] : env_vars_) {
                setenv(key.c_str(), value.c_str(), 1); // 1 = overwrite existing
            }

            // 执行语言服务器
            // 如果 execlp 成功，不会返回；如果失败，会继续执行
            execlp(server_command_.c_str(), server_command_.c_str(), (char*)nullptr);
            // 如果执行到这里，说明 execlp 失败了
            // 在子进程中，我们不能使用 Logger（因为文件描述符已关闭）
            // 直接退出
            _exit(1);
        } else {
            // 父进程：编辑器
            LOG("Parent process: setting up pipes, child PID: " + std::to_string(server_pid_));
            close(stdin_pipe[0]);   // 关闭读端
            close(stdout_pipe[1]);  // 关闭写端
            
            stdin_fd_ = stdin_pipe[1];
            stdout_fd_ = stdout_pipe[0];
            
            stdin_file_ = fdopen(stdin_fd_, "w");
            stdout_file_ = fdopen(stdout_fd_, "r");
            
            if (!stdin_file_ || !stdout_file_) {
                LOG_ERROR("Failed to create file streams: " + std::string(strerror(errno)));
                stop();
                return false;
            }
            
            // 设置行缓冲模式（LSP 使用行分隔的头部）
            setvbuf(stdin_file_, nullptr, _IOLBF, 0);
            // stdout 使用行缓冲，因为头部是行分隔的
            setvbuf(stdout_file_, nullptr, _IOLBF, 0);
            LOG("File streams created and buffering set");
            
            // 设置为非阻塞模式，用于通知监听线程
            // 但在 Send() 方法中会临时改为阻塞模式以确保完整读取响应
            int flags = fcntl(stdout_fd_, F_GETFL, 0);
            fcntl(stdout_fd_, F_SETFL, flags | O_NONBLOCK);
            LOG("stdout set to non-blocking mode");
            
            // 等待一小段时间，检查子进程是否还在运行
            usleep(100000);  // 100ms
            int status;
            pid_t result = waitpid(server_pid_, &status, WNOHANG);
            if (result != 0) {
                // 子进程已经退出
                LOG_ERROR("LSP server process exited immediately, status: " + std::to_string(status));
                if (WIFEXITED(status)) {
                    LOG_ERROR("Exit code: " + std::to_string(WEXITSTATUS(status)));
                }
                if (WIFSIGNALED(status)) {
                    LOG_ERROR("Killed by signal: " + std::to_string(WTERMSIG(status)));
                }
                running_ = false;
                return false;
            }
            
            running_ = true;
            LOG("LSP connector started successfully");
            // 延迟启动通知监听线程，确保初始化请求先发送
            // startNotificationListener();
            return true;
        }
#endif
    } catch (const std::exception& e) {
        std::cerr << "Failed to start LSP server: " << e.what() << std::endl;
        running_ = false;
        return false;
    }
}

void LspStdioConnector::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // 停止通知监听线程
    if (notification_thread_.joinable()) {
        notification_thread_.join();
    }
    
#ifdef USE_BOOST_PROCESS
    // 终止服务器进程
    if (server_process_ && server_process_->running()) {
        server_process_->terminate();
        server_process_->wait();
    }
    
    server_process_.reset();
    stdout_stream_.reset();
    stdin_stream_.reset();
#else
    // 先关闭 stdin，让服务器知道输入结束
    // 注意：在发送 exit 通知后，服务器会关闭连接，所以这里可能会出错
    // 忽略关闭时的错误
    if (stdin_file_) {
        // 清除错误状态
        clearerr(stdin_file_);
        // 尝试关闭，忽略错误
        int result = fclose(stdin_file_);
        if (result != 0) {
            // 关闭失败，但继续执行（服务器可能已经关闭）
            std::cerr << "[LSP Connector] Warning: fclose(stdin_file_) failed (ignored)" << std::endl;
        }
        stdin_file_ = nullptr;
    }
    if (stdin_fd_ >= 0) {
        close(stdin_fd_);
        stdin_fd_ = -1;
    }
    
    // 等待进程退出（最多等待 2 秒）
    if (server_pid_ > 0) {
        int status;
        pid_t result;
        int wait_count = 0;
        const int max_wait = 20;  // 20 * 100ms = 2秒
        
        while (wait_count < max_wait) {
            result = waitpid(server_pid_, &status, WNOHANG);
            if (result == server_pid_) {
                // 进程已退出
                server_pid_ = -1;
                break;
            } else if (result == 0) {
                // 进程还在运行，等待 100ms
                usleep(100000);  // 100ms
                wait_count++;
            } else {
                // 错误或进程不存在
                break;
            }
        }
        
        // 如果进程还在运行，强制终止
        if (server_pid_ > 0) {
            kill(server_pid_, SIGKILL);
            waitpid(server_pid_, nullptr, 0);
            server_pid_ = -1;
        }
    }
    
    // 关闭 stdout 文件流
    if (stdout_file_) {
        fclose(stdout_file_);
        stdout_file_ = nullptr;
    }
    if (stdout_fd_ >= 0) {
        close(stdout_fd_);
        stdout_fd_ = -1;
    }
#endif
}

bool LspStdioConnector::isRunning() const {
    if (!running_) {
        return false;
    }
    
#ifdef USE_BOOST_PROCESS
    return server_process_ && server_process_->running();
#else
    if (server_pid_ <= 0) {
        return false;
    }
    // 检查进程是否还在运行
    int status;
    pid_t result = waitpid(server_pid_, &status, WNOHANG);
    if (result == 0) {
        return true;  // 进程还在运行
    }
    return false;
#endif
}

std::string LspStdioConnector::Send(const std::string& request) {
    if (!isRunning()) {
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "LSP server is not running"
        );
    }
    
    // 解析 JSON 消息，检查是否为通知（没有 id 字段）
    bool is_notification = false;
    try {
        jsonrpccxx::json json_msg = jsonrpccxx::json::parse(request);
        if (!json_msg.contains("id") || json_msg["id"].is_null()) {
            is_notification = true;
        }
    } catch (const std::exception& e) {
        // 如果解析失败，假设是请求（保守策略）
    }
    
    // 使用 request_mutex_ 确保通知监听线程不会在 Send() 读取响应时干扰
    // 注意：通知监听线程在读取消息时也应该获取这个锁
    std::lock_guard<std::mutex> lock(request_mutex_);
    
#ifndef USE_BOOST_PROCESS
    // 对于请求，临时设置为阻塞模式，确保完整读取响应
    // 对于通知，保持非阻塞模式（不需要响应）
    int flags = fcntl(stdout_fd_, F_GETFL, 0);
    if (!is_notification) {
        fcntl(stdout_fd_, F_SETFL, flags & ~O_NONBLOCK);
    }
#endif
    
    // 写入请求（自动添加 Content-Length 头部）
    writeLspMessage(request);
    
    // 确保数据已刷新
    fflush(stdin_file_);
    
    // 如果是通知，不需要读取响应
    if (is_notification) {
        return "";  // 通知不需要响应
    }
    
    // 读取响应（仅对请求）
    // 需要从请求中提取 ID，以便匹配响应
    int request_id = -1;
    try {
        jsonrpccxx::json json_msg = jsonrpccxx::json::parse(request);
        if (json_msg.contains("id") && !json_msg["id"].is_null()) {
            request_id = json_msg["id"].get<int>();
        }
    } catch (const std::exception& e) {
        // 如果解析失败，无法匹配 ID，继续使用原来的逻辑
    }
    
    try {
        // 可能需要读取多个消息，跳过通知消息，直到找到匹配的请求响应
        int max_attempts = 10;  // 最多尝试 10 次，防止无限循环
        int attempts = 0;
        
        while (attempts < max_attempts) {
            std::string response = readLspMessage();
            
            // 验证响应不为空
            if (response.empty()) {
                throw jsonrpccxx::JsonRpcException(
                    jsonrpccxx::error_type::internal_error,
                    "Empty response from LSP server"
                );
            }
            
            // 检查是否是通知消息（包含 method 字段，没有 id 字段或 id 为 null）
            try {
                jsonrpccxx::json response_json = jsonrpccxx::json::parse(response);
                
                // 如果是通知消息（有 method 字段），跳过它
                if (response_json.contains("method") && 
                    (!response_json.contains("id") || response_json["id"].is_null())) {
                    // 这是通知消息，继续读取下一个消息
                    attempts++;
                    continue;
                }
                
                // 如果是响应消息，检查 ID 是否匹配
                if (request_id >= 0 && response_json.contains("id")) {
                    int response_id = response_json["id"].get<int>();
                    if (response_id != request_id) {
                        // ID 不匹配，可能是其他请求的响应，继续读取
                        attempts++;
                        continue;
                    }
                }
                
                // 找到了匹配的响应
#ifndef USE_BOOST_PROCESS
                // 恢复非阻塞模式（用于通知监听线程）
                fcntl(stdout_fd_, F_SETFL, flags | O_NONBLOCK);
#endif
                return response;
                
            } catch (const std::exception& e) {
                // JSON 解析失败，可能是格式错误，直接返回
#ifndef USE_BOOST_PROCESS
                // 恢复非阻塞模式
                fcntl(stdout_fd_, F_SETFL, flags | O_NONBLOCK);
#endif
                return response;
            }
        }
        
        // 如果尝试了多次仍然没有找到匹配的响应，抛出异常
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Failed to find matching response after " + std::to_string(max_attempts) + " attempts"
        );
        
    } catch (const jsonrpccxx::JsonRpcException& e) {
#ifndef USE_BOOST_PROCESS
        // 恢复非阻塞模式
        fcntl(stdout_fd_, F_SETFL, flags | O_NONBLOCK);
#endif
        // 重新抛出异常
        throw;
    }
}

void LspStdioConnector::writeLspMessage(const std::string& message) {
    // LSP 协议要求：Content-Length: <length>\r\n\r\n<message>
    std::ostringstream header;
    header << "Content-Length: " << message.size() << "\r\n\r\n";
    
#ifdef USE_BOOST_PROCESS
    *stdin_stream_ << header.str() << message;
    stdin_stream_->flush();
#else
    std::string full_message = header.str() + message;
    fwrite(full_message.c_str(), 1, full_message.size(), stdin_file_);
    fflush(stdin_file_);
#endif
}

std::string LspStdioConnector::readLine() {
    std::string line;
#ifdef USE_BOOST_PROCESS
    if (std::getline(*stdout_stream_, line)) {
        // 移除可能的 \r
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
    } else {
        // getline 失败（EOF 或错误）
        line.clear();
    }
#else
    if (!stdout_file_ || stdout_fd_ < 0) {
        return "\x01EOF\x01";
    }
    
    // 检查文件描述符是否为非阻塞模式
    int flags = fcntl(stdout_fd_, F_GETFL, 0);
    bool is_nonblock = (flags & O_NONBLOCK) != 0;
    
    // 无论阻塞还是非阻塞模式，都使用 select 来检查数据可用性并设置超时
    // 这样可以避免在阻塞模式下无限期等待
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(stdout_fd_, &read_fds);
    struct timeval timeout;
    
    if (is_nonblock) {
        // 非阻塞模式：100ms 超时
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100ms
    } else {
        // 阻塞模式：使用更短的超时（2秒），避免长时间卡住
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
    }
    
    int result = select(stdout_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
    if (result <= 0) {
        // 超时或错误
        if (feof(stdout_file_)) {
            return "\x01EOF\x01";
        }
        if (result == 0) {
            // 超时：在非阻塞模式下返回空，在阻塞模式下抛出异常
            if (is_nonblock) {
                return "";  // 非阻塞模式：超时返回空
            } else {
                // 阻塞模式：超时是错误，返回错误标记
                return "\x02TIMEOUT\x02";
            }
        }
        // select 错误
        return "\x02ERROR\x02";
    }
    
    // 有数据可用，使用 fgets 读取一行
    char buffer[1024];
    if (fgets(buffer, sizeof(buffer), stdout_file_)) {
        line = buffer;
        // 移除换行符
        while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) {
            line.pop_back();
        }
    } else {
        // fgets 失败
        if (feof(stdout_file_)) {
            return "\x01EOF\x01";
        } else if (ferror(stdout_file_)) {
            // 在非阻塞模式下，EAGAIN 不是真正的错误
            if (is_nonblock && errno == EAGAIN) {
                return "";  // 返回空，表示暂时没有数据
            }
            return "\x02ERROR\x02";
        }
        line.clear();
    }
#endif
    return line;
}

std::string LspStdioConnector::readLspMessage() {
    auto read_start = std::chrono::steady_clock::now();
    std::string line;
    int content_length = -1;
    bool found_empty_line = false;
    int max_header_lines = 10;  // 最多读取 10 行头部，防止无限循环
    int line_count = 0;
    std::string header_lines;  // 用于调试
    
    // 读取头部（LSP 协议：头部以空行结束）
    // 格式：Content-Length: <number>\r\n\r\n
    while (line_count < max_header_lines) {
        line = readLine();
        line_count++;
        
        // 检查是否是 EOF 标记
        if (line == "\x01EOF\x01") {
            // EOF 是正常的（当服务器关闭连接时），不应该抛出异常
            // 在通知监听线程中，这会优雅地退出循环
            throw jsonrpccxx::JsonRpcException(
                jsonrpccxx::error_type::internal_error,
                "EOF while reading headers"
            );
        }
        
        // 检查是否是错误标记
        if (line == "\x02ERROR\x02") {
            throw jsonrpccxx::JsonRpcException(
                jsonrpccxx::error_type::internal_error,
                "Error reading from LSP server"
            );
        }
        
        // 检查是否是超时标记（阻塞模式下的超时）
        if (line == "\x02TIMEOUT\x02") {
            auto timeout_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - read_start);
            throw jsonrpccxx::JsonRpcException(
                jsonrpccxx::error_type::internal_error,
                "Timeout waiting for message headers (waited " + 
                std::to_string(timeout_time.count()) + " ms)"
            );
        }
        
        // 在非阻塞模式下，readLine() 可能返回空字符串（超时）
        // 在阻塞模式下，这不应该发生（应该返回 TIMEOUT 标记）
        if (line.empty() && line_count == 1) {
            // 第一次读取就返回空，可能是非阻塞模式下的超时
            // 在非阻塞模式下，这表示暂时没有数据，应该抛出异常
            throw jsonrpccxx::JsonRpcException(
                jsonrpccxx::error_type::internal_error,
                "Timeout waiting for message headers"
            );
        }
        
        // 保存头部行用于调试
        if (!line.empty()) {
            header_lines += line + "\n";
        }
        
        // 空行表示头部结束
        if (line.empty()) {
            found_empty_line = true;
            break;
        }
        
        // 解析 Content-Length（不区分大小写）
        std::string lower_line = line;
        std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(), ::tolower);
        
        // 查找 Content-Length 头部
        if (lower_line.find("content-length") != std::string::npos) {
            // 提取数字部分 - 使用更简单的字符串处理
            size_t colon_pos = lower_line.find(':');
            if (colon_pos != std::string::npos) {
                std::string value = line.substr(colon_pos + 1);
                // 去除前后空白
                value.erase(0, value.find_first_not_of(" \t\r\n"));
                value.erase(value.find_last_not_of(" \t\r\n") + 1);
                try {
                    content_length = std::stoi(value);
                } catch (const std::exception& e) {
                    // 解析失败，尝试正则表达式
            std::regex re(R"(content-length:\s*(\d+))", std::regex::icase);
            std::smatch match;
            if (std::regex_search(line, match, re)) {
                try {
                    content_length = std::stoi(match[1].str());
                        } catch (const std::exception& e2) {
                    // 解析失败，继续查找
                        }
                    }
                }
            }
        }
        // 忽略其他头部字段（如 Content-Type）
    }
    
    if (!found_empty_line) {
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Headers not properly terminated with empty line (read " + 
            std::to_string(line_count) + " lines). Headers: " + header_lines
        );
    }
    
    if (content_length <= 0) {
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Invalid Content-Length header: " + std::to_string(content_length) + 
            ". Headers: " + header_lines
        );
    }
    
    // 读取消息体
    std::string message;
    message.resize(content_length);
    
#ifdef USE_BOOST_PROCESS
    stdout_stream_->read(&message[0], content_length);
    if (stdout_stream_->gcount() != static_cast<std::streamsize>(content_length)) {
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Failed to read complete message: expected " + std::to_string(content_length) + 
            " bytes, got " + std::to_string(stdout_stream_->gcount())
        );
    }
#else
    // 确保读取完整的消息体
    size_t total_read = 0;
    size_t expected = static_cast<size_t>(content_length);
    
    while (total_read < expected) {
        size_t to_read = expected - total_read;
        size_t read = fread(&message[total_read], 1, to_read, stdout_file_);
        
        if (read == 0) {
            if (feof(stdout_file_)) {
                throw jsonrpccxx::JsonRpcException(
                    jsonrpccxx::error_type::internal_error,
                    "Unexpected EOF while reading message: expected " + std::to_string(expected) + 
                    " bytes, got " + std::to_string(total_read)
                );
            }
            // 如果不是EOF，可能是暂时没有数据，等待一下
            usleep(10000);  // 10ms
            continue;
        }
        total_read += read;
    }
    
    if (total_read != expected) {
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Failed to read complete message: expected " + std::to_string(expected) + 
            " bytes, got " + std::to_string(total_read)
        );
    }
#endif
    return message;
}

void LspStdioConnector::startNotificationListener() {
    notification_thread_ = std::thread([this]() {
        while (running_ && isRunning()) {
            try {
                // 使用非阻塞方式检查是否有完整的消息可用
                // 先检查是否有数据可读
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(stdout_fd_, &read_fds);
                struct timeval timeout;
                timeout.tv_sec = 0;
                timeout.tv_usec = 100000;  // 100ms 超时
                
                int result = select(stdout_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
                if (result <= 0) {
                    // 超时或错误，继续循环
                    if (feof(stdout_file_)) {
                        break;  // EOF，退出
                    }
                    // 超时是正常的，继续循环
                    continue;
                }
                
                // 有数据可用，尝试读取完整的 LSP 消息
                // 使用 try_lock 避免在 Send() 读取响应时长时间阻塞
                std::unique_lock<std::mutex> request_lock(request_mutex_, std::try_to_lock);
                if (!request_lock.owns_lock()) {
                    // 如果无法获取锁，说明 Send() 正在读取响应，等待一下再试
                    usleep(10000);  // 10ms
                    continue;
                }
                
                // 检查是否是通知消息（只读取通知，不读取请求响应）
                // 通过 peek 消息来判断，如果是通知则读取，否则跳过
                try {
                    std::string message = readLspMessage();
                    
                    // 检查消息是否为空
                    if (message.empty()) {
                        continue;
                    }
                    
                    // 解析消息，检查是否是通知
                    try {
                        jsonrpccxx::json msg_json = jsonrpccxx::json::parse(message);
                        
                        // 如果是通知消息（有 method 字段，没有 id 或 id 为 null）
                        if (msg_json.contains("method") && 
                            (!msg_json.contains("id") || msg_json["id"].is_null())) {
                            // 这是通知消息，加入队列
                            std::lock_guard<std::mutex> notif_lock(notification_mutex_);
                            notification_queue_.push(message);
                            
                            if (notification_callback_) {
                                notification_callback_(message);
                            }
                        } else {
                            // 这是请求响应，不应该被通知监听线程读取
                            // 这种情况不应该发生，因为 Send() 应该已经读取了
                            // 但为了安全，我们将其放回（实际上无法放回，所以记录错误）
                            LOG_ERROR("[LspConnector] Notification thread read a request response");
                        }
                    } catch (const std::exception& e) {
                        // JSON 解析失败，可能是格式错误，跳过
                        continue;
                    }
                } catch (const jsonrpccxx::JsonRpcException& e) {
                    // 检查是否是超时（非阻塞模式下的正常情况）
                    std::string error_msg = e.what() ? e.what() : "";
                    if (error_msg.find("Timeout") != std::string::npos) {
                        // 超时是正常的，继续循环
                        continue;
                    }
                    // 检查是否是 EOF（服务器关闭连接）
                    if (error_msg.find("EOF") != std::string::npos) {
                        break;  // EOF，优雅退出
                    }
                    // 其他错误，静默处理，继续循环
                    usleep(100000);  // 100ms
                }
            } catch (const jsonrpccxx::JsonRpcException& e) {
                if (running_) {
                    // 检查是否是超时或EOF
                    std::string error_msg = e.what() ? e.what() : "";
                    if (error_msg.find("EOF") != std::string::npos) {
                        break;  // EOF，退出
                    }
                    if (error_msg.find("Timeout") != std::string::npos) {
                        // 超时是正常的，继续循环
                        continue;
                    }
                    // 静默处理错误，避免影响界面
                    usleep(100000);  // 100ms
                } else {
                    break;
                }
            } catch (const std::exception& e) {
                if (running_) {
                    // 静默处理错误，避免影响界面
                    usleep(100000);  // 100ms
                } else {
                    break;
                }
            }
        }
    });
}

void LspStdioConnector::stopNotificationListener() {
    running_ = false;
    
    // 等待通知监听线程结束
    if (notification_thread_.joinable()) {
        notification_thread_.join();
    }
}

std::string LspStdioConnector::popNotification() {
    std::lock_guard<std::mutex> lock(notification_mutex_);
    if (notification_queue_.empty()) {
        return "";
    }
    
    std::string notification = notification_queue_.front();
    notification_queue_.pop();
    return notification;
}

void LspStdioConnector::setNotificationCallback(NotificationCallback callback) {
    notification_callback_ = callback;
}

} // namespace features
} // namespace pnana

