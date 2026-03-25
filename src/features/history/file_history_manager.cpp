#include "features/history/file_history_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <set>
#include <sstream>

namespace pnana {
namespace features {
namespace history {

namespace {
using json = nlohmann::json;

std::string joinPath(const std::string& a, const std::string& b) {
    return (std::filesystem::path(a) / b).string();
}
} // namespace

FileHistoryManager::FileHistoryManager() {
    const char* home = std::getenv("HOME");
    std::string home_dir = home ? std::string(home) : std::string(".");

    // 固定使用 ~/.config/pnana/history 作为历史版本根目录
    history_root_ = joinPath(home_dir, ".config/pnana/history");

    std::error_code ec;
    std::filesystem::create_directories(history_root_, ec);
    if (ec) {
        // 仅在该路径不可用时，最后回退到当前目录，避免功能失效
        LOG_ERROR("[history] create fixed root failed path=" + history_root_ +
                  " ec=" + ec.message());

        ec.clear();
        history_root_ = ".pnana_history";
        std::filesystem::create_directories(history_root_, ec);
        if (ec) {
            LOG_ERROR("[history] local fallback root failed path=" + history_root_ +
                      " ec=" + ec.message());
        } else {
            LOG_WARNING("[history] using local fallback root=" + history_root_);
        }
    } else {
        LOG("[history] using fixed root=" + history_root_);
    }
}

void FileHistoryManager::setRetentionConfig(const HistoryRetentionConfig& cfg) {
    std::lock_guard<std::mutex> lock(mutex_);
    retention_config_ = cfg;
}

long long FileHistoryManager::parseSizeToBytes(const std::string& text) const {
    if (text.empty())
        return 0;
    std::string s = text;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    long long multiplier = 1;
    if (s.size() >= 2 && s.substr(s.size() - 2) == "KB") {
        multiplier = 1024LL;
        s = s.substr(0, s.size() - 2);
    } else if (s.size() >= 2 && s.substr(s.size() - 2) == "MB") {
        multiplier = 1024LL * 1024LL;
        s = s.substr(0, s.size() - 2);
    } else if (s.size() >= 2 && s.substr(s.size() - 2) == "GB") {
        multiplier = 1024LL * 1024LL * 1024LL;
        s = s.substr(0, s.size() - 2);
    } else if (!s.empty() && s.back() == 'B') {
        s.pop_back();
    }

    try {
        return std::stoll(s) * multiplier;
    } catch (...) {
        return 0;
    }
}

long long FileHistoryManager::calculateDirectorySize(const std::filesystem::path& path) const {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec)
        return 0;

    long long total = 0;
    for (auto it = std::filesystem::recursive_directory_iterator(path, ec);
         it != std::filesystem::recursive_directory_iterator(); it.increment(ec)) {
        if (ec)
            continue;
        if (it->is_regular_file(ec) && !ec) {
            total += static_cast<long long>(it->file_size(ec));
        }
    }
    return total;
}

bool FileHistoryManager::cleanupHistory(const std::string& dir, int latest_version,
                                        std::vector<VersionMeta>& versions) {
    if (!retention_config_.enable) {
        return true;
    }

    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                      std::chrono::system_clock::now().time_since_epoch())
                      .count();
    long long max_age_ms =
        static_cast<long long>(retention_config_.max_age_days) * 86400LL * 1000LL;

    std::set<int> keep_versions;
    if (latest_version > 0)
        keep_versions.insert(latest_version);
    if (retention_config_.keep_critical_versions) {
        for (const auto& v : versions) {
            if (v.critical)
                keep_versions.insert(v.version);
        }
    }

    std::vector<VersionMeta> candidates = versions;
    std::sort(candidates.begin(), candidates.end(), [](const VersionMeta& a, const VersionMeta& b) {
        return a.version > b.version;
    });

    int kept_non_critical = 0;
    for (const auto& v : candidates) {
        bool keep = keep_versions.count(v.version) > 0;
        bool too_old = (max_age_ms > 0) && (now_ms - v.timestamp > max_age_ms);
        if (!keep && !too_old && kept_non_critical < retention_config_.max_entries) {
            keep = true;
            kept_non_critical++;
        }
        if (keep)
            keep_versions.insert(v.version);
    }

    std::vector<VersionMeta> new_versions;
    for (const auto& v : versions) {
        if (keep_versions.count(v.version)) {
            new_versions.push_back(v);
        }
    }

    // 按总大小再收紧（优先删最老的非关键版本）
    long long max_total_bytes = parseSizeToBytes(retention_config_.max_total_size);
    if (max_total_bytes > 0) {
        while (calculateDirectorySize(dir) > max_total_bytes) {
            auto it = std::min_element(new_versions.begin(), new_versions.end(),
                                       [](const VersionMeta& a, const VersionMeta& b) {
                                           return a.version < b.version;
                                       });
            if (it == new_versions.end())
                break;
            if (it->critical || it->version == latest_version) {
                break;
            }
            std::error_code ec;
            std::filesystem::remove(diffPath(dir, it->version, it->version + 1), ec);
            new_versions.erase(it);
        }
    }

