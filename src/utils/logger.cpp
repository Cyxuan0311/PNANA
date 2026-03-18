#include "utils/logger.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <thread>

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
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string Logger::getThreadTag() {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

bool Logger::isEnabled() const {
    std::lock_guard<std::mutex> lock(log_mutex_);
    return initialized_ && log_file_.is_open();
}

void Logger::writeLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    // 如果未初始化或文件未打开，静默忽略（不写入日志文件）
    if (!initialized_ || !log_file_.is_open()) {
        return;
    }

    std::string timestamp = getTimestamp();
    log_file_ << "[" << timestamp << "] [" << level << "] [T" << getThreadTag() << "] " << message
              << std::endl;
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

} // namespace utils
} // namespace pnana
