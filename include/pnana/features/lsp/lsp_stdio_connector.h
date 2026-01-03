#ifndef PNANA_FEATURES_LSP_STDIO_CONNECTOR_H
#define PNANA_FEATURES_LSP_STDIO_CONNECTOR_H

#include "jsonrpccxx/iclientconnector.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

// 使用标准库实现跨平台进程通信
// 如果系统支持，可以使用 boost::process
#ifdef USE_BOOST_PROCESS
#include <boost/process.hpp>
namespace bp = boost::process;
#else
// 使用标准库和 POSIX API
#include <cstdio>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace pnana {
namespace features {

/**
 * LSP STDIO 传输层实现
 * 负责管理语言服务器进程和 stdio 通信
 */
class LspStdioConnector : public jsonrpccxx::IClientConnector {
  public:
    explicit LspStdioConnector(const std::string& server_command);
    LspStdioConnector(const std::string& server_command,
                      const std::map<std::string, std::string>& env_vars);
    ~LspStdioConnector() override;

    // 启动语言服务器
    bool start();

    // 停止语言服务器
    void stop();

    // 检查服务器是否运行
    bool isRunning() const;

    // 实现 IClientConnector 接口
    std::string Send(const std::string& request) override;

    // 启动后台线程监听服务器通知
    void startNotificationListener();

    // 停止通知监听线程
    void stopNotificationListener();

    // 获取待处理的通知
    std::string popNotification();

    // 设置通知回调
    using NotificationCallback = std::function<void(const std::string&)>;
    void setNotificationCallback(NotificationCallback callback);

  private:
    std::string server_command_;
    std::map<std::string, std::string> env_vars_;

#ifdef USE_BOOST_PROCESS
    std::unique_ptr<bp::child> server_process_;
    std::unique_ptr<bp::ipstream> stdout_stream_;
    std::unique_ptr<bp::opstream> stdin_stream_;
#else
    // 使用标准库和 POSIX API
    pid_t server_pid_;
    FILE* stdin_file_;
    FILE* stdout_file_;
    int stdin_fd_;
    int stdout_fd_;
#endif

    std::mutex request_mutex_;
    std::mutex response_mutex_;
    std::atomic<bool> running_;

    // 通知监听
    std::thread notification_thread_;
    std::queue<std::string> notification_queue_;
    std::mutex notification_mutex_;
    NotificationCallback notification_callback_;

    // 读取 LSP 消息（处理 Content-Length 头部）
    std::string readLspMessage();

    // 写入 LSP 消息（添加 Content-Length 头部）
    void writeLspMessage(const std::string& message);

    // 读取一行（处理 \r\n）
    std::string readLine();
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_STDIO_CONNECTOR_H
