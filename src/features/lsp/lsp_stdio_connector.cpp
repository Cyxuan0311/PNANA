#include "features/lsp/lsp_stdio_connector.h"
#include "jsonrpccxx/common.hpp"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <signal.h>
#include <sstream>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#ifdef USE_BOOST_PROCESS
#include <boost/process.hpp>
namespace bp = boost::process;
#endif

namespace pnana {
namespace features {

// 检查命令是否存在（在 PATH 中）
static bool checkCommandExists(const std::string& command) {
    // 解析命令，获取第一个参数（可执行文件名）
    std::istringstream iss(command);
    std::string executable;
    iss >> executable;

    // 如果命令包含路径分隔符，直接检查文件是否存在
    if (executable.find('/') != std::string::npos) {
        struct stat st;
        return stat(executable.c_str(), &st) == 0 && (st.st_mode & S_IXUSR);
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
        std::string full_path = path_dir + "/" + executable;
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
      ,
      server_process_(nullptr), stdout_stream_(nullptr), stdin_stream_(nullptr)
#else
      ,
      server_pid_(-1), stdin_file_(nullptr), stdout_file_(nullptr), stdin_fd_(-1), stdout_fd_(-1)
#endif
      ,
      running_(false) {
}

LspStdioConnector::LspStdioConnector(const std::string& server_command,
                                     const std::map<std::string, std::string>& env_vars)
    : server_command_(server_command), env_vars_(env_vars)
#ifdef USE_BOOST_PROCESS
      ,
      server_process_(nullptr), stdout_stream_(nullptr), stdin_stream_(nullptr)
#else
      ,
      server_pid_(-1), stdin_file_(nullptr), stdout_file_(nullptr), stdin_fd_(-1), stdout_fd_(-1)
#endif
      ,
      running_(false) {
}

LspStdioConnector::~LspStdioConnector() {
    stop();
}

bool LspStdioConnector::start() {
    if (running_) {
        return true;
    }

    // 检查服务器命令是否存在
    if (!checkCommandExists(server_command_)) {
        LOG_WARNING("LSP server command not found: " + server_command_ +
                    ", skipping LSP initialization");
        return false;
    }

    try {
#ifdef USE_BOOST_PROCESS
        // 使用 boost::process
        stdout_stream_ = std::make_unique<bp::ipstream>();
        stdin_stream_ = std::make_unique<bp::opstream>();

        server_process_ =
            std::make_unique<bp::child>(server_command_, bp::std_out > *stdout_stream_,
                                        bp::std_in<*stdin_stream_, bp::std_err> bp::null);

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

        server_pid_ = fork();
        if (server_pid_ < 0) {
            LOG_ERROR("Failed to fork process: " + std::string(strerror(errno)));
            return false;
        }

        if (server_pid_ == 0) {
            // 子进程：语言服务器
            close(stdin_pipe[1]);  // 关闭写端
            close(stdout_pipe[0]); // 关闭读端

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

            // 解析命令字符串为参数数组
            std::vector<std::string> args;
            std::istringstream iss(server_command_);
            std::string arg;
            while (iss >> arg) {
                args.push_back(arg);
            }

            if (args.empty()) {
                _exit(1);
            }

            // 转换为 char* 数组，用于 execvp
            std::vector<char*> argv;
            for (auto& a : args) {
                argv.push_back(const_cast<char*>(a.c_str()));
            }
            argv.push_back(nullptr); // NULL 终止符

            // 执行语言服务器
            // 如果 execvp 成功，不会返回；如果失败，会继续执行
            execvp(argv[0], argv.data());
            // 如果执行到这里，说明 execvp 失败了
            // 在子进程中，我们不能使用 Logger（因为文件描述符已关闭）
            // 直接退出
            _exit(1);
        } else {
            // 父进程：编辑器
            close(stdin_pipe[0]);  // 关闭读端
            close(stdout_pipe[1]); // 关闭写端

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
            // 保持非阻塞模式
            int flags = fcntl(stdout_fd_, F_GETFL, 0);
            fcntl(stdout_fd_, F_SETFL, flags | O_NONBLOCK);

            // 等待一小段时间，检查子进程是否还在运行
            usleep(100000); // 100ms
            int status;
            pid_t result = waitpid(server_pid_, &status, WNOHANG);
            if (result != 0) {
                // 子进程已经退出
                LOG_ERROR("LSP server process exited immediately, status: " +
                          std::to_string(status));
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
            std::cerr << "[LSP Connector] Warning: fclose(stdin_file_) failed (ignored)"
                      << std::endl;
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
        const int max_wait = 20; // 20 * 100ms = 2秒

        while (wait_count < max_wait) {
            result = waitpid(server_pid_, &status, WNOHANG);
            if (result == server_pid_) {
                // 进程已退出
                server_pid_ = -1;
                break;
            } else if (result == 0) {
                // 进程还在运行，等待 100ms
                usleep(100000); // 100ms
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
        return true; // 进程还在运行
    }
    return false;
#endif
}

std::string LspStdioConnector::Send(const std::string& request) {
    if (!isRunning()) {
        throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                           "LSP server is not running");
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
    // 在非阻塞模式下，需要循环等待响应
    // 通知监听线程不会干扰，因为它只读取通知消息（没有id字段）
#endif

    // 写入请求（自动添加 Content-Length 头部）
    writeLspMessage(request);

    // 确保数据已刷新
    fflush(stdin_file_);

    // 如果是通知，不需要读取响应
    if (is_notification) {
        return ""; // 通知不需要响应
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
        int max_attempts = 10; // 最多尝试 10 次，防止无限循环
        int attempts = 0;

        while (attempts < max_attempts) {
            std::string response = readLspMessage();

            // 验证响应不为空
            if (response.empty()) {
                throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                                   "Empty response from LSP server");
            }

            // 检查是否是通知消息（包含 method 字段，没有 id 字段或 id 为 null）
            try {
                jsonrpccxx::json response_json;
                bool parsed = false;
                // 尝试清理响应两端的控制字符（避免因前导/尾随控制字符导致解析失败）
                auto trim_control_ends = [](std::string& s) {
                    while (!s.empty() && static_cast<unsigned char>(s.front()) <= 0x20) {
                        s.erase(0, 1);
                    }
                    while (!s.empty() && static_cast<unsigned char>(s.back()) <= 0x20) {
                        s.pop_back();
                    }
                };
                trim_control_ends(response);

                try {
                    response_json = jsonrpccxx::json::parse(response);
                    parsed = true;
                } catch (const jsonrpccxx::json::parse_error& pe) {
                    // 尝试裁剪尾部最多 256 字节以恢复（避免无限循环）
                    std::string trimmed = response;
                    size_t max_trim = std::min<size_t>(trimmed.size(), 256);
                    for (size_t t = 1; t <= max_trim; ++t) {
                        trimmed.pop_back();
                        try {
                            response_json = jsonrpccxx::json::parse(trimmed);
                            parsed = true;
                            break;
                        } catch (...) {
                            continue;
                        }
                    }
                    if (!parsed) {
                        // 尝试通过提取首个 '{' 到最后 '}' 之间的内容来恢复
                        size_t first_brace = response.find('{');
                        size_t last_brace = response.rfind('}');
                        if (first_brace != std::string::npos && last_brace != std::string::npos &&
                            last_brace > first_brace) {
                            std::string between =
                                response.substr(first_brace, last_brace - first_brace + 1);
                            try {
                                response_json = jsonrpccxx::json::parse(between);
                                parsed = true;
                                response = between;
                            } catch (...) {
                                // Brace-extraction fallback failed
                            }
                        }

                        if (!parsed) {
                            // 替换内部不可打印控制字符（除了 \t \n \r）为空格，然后尝试解析
                            std::string sanitized = response;
                            bool changed = false;
                            for (size_t i = 0; i < sanitized.size(); ++i) {
                                unsigned char uc = static_cast<unsigned char>(sanitized[i]);
                                if (uc < 0x20 && uc != 9 && uc != 10 && uc != 13) {
                                    sanitized[i] = ' ';
                                    changed = true;
                                }
                            }
                            if (changed) {
                                try {
                                    response_json = jsonrpccxx::json::parse(sanitized);
                                    parsed = true;
                                    response = sanitized;
                                } catch (...) {
                                    // Sanitizing fallback failed
                                }
                            }

                            if (!parsed) {
                                // 返回原始响应，交由上层处理
                                return response;
                            }
                        }
                    }
                }

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
                return response;

            } catch (const std::exception& e) {
                // JSON 解析或其他错误时，返回原始响应以便上层处理
                LOG(std::string("LSP <- Exception while handling response: ") + e.what());
                return response;
            }
        }

        // 如果尝试了多次仍然没有找到匹配的响应，抛出异常
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Failed to find matching response after " + std::to_string(max_attempts) + " attempts");

    } catch (const jsonrpccxx::JsonRpcException& e) {
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
    // 记录写入的头部和消息（限长以避免日志过大）
    std::string preview =
        message.size() > 512 ? message.substr(0, 512) + "...(truncated)" : message;
    LOG("LSP -> Writing message (header + preview):\n" + header.str() + preview);
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
        timeout.tv_usec = 100000; // 100ms
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
                return ""; // 非阻塞模式：超时返回空
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
                return ""; // 返回空，表示暂时没有数据
            }
            return "\x02ERROR\x02";
        }
        line.clear();
    }
#endif
    return line;
}

std::string LspStdioConnector::readLspMessage() {
    // 使用更稳健的消息读取方式：逐字节读取头部，避免行读取导致的边界问题
    std::string header_data;
    int content_length = -1;
    bool found_content_length = false;
    bool found_empty_line = false;

    // 读取头部数据，直到找到空行（\r\n\r\n）
    const int max_header_size = 4096; // 最大头部大小，防止无限读取
    size_t header_bytes_read = 0;

    // 整体超时控制
    const int max_wait_ms = 10000; // 10秒最大等待时间
    int total_wait_ms = 0;

    while (!found_empty_line && header_bytes_read < max_header_size &&
           total_wait_ms < max_wait_ms) {
        // 检查是否有数据可用
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(stdout_fd_, &read_fds);
        struct timeval timeout = {0, 100000}; // 100ms超时，更短的轮询间隔

        int result = select(stdout_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);

        if (result < 0) {
            // select错误
            if (feof(stdout_file_)) {
                throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                                   "EOF while reading headers");
            }
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Error reading from LSP server");
        }

        if (result == 0) {
            // 超时，没有数据，继续等待但不要抛出异常
            // 这在非阻塞模式下是正常的
            total_wait_ms += 100; // select超时了100ms
            continue;
        }

        // 有数据可用，使用read()而不是fgetc()来确保非阻塞读取
        char single_char;
        ssize_t bytes_read = read(stdout_fd_, &single_char, 1);

        if (bytes_read < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue; // 暂时没有数据，继续等待
            }
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Read error while reading headers");
        }

        if (bytes_read == 0) {
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Unexpected EOF while reading headers");
        }

        header_data += single_char;
        header_bytes_read++;

        // 检查是否找到了空行（\r\n\r\n的结束）
        if (header_data.size() >= 4) {
            size_t len = header_data.size();
            if (header_data[len - 4] == '\r' && header_data[len - 3] == '\n' &&
                header_data[len - 2] == '\r' && header_data[len - 1] == '\n') {
                found_empty_line = true;
                break;
            }
        }

        // 尝试解析 Content-Length（当我们有足够的数据时）
        if (!found_content_length && header_data.find("Content-Length:") != std::string::npos) {
            // 查找 Content-Length 头部
            std::string lower_header = header_data;
            std::transform(lower_header.begin(), lower_header.end(), lower_header.begin(),
                           ::tolower);

            size_t cl_pos = lower_header.find("content-length:");
            if (cl_pos != std::string::npos) {
                size_t colon_pos = header_data.find(':', cl_pos);
                if (colon_pos != std::string::npos) {
                    size_t end_pos = header_data.find('\r', colon_pos);
                    if (end_pos == std::string::npos) {
                        end_pos = header_data.find('\n', colon_pos);
                    }
                    if (end_pos != std::string::npos) {
                        std::string value =
                            header_data.substr(colon_pos + 1, end_pos - colon_pos - 1);
                        // 去除前后空白
                        value.erase(0, value.find_first_not_of(" \t\r\n"));
                        value.erase(value.find_last_not_of(" \t\r\n") + 1);
                        try {
                            content_length = std::stoi(value);
                            found_content_length = true;
                        } catch (const std::exception& e) {
                            // Failed to parse Content-Length
                        }
                    }
                }
            }
        }
    }

    // 检查是否因为超时而退出循环
    if (!found_empty_line) {
        if (total_wait_ms >= max_wait_ms) {
            std::string preview = header_data.substr(0, std::min<size_t>(header_data.size(), 200));
            if (header_data.size() > 200) {
                preview += "...(truncated)";
            }
            throw jsonrpccxx::JsonRpcException(
                jsonrpccxx::error_type::internal_error,
                "Timeout waiting for complete headers. Waited " + std::to_string(total_wait_ms) +
                    " ms. Header data (" + std::to_string(header_bytes_read) +
                    " bytes): " + preview);
        } else if (header_bytes_read >= max_header_size) {
            std::string preview = header_data.substr(0, std::min<size_t>(header_data.size(), 200));
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Header data too large (" +
                                                   std::to_string(header_bytes_read) +
                                                   " bytes). Preview: " + preview);
        } else {
            std::string preview = header_data.substr(0, std::min<size_t>(header_data.size(), 200));
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Unexpected end of header reading. Header data (" +
                                                   std::to_string(header_bytes_read) +
                                                   " bytes): " + preview);
        }
    }

    if (!found_empty_line) {
        std::string preview = header_data.substr(0, std::min<size_t>(header_data.size(), 200));
        if (header_data.size() > 200) {
            preview += "...(truncated)";
        }
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Headers not properly terminated with empty line. Header data (" +
                std::to_string(header_bytes_read) + " bytes): " + preview);
    }

    if (content_length <= 0) {
        std::string preview = header_data.substr(0, std::min<size_t>(header_data.size(), 200));
        if (header_data.size() > 200) {
            preview += "...(truncated)";
        }
        throw jsonrpccxx::JsonRpcException(
            jsonrpccxx::error_type::internal_error,
            "Invalid Content-Length header: " + std::to_string(content_length) + ". Header data (" +
                std::to_string(header_bytes_read) + " bytes): " + preview);
    }

    // 读取消息体
    std::string message;
    message.resize(content_length);
    size_t total_read = 0;
    size_t expected = static_cast<size_t>(content_length);

#ifdef USE_BOOST_PROCESS
    // 对于 BOOST_PROCESS，使用流读取
    stdout_stream_->read(&message[0], content_length);
    total_read = stdout_stream_->gcount();
    if (total_read != expected) {
        throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                           "Failed to read complete message: expected " +
                                               std::to_string(expected) + " bytes, got " +
                                               std::to_string(total_read));
    }
#else
    // 对于 POSIX，使用更稳健的读取方式
    while (total_read < expected) {
        // 检查是否有数据可用
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(stdout_fd_, &read_fds);
        struct timeval timeout = {5, 0}; // 5秒超时

        int result = select(stdout_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
        if (result <= 0) {
            if (result == 0) {
                throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                                   "Timeout waiting for message body data (" +
                                                       std::to_string(total_read) + "/" +
                                                       std::to_string(expected) + " bytes read)");
            }
            if (feof(stdout_file_)) {
                throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                                   "EOF while reading message body (" +
                                                       std::to_string(total_read) + "/" +
                                                       std::to_string(expected) + " bytes read)");
            }
            throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                               "Error reading message body");
        }