    versions = std::move(new_versions);
    return true;
}

std::string FileHistoryManager::getFileHash(const std::string& file_path) const {
    std::size_t h = std::hash<std::string>{}(file_path);
    std::stringstream ss;
    ss << std::hex << h;
    return ss.str();
}

std::string FileHistoryManager::getFileHistoryDir(const std::string& file_path) const {
    return joinPath(history_root_, getFileHash(file_path));
}

bool FileHistoryManager::ensureHistoryDir(const std::string& dir) const {
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return !ec;
}

std::string FileHistoryManager::basePath(const std::string& dir) const {
    return joinPath(dir, "base_v1");
}

std::string FileHistoryManager::diffPath(const std::string& dir, int from_version,
                                         int to_version) const {
    return joinPath(dir,
                    "diff_v" + std::to_string(from_version) + "_to_v" + std::to_string(to_version));
}

bool FileHistoryManager::loadMeta(const std::string& dir, int& latest_version,
                                  std::vector<VersionMeta>& versions) const {
    latest_version = 0;
    versions.clear();

    const std::string meta_path = joinPath(dir, "meta.json");
    std::ifstream in(meta_path);
    if (!in.is_open()) {
        LOG_WARNING("[history] loadMeta open failed path=" + meta_path);
        return false;
    }

    try {
        json j;
        in >> j;
        latest_version = j.value("latest_version", 0);
        auto arr = j.value("versions", json::array());
        for (const auto& v : arr) {
            VersionMeta m;
            m.version = v.value("version", 0);
            m.timestamp = v.value("timestamp", 0LL);
            m.critical = v.value("critical", false);
            versions.push_back(m);
        }
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR(std::string("[history] loadMeta parse failed: ") + e.what() +
                  " path=" + meta_path);
        return false;
    } catch (...) {
        LOG_ERROR("[history] loadMeta parse failed: unknown error path=" + meta_path);
        return false;
    }
}

bool FileHistoryManager::saveMeta(const std::string& dir, int latest_version,
                                  const std::vector<VersionMeta>& versions) const {
    json j;
    j["latest_version"] = latest_version;
    j["versions"] = json::array();
    for (const auto& v : versions) {
        j["versions"].push_back(
            {{"version", v.version}, {"timestamp", v.timestamp}, {"critical", v.critical}});
    }

    std::ofstream out(joinPath(dir, "meta.json"));
    if (!out.is_open()) {
        return false;
    }
    out << j.dump(2);
    return true;
}

bool FileHistoryManager::readLinesFromFile(const std::string& path,
                                           std::vector<std::string>& out) const {
    out.clear();
    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        out.push_back(line);
    }

    if (out.empty()) {
        out.push_back("");
    }

    return true;
}

bool FileHistoryManager::writeLinesToFile(const std::string& path,
                                          const std::vector<std::string>& lines) const {
    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }

    for (size_t i = 0; i < lines.size(); ++i) {
        out << lines[i];
        if (i + 1 < lines.size()) {
            out << "\n";
        }
    }

    return true;
}

bool FileHistoryManager::writeDiff(const std::string& path,
                                   const std::vector<diff::DiffRecord>& diff_records) const {
    json arr = json::array();
    for (const auto& r : diff_records) {
        std::string op = "equal";
        if (r.op == diff::DiffRecord::OpType::ADD)
            op = "add";
        else if (r.op == diff::DiffRecord::OpType::DELETE)
            op = "delete";

        arr.push_back({{"line_num", r.line_num}, {"op", op}, {"content", r.content}});
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        return false;
    }
    out << arr.dump();
    return true;
}

