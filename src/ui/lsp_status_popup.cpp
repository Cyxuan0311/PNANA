#include "ui/lsp_status_popup.h"
#include "ui/icons.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <thread>

using namespace ftxui;

namespace pnana {
namespace ui {

static inline Decorator borderWithColor(Color c) {
    return [=](Element child) {
        return child | border | color(c);
    };
}

LspStatusPopup::LspStatusPopup(Theme& theme) : theme_(theme) {}

void LspStatusPopup::setPidProvider(std::function<int(const std::string&)> provider) {
    pid_provider_ = std::move(provider);
}

void LspStatusPopup::setStatusProvider(
    std::function<std::vector<features::LspStatusEntry>()> provider) {
    status_provider_ = std::move(provider);
}

void LspStatusPopup::open() {
    is_open_ = true;
    refreshEntries();
    selected_index_ = 0;
    scroll_offset_ = 0;
    // 启动后台内存监测线程
    mem_thread_running_.store(true);
    mem_thread_ = std::thread([this]() {
        std::unique_lock<std::mutex> lock(mem_mutex_, std::defer_lock);
        int prev_pid = -1;
        unsigned long long prev_proc_jiffies = 0;
        unsigned long long prev_total_jiffies = 0;
        while (mem_thread_running_.load()) {
            // 获取当前选中语言 id（在 entries_mutex_ 下读取以避免数据竞争）
            std::string lang_id;
            {
                std::lock_guard<std::mutex> g(entries_mutex_);
                size_t idx = selected_index_;
                if (idx < entries_.size()) {
                    lang_id = entries_[idx].config.language_id;
                }
            }

            int pid = -1;
            if (pid_provider_ && !lang_id.empty()) {
                try {
                    pid = pid_provider_(lang_id);
                } catch (...) {
                    pid = -1;
                }
            }

            std::string mem = "(n/a)";
            std::string cpu = "(n/a)";
            if (pid > 0) {
                // 读取 /proc/<pid>/status，查找 VmRSS
                std::ostringstream path;
                path << "/proc/" << pid << "/status";
                std::ifstream f(path.str());
                if (f) {
                    std::string line;
                    while (std::getline(f, line)) {
                        if (line.rfind("VmRSS:", 0) == 0) {
                            // 格式: VmRSS:\t  123456 kB
                            std::istringstream iss(line);
                            std::string key, value, unit;
                            iss >> key >> value >> unit;
                            try {
                                long kb = std::stol(value);
                                std::ostringstream ms;
                                if (kb >= 1024 * 1024) {
                                    double gb = static_cast<double>(kb) / (1024.0 * 1024.0);
                                    ms.setf(std::ios::fixed);
                                    ms.precision(2);
                                    ms << gb << " GB";
                                } else if (kb >= 1024) {
                                    double mb = static_cast<double>(kb) / 1024.0;
                                    ms.setf(std::ios::fixed);
                                    ms.precision(1);
                                    ms << mb << " MB";
                                } else {
                                    ms << kb << " kB";
                                }
                                mem = ms.str();
                                break;
                            } catch (...) {
                                // ignore parse error
                            }
                        }
                    }
                } else {
                    mem = "(not running)";
                }

                // 计算 CPU 使用率：读取 /proc/<pid>/stat 的 utime(14) + stime(15)
                // 以及 /proc/stat 第一行的总 jiffies，计算差值比例
                unsigned long long proc_jiffies = 0;
                std::ostringstream stat_path;
                stat_path << "/proc/" << pid << "/stat";
                std::ifstream pf(stat_path.str());
                if (pf) {
                    std::string stat_line;
                    if (std::getline(pf, stat_line)) {
                        // parse fields; utime is 14, stime is 15 (1-based)
                        std::istringstream iss(stat_line);
                        std::string token;
                        // pid and comm (comm may contain spaces inside parentheses)
                        iss >> token; // pid
                        // read comm (may contain spaces) up to last ')'
                        std::string comm;
                        iss >> comm;
                        while (!comm.empty() && comm.front() != '(')
                            break;
                        // continue reading until token ends with ')'
                        while (!comm.empty() && comm.back() != ')') {
                            std::string part;
                            iss >> part;
                            comm += " " + part;
                        }

                        // now read fields until 13 more to reach utime
                        for (int i = 0; i < 11; ++i) {
                            if (!(iss >> token)) {
                                token = "0";
                            }
                        }
                        unsigned long utime = 0, stime = 0;
                        if (!(iss >> utime))
                            utime = 0;
                        if (!(iss >> stime))
                            stime = 0;
                        proc_jiffies = static_cast<unsigned long long>(utime) +
                                       static_cast<unsigned long long>(stime);
                    }
                }

                // 读取系统总 jiffies
                unsigned long long total_jiffies = 0;
                std::ifstream sf("/proc/stat");
                if (sf) {
                    std::string cpu_line;
                    if (std::getline(sf, cpu_line)) {
                        std::istringstream cist(cpu_line);
                        std::string cpu_label;
                        cist >> cpu_label; // "cpu"
                        unsigned long long v;
                        while (cist >> v) {
                            total_jiffies += v;
                        }
                    }
                }

                if (prev_pid == pid && prev_total_jiffies > 0 && prev_proc_jiffies > 0 &&
                    total_jiffies > prev_total_jiffies) {
                    unsigned long long delta_proc = 0;
                    unsigned long long delta_total = total_jiffies - prev_total_jiffies;
                    if (proc_jiffies >= prev_proc_jiffies) {
                        delta_proc = proc_jiffies - prev_proc_jiffies;
                    }
                    double cpu_percent = 0.0;
                    if (delta_total > 0) {
                        cpu_percent = 100.0 * static_cast<double>(delta_proc) /
                                      static_cast<double>(delta_total);
                    }
                    std::ostringstream cpu_ms;
                    cpu_ms.setf(std::ios::fixed);
                    cpu_ms.precision(1);
                    cpu_ms << cpu_percent << " %";
                    cpu = cpu_ms.str();
                }

                // 保存当前为 prev，供下一次计算
                prev_pid = pid;
                prev_proc_jiffies = proc_jiffies;
                prev_total_jiffies = total_jiffies;
            }

            {
                std::lock_guard<std::mutex> g(mem_mutex_);
                mem_text_ = mem;
                cpu_text_ = cpu;
            }

            // 等待或被唤醒（例如选中项变化）
            std::unique_lock<std::mutex> cv_lock(mem_mutex_);
            mem_cv_.wait_for(cv_lock, std::chrono::seconds(1));
        }
    });
}

void LspStatusPopup::close() {
    is_open_ = false;
    // 停止并 join 内存监测线程
    mem_thread_running_.store(false);
    mem_cv_.notify_one();
    if (mem_thread_.joinable()) {
        mem_thread_.join();
    }
}

void LspStatusPopup::refreshEntries() {
    if (status_provider_) {
        auto new_entries = status_provider_();
        std::lock_guard<std::mutex> g(entries_mutex_);
        entries_ = std::move(new_entries);
        if (entries_.empty()) {
            selected_index_ = 0;
            scroll_offset_ = 0;
        } else if (selected_index_ >= entries_.size()) {
            selected_index_ = entries_.size() - 1;
            if (selected_index_ < scroll_offset_)
                scroll_offset_ = selected_index_;
        }
    } else {
        std::lock_guard<std::mutex> g(entries_mutex_);
        entries_.clear();
        selected_index_ = 0;
        scroll_offset_ = 0;
    }
}

bool LspStatusPopup::handleInput(Event event) {
    if (!is_open_)
        return false;
    if (event == Event::Escape) {
        close();
        return true;
    }
    if (event == Event::ArrowUp) {
        {
            std::lock_guard<std::mutex> g(entries_mutex_);
            if (selected_index_ > 0) {
                selected_index_--;
                if (selected_index_ < scroll_offset_)
                    scroll_offset_ = selected_index_;
            }
        }
        mem_cv_.notify_one();
        return true;
    }
    if (event == Event::ArrowDown) {
        {
            std::lock_guard<std::mutex> g(entries_mutex_);
            if (!entries_.empty() && selected_index_ < entries_.size() - 1) {
                selected_index_++;
                if (selected_index_ >= scroll_offset_ + list_display_count_)
                    scroll_offset_ = selected_index_ - list_display_count_ + 1;
            }
        }
        mem_cv_.notify_one();
        return true;
    }
    return true;
}

Element LspStatusPopup::render() {
    if (!is_open_)
        return text("");
    refreshEntries();

    const auto& colors = theme_.getColors();
    Elements content;
    content.push_back(renderTitle());
    content.push_back(separator());
    content.push_back(hbox({
                          renderLeftList() | size(WIDTH, EQUAL, 52),
                          separator(),
                          renderRightDetail() | flex,
                      }) |
                      flex);
    content.push_back(separator());
    content.push_back(renderHelpBar());

    return window(text("LSP Connection Status"), vbox(content)) | size(WIDTH, EQUAL, 120) |
           size(HEIGHT, EQUAL, 28) | bgcolor(colors.background) |
           borderWithColor(colors.dialog_border);
}

Element LspStatusPopup::renderTitle() const {
    const auto& colors = theme_.getColors();
    return hbox({text(" "), text(icons::REFRESH) | color(colors.function),
                 text(" LSP — Servers & Connection "), text(" ")}) |
           bold | bgcolor(colors.current_line) | color(colors.foreground) | center;
}

Element LspStatusPopup::renderLeftList() const {
    const auto& colors = theme_.getColors();
    Elements lines;
    size_t start = 0;
    size_t end = 0;
    size_t local_selected = 0;
    {
        std::lock_guard<std::mutex> g(entries_mutex_);
        start = scroll_offset_;
        end = std::min(start + list_display_count_, entries_.size());
        local_selected = selected_index_;
    }

    for (size_t i = start; i < end; ++i) {
        features::LspStatusEntry e;
        {
            std::lock_guard<std::mutex> g(entries_mutex_);
            e = entries_[i];
        }
        bool selected = (i == local_selected);
        std::string status = e.connected ? "●" : "○";
        std::string label = e.config.language_id + " | " + e.config.name + " " + status;
        Element row = text("  " + label);
        if (selected) {
            row = row | bgcolor(colors.function) | color(colors.background) | bold;
        } else {
            row = row | color(e.connected ? colors.success : colors.comment);
        }
        lines.push_back(row);
    }
    if (lines.empty()) {
        lines.push_back(text("  No LSP servers configured") | color(colors.comment) | dim);
    }
    return vbox(lines) | bgcolor(colors.background) | yframe;
}

Element LspStatusPopup::renderRightDetail() const {
    const auto& colors = theme_.getColors();
    Elements block;

    size_t local_selected = 0;
    bool local_empty = false;
    features::LspStatusEntry local_entry;
    {
        std::lock_guard<std::mutex> g(entries_mutex_);
        if (entries_.empty()) {
            local_empty = true;
        } else if (selected_index_ >= entries_.size()) {
            local_empty = true;
        } else {
            local_selected = selected_index_;
            local_entry = entries_[local_selected];
        }
    }

    if (local_empty) {
        block.push_back(text("  No servers to show.") | color(colors.comment) | dim);
        return vbox(block) | bgcolor(colors.background) | yframe;
    }

    const auto& e = local_entry;
    const auto& c = e.config;

    auto line = [&](const std::string& key, const std::string& val) {
        return hbox({text("  " + key + ": ") | color(colors.comment) | size(WIDTH, EQUAL, 14),
                     text(val) | color(colors.foreground)});
    };

    block.push_back(line("Name", c.name));
    block.push_back(line("Command", c.command));
    block.push_back(line("Language ID", c.language_id));
    block.push_back(line("Status", e.connected ? "Connected" : "Not connected"));

    // Memory: 使用异步更新的缓存文本，避免阻塞渲染
    std::string mem_display;
    std::string cpu_display;
    {
        std::lock_guard<std::mutex> g(mem_mutex_);
        mem_display = mem_text_.empty() ? "(n/a)" : mem_text_;
        cpu_display = cpu_text_.empty() ? "(n/a)" : cpu_text_;
    }
    block.push_back(line("Memory", mem_display));
    block.push_back(line("CPU", cpu_display));

    std::string exts;
    for (auto it = c.file_extensions.begin(); it != c.file_extensions.end(); ++it) {
        if (it != c.file_extensions.begin())
            exts += ", ";
        exts += *it;
    }
    block.push_back(line("Extensions", exts.empty() ? "(none)" : exts));

    std::string args_str;
    for (size_t i = 0; i < c.args.size(); ++i) {
        if (i)
            args_str += " ";
        args_str += c.args[i];
    }
    block.push_back(line("Args", args_str.empty() ? "(none)" : args_str));

    if (!c.env_vars.empty()) {
        block.push_back(text("  Env:") | color(colors.comment));
        for (const auto& [k, v] : c.env_vars) {
            block.push_back(hbox({text("    " + k + "=") | color(colors.keyword),
                                  text(v) | color(colors.foreground)}));
        }
    }

    return vbox(block) | bgcolor(colors.background) | yframe;
}

Element LspStatusPopup::renderHelpBar() const {
    const auto& colors = theme_.getColors();
    return hbox({text("  ↑↓") | color(colors.keyword) | bold, text(": Select  "),
                 text("Esc") | color(colors.keyword) | bold, text(": Close")}) |
           color(colors.comment) | dim;
}

} // namespace ui
} // namespace pnana