        size_t to_read = expected - total_read;
        size_t read = fread(&message[total_read], 1, to_read, stdout_file_);

        if (read == 0) {
            if (feof(stdout_file_)) {
                throw jsonrpccxx::JsonRpcException(
                    jsonrpccxx::error_type::internal_error,
                    "Unexpected EOF while reading message body: expected " +
                        std::to_string(expected) + " bytes, got " + std::to_string(total_read));
            }
            // 可能是暂时没有数据，继续等待
            continue;
        }
        total_read += read;
    }

    if (total_read != expected) {
        throw jsonrpccxx::JsonRpcException(jsonrpccxx::error_type::internal_error,
                                           "Failed to read complete message: expected " +
                                               std::to_string(expected) + " bytes, got " +
                                               std::to_string(total_read));
    }
#endif
    // 若花括号不平衡（'{'>'}'），尝试补足缺失的右花括号（最多补 8 个），作为最后的容错措施
    int open_braces = 0, close_braces = 0;
    for (char c : message) {
        if (c == '{')
            open_braces++;
        else if (c == '}')
            close_braces++;
    }
    if (open_braces > close_braces) {
        int need = open_braces - close_braces;
        int to_append = std::min(need, 8);
        message.append(to_append, '}');
    }

    std::string preview = message;
    if (preview.size() > 1024) {
        preview = preview.substr(0, 1024) + "...(truncated)";
    }
    // 如果消息长度较大，将完整二进制内容写入临时文件供调试使用
    try {
        std::string dump_path = std::string("/tmp/pnana_lsp_msg.bin");
        FILE* dump = fopen(dump_path.c_str(), "wb");
        if (dump) {
            fwrite(message.data(), 1, message.size(), dump);
            fflush(dump);
            fclose(dump);
        }
    } catch (...) {
        // 忽略写入错误，不影响主流程
    }

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
                timeout.tv_usec = 100000; // 100ms 超时

