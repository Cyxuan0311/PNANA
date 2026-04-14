#include "utils/logger.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace pnana {
namespace utils {

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::initialize(const std::string& log_file) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (initialized_) {
        return;
    }

    log_file_.open(log_file, std::ios::app);
    if (log_file_.is_open()) {
        initialized_ = true;
    }
}

Logger::~Logger() {
    close();
}

void Logger::close() {
    std::lock_guard<std::mutex> lock(log_mutex_);

    if (log_file_.is_open()) {
        log_file_.close();
    }
    initialized_ = false;
}

std::string Logger::getTimestamp() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto tt = system_clock::to_time_t(now);
    const auto tm = *std::localtime(&tt);
    const auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0')
        << ms.count();
    return oss.str();
}

bool Logger::isEnabled() const {
    std::lock_guard<std::mutex> lock(log_mutex_);
    return initialized_ && log_file_.is_open();
}

void Logger::writeLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    // 仅在显式启用日志（例如 -l 参数）后才写入
    if (!initialized_ || !log_file_.is_open()) {
        return;
    }

    std::string timestamp = getTimestamp();
    log_file_ << "[" << timestamp << "] [" << level << "] " << message << std::endl;
    log_file_.flush();
}

void Logger::log(const std::string& message) {
    writeLog("INFO", message);
}

void Logger::logError(const std::string& message) {
    writeLog("ERROR", message);
}

void Logger::logWarning(const std::string& message) {
    writeLog("WARN", message);
}

void Logger::logDebug(const std::string& message) {
    writeLog("DEBUG", message);
}

void Logger::startTiming(const std::string& label) {
    std::lock_guard<std::mutex> lock(timing_mutex_);
    timing_points_[label] = std::chrono::steady_clock::now();
}

void Logger::endTiming(const std::string& label, const std::string& context) {
    std::lock_guard<std::mutex> lock(timing_mutex_);

    auto it = timing_points_.find(label);
    if (it == timing_points_.end()) {
        return; // 没有找到对应的开始点
    }

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - it->second);

    std::ostringstream oss;
    oss << "TIMING [" << label << "]: " << duration.count() << "ms";
    if (!context.empty()) {
        oss << " - " << context;
    }

    timing_points_.erase(it);

    // 使用 INFO 级别记录性能数据
    writeLog("INFO", oss.str());
}

void Logger::recordMetric(const std::string& metric_name, long long value,
                          const std::string& context) {
    std::ostringstream oss;
    oss << "METRIC [" << metric_name << "]: " << value;
    if (!context.empty()) {
        oss << " - " << context;
    }

    writeLog("INFO", oss.str());
}

} // namespace utils
} // namespace pnana