bool FileHistoryManager::readDiff(const std::string& path,
                                  std::vector<diff::DiffRecord>& out) const {
    out.clear();

    std::ifstream in(path);
    if (!in.is_open()) {
        return false;
    }

    try {
        json arr;
        in >> arr;
        for (const auto& item : arr) {
            diff::DiffRecord rec;
            rec.line_num = item.value("line_num", 0);
            std::string op = item.value("op", "equal");
            if (op == "add")
                rec.op = diff::DiffRecord::OpType::ADD;
            else if (op == "delete")
                rec.op = diff::DiffRecord::OpType::DELETE;
            else
                rec.op = diff::DiffRecord::OpType::NO_CHANGE;
            rec.content = item.value("content", "");
            out.push_back(std::move(rec));
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool FileHistoryManager::recordVersion(const std::string& file_path,
                                       const std::vector<std::string>& lines) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (file_path.empty()) {
        LOG_WARNING("[history] recordVersion skipped: empty file_path");
        return false;
    }

    const std::string dir = getFileHistoryDir(file_path);
    LOG("[history] recordVersion begin path=" + file_path + " hash=" + getFileHash(file_path) +
        " dir=" + dir + " lines=" + std::to_string(lines.size()));

    if (!ensureHistoryDir(dir)) {
        LOG_ERROR("[history] ensureHistoryDir failed dir=" + dir);
        return false;
    }

    int latest = 0;
    std::vector<VersionMeta> versions;
    bool has_meta = loadMeta(dir, latest, versions);
    LOG("[history] meta has_meta=" + std::string(has_meta ? "true" : "false") +
        " latest=" + std::to_string(latest) + " versions=" + std::to_string(versions.size()));

    long long now_ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                           std::chrono::system_clock::now().time_since_epoch())
                           .count();

    if (!has_meta || latest <= 0) {
        std::string bpath = basePath(dir);
        if (!writeLinesToFile(bpath, lines)) {
            LOG_ERROR("[history] write base failed path=" + bpath);
            return false;
        }
        versions.push_back({1, now_ts, true});
        bool ok = saveMeta(dir, 1, versions);
        LOG(std::string("[history] create v1 ") + (ok ? "ok" : "failed") + " file=" + file_path);
        return ok;
    }

    std::vector<std::string> prev;
    if (!readLinesFromFile(basePath(dir), prev)) {
        LOG_ERROR("[history] read base failed path=" + basePath(dir));
        return false;
    }
    for (int v = 1; v < latest; ++v) {
        std::vector<diff::DiffRecord> records;
        const std::string dpath = diffPath(dir, v, v + 1);
        if (!readDiff(dpath, records)) {
            LOG_ERROR("[history] read diff failed path=" + dpath);
            return false;
        }
        prev = diff::MyersDiff::applyForward(prev, records);
    }

    auto diff_records = diff::MyersDiff::compute(prev, lines);

    int changed = 0;
    for (const auto& rec : diff_records) {
        if (rec.op != diff::DiffRecord::OpType::NO_CHANGE)
            changed++;
    }
    int base_size = static_cast<int>(std::max(prev.size(), lines.size()));
    int change_percent = (base_size > 0) ? (changed * 100 / base_size) : 0;

    bool critical = false;
    if (retention_config_.keep_critical_versions) {
        if (change_percent >= retention_config_.critical_change_threshold) {
            critical = true;
        } else if (!versions.empty()) {
            long long interval_sec = (now_ts - versions.back().timestamp) / 1000LL;
            if (interval_sec >= retention_config_.critical_time_interval) {
                critical = true;
            }
        }
    }

    int next_ver = latest + 1;
    const std::string out_diff = diffPath(dir, latest, next_ver);
    if (!writeDiff(out_diff, diff_records)) {
        LOG_ERROR("[history] write diff failed path=" + out_diff);
        return false;
    }

    versions.push_back({next_ver, now_ts, critical});

    cleanupHistory(dir, next_ver, versions);

    bool ok = saveMeta(dir, next_ver, versions);
    LOG(std::string("[history] append v") + std::to_string(next_ver) + (ok ? " ok" : " failed") +
        " diff_records=" + std::to_string(diff_records.size()) +
        " critical=" + std::string(critical ? "true" : "false"));
    return ok;
}

std::vector<VersionMeta> FileHistoryManager::listVersions(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    int latest = 0;
    std::vector<VersionMeta> versions;
    const std::string dir = getFileHistoryDir(file_path);
    bool ok = loadMeta(dir, latest, versions);

    std::sort(versions.begin(), versions.end(), [](const VersionMeta& a, const VersionMeta& b) {
        return a.timestamp > b.timestamp;
    });

    LOG("[history] listVersions path=" + file_path + " dir=" + dir +
        " loadMeta=" + std::string(ok ? "ok" : "failed") + " latest=" + std::to_string(latest) +
        " count=" + std::to_string(versions.size()));

    return versions;
}

bool FileHistoryManager::restoreVersion(const std::string& file_path, int version,
                                        std::vector<std::string>& out) {
    std::lock_guard<std::mutex> lock(mutex_);

    const std::string dir = getFileHistoryDir(file_path);

    int latest = 0;
    std::vector<VersionMeta> versions;
    if (!loadMeta(dir, latest, versions)) {
        return false;
    }

    if (version <= 0 || version > latest) {
        return false;
    }

    std::vector<std::string> current;
    if (!readLinesFromFile(basePath(dir), current)) {
        return false;
    }

    if (version == 1) {
        out = std::move(current);
        return true;
    }

    for (int v = 1; v < version; ++v) {
        std::vector<diff::DiffRecord> records;
        if (!readDiff(diffPath(dir, v, v + 1), records)) {
            return false;
        }
        current = diff::MyersDiff::applyForward(current, records);
    }

    out = std::move(current);
    return true;
}

bool FileHistoryManager::diffBetweenVersions(const std::string& file_path, int from_version,
                                             int to_version, std::vector<diff::DiffRecord>& out) {
    std::vector<std::string> from_lines;
    std::vector<std::string> to_lines;

    if (!restoreVersion(file_path, from_version, from_lines)) {
        return false;
    }
    if (!restoreVersion(file_path, to_version, to_lines)) {
        return false;
    }

    out = diff::MyersDiff::compute(from_lines, to_lines);
    return true;
}

} // namespace history
} // namespace features
} // namespace pnana
