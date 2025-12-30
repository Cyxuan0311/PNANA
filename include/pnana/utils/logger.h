#ifndef PNANA_UTILS_LOGGER_H
#define PNANA_UTILS_LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

namespace pnana {
namespace utils {

/**
 * 简单的日志系统
 * 将调试信息写入日志文件，避免影响界面
 * 只有在调用 initialize() 后才会写入日志文件
 */
class Logger {
public:
    static Logger& getInstance();
    
    // 初始化日志文件（可选，只有调用此方法后才会写入日志）
    void initialize(const std::string& log_file = "pnana.log");
    
    // 检查日志是否已启用
    bool isEnabled() const;
    
    // 写入日志（如果未初始化，则静默忽略）
    void log(const std::string& message);
    void logError(const std::string& message);
    void logWarning(const std::string& message);
    
    // 关闭日志
    void close();
    
private:
    Logger() : initialized_(false) {}
    ~Logger();
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::ofstream log_file_;
    mutable std::mutex log_mutex_;
    bool initialized_;
    
    void writeLog(const std::string& level, const std::string& message);
    std::string getTimestamp();
};

// 便捷宏
#define LOG(msg) pnana::utils::Logger::getInstance().log(msg)
#define LOG_ERROR(msg) pnana::utils::Logger::getInstance().logError(msg)
#define LOG_WARNING(msg) pnana::utils::Logger::getInstance().logWarning(msg)

} // namespace utils
} // namespace pnana

#endif // PNANA_UTILS_LOGGER_H