                int result = select(stdout_fd_ + 1, &read_fds, nullptr, nullptr, &timeout);
                if (result <= 0) {
                    // 超时或错误，继续循环
                    if (feof(stdout_file_)) {
                        break; // EOF，退出
                    }
                    // 超时是正常的，继续循环
                    continue;
                }

                // 有数据可用，尝试读取完整的 LSP 消息
                // 使用 try_lock 避免在 Send() 读取响应时长时间阻塞
                std::unique_lock<std::mutex> request_lock(request_mutex_, std::try_to_lock);
                if (!request_lock.owns_lock()) {
                    // 如果无法获取锁，说明 Send() 正在读取响应，等待一下再试
                    usleep(10000); // 10ms
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
                        break; // EOF，优雅退出
                    }
                    // 其他错误，静默处理，继续循环
                    usleep(100000); // 100ms
                }
            } catch (const jsonrpccxx::JsonRpcException& e) {
                if (running_) {
                    // 检查是否是超时或EOF
                    std::string error_msg = e.what() ? e.what() : "";
                    if (error_msg.find("EOF") != std::string::npos) {
                        break; // EOF，退出
                    }
                    if (error_msg.find("Timeout") != std::string::npos) {
                        // 超时是正常的，继续循环
                        continue;
                    }
                    // 静默处理错误，避免影响界面
                    usleep(100000); // 100ms
                } else {
                    break;
                }
            } catch (const std::exception& e) {
                if (running_) {
                    // 静默处理错误，避免影响界面
                    usleep(100000); // 100ms
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
