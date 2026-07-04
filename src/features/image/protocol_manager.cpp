#include "features/image/protocol_manager.h"
#include "utils/logger.h"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

// 终端字符单元像素尺寸（默认值，可通过 queryCellPixelSize 动态获取）
static int s_cell_w_px = 8;  // pixels per terminal column
static int s_cell_h_px = 16; // pixels per terminal row
static bool s_cell_size_queried = false;

// 动态获取终端 cell 像素尺寸
// 使用 XTVERSION/XTWINOPS 转义序列：
//   \033[16t  → 终端回复 \033[6;height;widtht  (cell height/width in pixels)
//   \033[18t  → 终端回复 \033[8;rows;colst     (terminal size in cells)
//   \033[14t  → 终端回复 \033[4;height;widtht   (window size in pixels)
// 通过 窗口像素尺寸 / 终端cell数 = cell像素尺寸
static void queryCellPixelSize() {
    if (s_cell_size_queried)
        return;
    s_cell_size_queried = true;

    int fd = ::open("/dev/tty", O_RDWR);
    if (fd < 0)
        return;

    // 保存原始终端属性
    struct termios orig_tios;
    if (::tcgetattr(fd, &orig_tios) != 0) {
        ::close(fd);
        return;
    }

    // 设置为 raw 模式以读取响应
    struct termios raw = orig_tios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1; // 100ms timeout
    if (::tcsetattr(fd, TCSAFLUSH, &raw) != 0) {
        ::close(fd);
        return;
    }

    // 请求 cell 像素尺寸: \033[16t
    // 有些终端支持直接返回 cell 尺寸
    const char* req_cell = "\033[16t";
    (void)::write(fd, req_cell, std::strlen(req_cell));

    // 读取响应 (最多等待 200ms)
    char buf[64] = {};
    ssize_t n = 0;
    for (int attempt = 0; attempt < 10 && n < 63; ++attempt) {
        ssize_t r = ::read(fd, buf + n, 63 - n);
        if (r <= 0)
            break;
        n += r;
        // 检查是否收到完整响应 (以 't' 结尾)
        if (n > 0 && buf[n - 1] == 't')
            break;
    }

    // 解析响应: \033[6;height;widtht
    int cell_h = 0, cell_w = 0;
    if (n > 0) {
        // 查找 \033[6; 模式
        for (int i = 0; i < n - 3; ++i) {
            if ((unsigned char)buf[i] == 033 && buf[i + 1] == '[' && buf[i + 2] == '6' &&
                buf[i + 3] == ';') {
                // 找到，解析 height;widtht
                char* end = nullptr;
                cell_h = std::strtol(&buf[i + 4], &end, 10);
                if (end && *end == ';') {
                    cell_w = std::strtol(end + 1, nullptr, 10);
                }
                break;
            }
        }
    }

    if (cell_h > 0 && cell_w > 0) {
        s_cell_h_px = cell_h;
        s_cell_w_px = cell_w;
        LOG_DEBUG("[Proto] cell pixel size from terminal: " + std::to_string(s_cell_w_px) + "x" +
                  std::to_string(s_cell_h_px));
    } else {
        // 备选方案：请求窗口像素尺寸 + cell 数量来计算
        // 清空输入缓冲
        ::tcflush(fd, TCIFLUSH);

        const char* req_pixels = "\033[14t";
        (void)::write(fd, req_pixels, std::strlen(req_pixels));

        n = 0;
        for (int attempt = 0; attempt < 10 && n < 63; ++attempt) {
            ssize_t r = ::read(fd, buf + n, 63 - n);
            if (r <= 0)
                break;
            n += r;
            if (n > 0 && buf[n - 1] == 't')
                break;
        }

        int win_h_px = 0, win_w_px = 0;
        if (n > 0) {
            for (int i = 0; i < n - 3; ++i) {
                if ((unsigned char)buf[i] == 033 && buf[i + 1] == '[' && buf[i + 2] == '4' &&
                    buf[i + 3] == ';') {
                    char* end = nullptr;
                    win_h_px = std::strtol(&buf[i + 4], &end, 10);
                    if (end && *end == ';') {
                        win_w_px = std::strtol(end + 1, nullptr, 10);
                    }
                    break;
                }
            }
        }

        if (win_h_px > 0 && win_w_px > 0) {
            // 获取终端 cell 数量
            struct winsize ws;
            if (::ioctl(fd, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
                s_cell_w_px = win_w_px / ws.ws_col;
                s_cell_h_px = win_h_px / ws.ws_row;
                if (s_cell_w_px < 1)
                    s_cell_w_px = 8;
                if (s_cell_h_px < 1)
                    s_cell_h_px = 16;
                LOG_DEBUG(
                    "[Proto] cell pixel size from window/cells: " + std::to_string(s_cell_w_px) +
                    "x" + std::to_string(s_cell_h_px) + " (window=" + std::to_string(win_w_px) +
                    "x" + std::to_string(win_h_px) + " cells=" + std::to_string(ws.ws_col) + "x" +
                    std::to_string(ws.ws_row) + ")");
            }
        } else {
            LOG_DEBUG("[Proto] cell pixel size query failed, using defaults: " +
                      std::to_string(s_cell_w_px) + "x" + std::to_string(s_cell_h_px));
        }
    }

    ::tcsetattr(fd, TCSAFLUSH, &orig_tios);
    ::close(fd);
}

#include "dsa/stb_image.h"
#include "dsa/stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include "dsa/stb_image_write.h"
#pragma GCC diagnostic pop

#include <sixel.h>

namespace pnana {
namespace features {

// static members
TerminalInfo ProtocolManager::s_info;
bool ProtocolManager::s_detected = false;
bool ProtocolManager::s_protocol_enabled = true;
std::string ProtocolManager::s_preferred = "auto";
int ProtocolManager::s_active_protocol = 0;

ProtocolImageData ProtocolManager::s_pending_data;
int ProtocolManager::s_pending_row = 0;
int ProtocolManager::s_pending_col = 0;
bool ProtocolManager::s_has_pending = false;

std::string ProtocolManager::s_cache_key;
ProtocolImageData ProtocolManager::s_cache_data;

static int tty_fd = -1;
static int ensureTTY() {
    if (tty_fd < 0) {
        tty_fd = ::open("/dev/tty", O_WRONLY);
    }
    return tty_fd;
}

// Safe write helper: suppress -Wunused-result by assigning to a variable
static void writeAll(int fd, const void* buf, size_t count) {
    ssize_t unused = ::write(fd, buf, count);
    (void)unused;
}
static void writeAll(int fd, const std::string& s) {
    writeAll(fd, s.data(), s.size());
}

// ========== public API ==========

void ProtocolManager::init() {
    s_detected = false;
    detect();
    queryCellPixelSize();
}

void ProtocolManager::detect() {
    if (s_detected)
        return;
    if (!s_protocol_enabled) {
        s_active_protocol = 0;
        return;
    }
    auto info = getTerminalInfo();
    if (info.kitty) {
        s_active_protocol = 1;
    } else if (info.iterm2) {
        s_active_protocol = 2;
    } else if (info.sixel) {
        s_active_protocol = 3;
    } else {
        s_active_protocol = 0;
    }
}

bool ProtocolManager::isActive() {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    if (!s_detected)
        detect();
    return s_protocol_enabled && s_active_protocol > 0;
#else
    return false;
#endif
}

std::string ProtocolManager::protocolName() {
    switch (s_active_protocol) {
        case 1:
            return "Kitty";
        case 2:
            return "iTerm2";
        case 3:
            return "Sixel";
        default:
            return "(none)";
    }
}

int ProtocolManager::getCellWidthPx() {
    return s_cell_w_px;
}
int ProtocolManager::getCellHeightPx() {
    return s_cell_h_px;
}

bool ProtocolManager::encodeImage(const std::string& filepath, int pixel_w, int pixel_h,
                                  ProtocolImageData& out) {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    if (!isActive())
        return false;
    // Check cache first
    if (getCached(filepath, pixel_w, pixel_h, out))
        return true;
    bool ok = false;
    switch (s_active_protocol) {
        case 1:
            ok = encodeKitty(filepath, pixel_w, pixel_h, out);
            break;
        case 2:
            ok = encodeITerm2(filepath, pixel_w, pixel_h, out);
            break;
        case 3:
            ok = encodeSixel(filepath, pixel_w, pixel_h, out);
            break;
    }
    if (ok)
        cacheResult(filepath, pixel_w, pixel_h, out);
    return ok;
#endif
    return false;
}

void ProtocolManager::writeToTerminal(const ProtocolImageData& data, int term_row, int term_col) {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    int fd = ensureTTY();
    if (fd < 0) {
        LOG_DEBUG("[Proto] writeToTerminal FAILED: no tty fd");
        return;
    }
    std::string seq = "\033[" + std::to_string(term_row) + ";" + std::to_string(term_col) + "H";
    writeAll(fd, seq);
    writeAll(fd, data.data);
    ::fsync(fd);
#endif
}

void ProtocolManager::clearArea(int start_row, int num_rows) {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    int fd = ensureTTY();
    if (fd < 0)
        return;
    for (int i = 0; i < num_rows; ++i) {
        std::string seq = "\033[" + std::to_string(start_row + i) + ";1H\033[K";
        writeAll(fd, seq);
    }
    ::fsync(fd);
#endif
}

bool ProtocolManager::isEnabled() {
    return s_protocol_enabled;
}
void ProtocolManager::setEnabled(bool on) {
    s_protocol_enabled = on;
    s_detected = false;
    s_has_pending = false;
    if (on)
        detect();
    else
        s_active_protocol = 0;
}
std::string ProtocolManager::getPreferred() {
    return s_preferred;
}
void ProtocolManager::setPreferred(const std::string& pref) {
    s_preferred = pref;
    s_detected = false;
    detect();
}

int ProtocolManager::getActiveProtocol() {
    return s_active_protocol;
}

const TerminalInfo& ProtocolManager::getTerminalInfo() {
    if (!s_detected) {
        s_info = TerminalInfo{};
        std::string name = detectTerminalByProcessTree();
        if (name.empty())
            name = detectTerminalByEnv();
        if (!name.empty()) {
            s_info = nameToCapabilities(name);
            s_info.name = name;
        }
        s_detected = true;
    }
    return s_info;
}

std::vector<std::string> ProtocolManager::getAvailableProtocols() {
    return {"kitty", "iterm2", "sixel"};
}

std::string ProtocolManager::getActiveProtocolName() {
    return protocolName();
}

bool ProtocolManager::flushPending() {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    if (!s_has_pending || !s_pending_data.valid)
        return false;
    writeToTerminal(s_pending_data, s_pending_row, s_pending_col);
    s_has_pending = false;
    return true;
#else
    return false;
#endif
}

bool ProtocolManager::hasPending() {
    return s_has_pending;
}

void ProtocolManager::setPending(const ProtocolImageData& data, int term_row, int term_col) {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    s_pending_data = data;
    s_pending_row = term_row;
    s_pending_col = term_col;
    s_has_pending = true;
#endif
}

void ProtocolManager::updatePendingPosition(int term_row, int term_col) {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    s_pending_row = term_row;
    s_pending_col = term_col;
#endif
}

void ProtocolManager::resendLast() {
#ifdef BUILD_IMAGE_PROTOCOL_SUPPORT
    if (s_pending_data.valid) {
        writeToTerminal(s_pending_data, s_pending_row, s_pending_col);
    }
#endif
}

void ProtocolManager::cacheResult(const std::string& filepath, int pixel_w, int pixel_h,
                                  const ProtocolImageData& data) {
    s_cache_key = filepath + "_" + std::to_string(pixel_w) + "x" + std::to_string(pixel_h);
    s_cache_data = data;
}

bool ProtocolManager::getCached(const std::string& filepath, int pixel_w, int pixel_h,
                                ProtocolImageData& out) {
    std::string key = filepath + "_" + std::to_string(pixel_w) + "x" + std::to_string(pixel_h);
    if (key != s_cache_key || !s_cache_data.valid)
        return false;
    out = s_cache_data;
    return true;
}

// ========== terminal detection (process tree) ==========

static std::string getProcessName(int pid) {
    // Try /proc/[pid]/comm (just the process name, no args)
    std::string path = "/proc/" + std::to_string(pid) + "/comm";
    FILE* f = std::fopen(path.c_str(), "r");
    if (f) {
        char buf[256];
        if (std::fgets(buf, sizeof(buf), f)) {
            std::fclose(f);
            size_t len = std::strlen(buf);
            while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r'))
                buf[--len] = '\0';
            return std::string(buf);
        }
        std::fclose(f);
    }

    // Fallback: /proc/[pid]/exe symlink (handles gnome-terminal-server, etc.)
    path = "/proc/" + std::to_string(pid) + "/exe";
    char buf[256];
    ssize_t len = ::readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (len > 0) {
        buf[len] = '\0';
        std::string exe(buf);
        size_t pos = exe.rfind('/');
        if (pos != std::string::npos)
            return exe.substr(pos + 1);
        return exe;
    }

    // Fallback: ps (macOS / other Unix)
    char cmd[64];
    std::snprintf(cmd, sizeof(cmd), "ps -o comm= -p %d 2>/dev/null", pid);
    FILE* pipe = ::popen(cmd, "r");
    if (pipe) {
        char buf2[256];
        if (std::fgets(buf2, sizeof(buf2), pipe)) {
            ::pclose(pipe);
            size_t len2 = std::strlen(buf2);
            while (len2 > 0 && (buf2[len2 - 1] == '\n' || buf2[len2 - 1] == '\r'))
                buf2[--len2] = '\0';
            return std::string(buf2);
        }
        ::pclose(pipe);
    }

    return {};
}

static int getParentPid(int pid) {
    // Try /proc/[pid]/status (Linux — robust against ) in comm)
    std::string path = "/proc/" + std::to_string(pid) + "/status";
    FILE* f = std::fopen(path.c_str(), "r");
    if (f) {
        char buf[256];
        while (std::fgets(buf, sizeof(buf), f)) {
            if (std::strncmp(buf, "PPid:", 5) == 0) {
                std::fclose(f);
                return std::atoi(buf + 5);
            }
        }
        std::fclose(f);
    }

    // Fallback: ps (macOS / other Unix)
    char cmd[64];
    std::snprintf(cmd, sizeof(cmd), "ps -o ppid= -p %d 2>/dev/null", pid);
    FILE* pipe = ::popen(cmd, "r");
    if (pipe) {
        char buf2[64];
        if (std::fgets(buf2, sizeof(buf2), pipe)) {
            ::pclose(pipe);
            return std::atoi(buf2);
        }
        ::pclose(pipe);
    }

    return 0;
}

static bool isShell(const std::string& lower) {
    return lower.find("bash") != std::string::npos || lower.find("zsh") != std::string::npos ||
           lower.find("fish") != std::string::npos || lower.find("dash") != std::string::npos ||
           lower == "sh" || (lower.find("nu") == 0);
}

static bool isSessionOrServer(const std::string& lower) {
    return lower.find("systemd") != std::string::npos || lower.find("sshd") != std::string::npos ||
           lower.find("login") != std::string::npos || lower.find("sudo") != std::string::npos ||
           lower.find("cron") != std::string::npos || lower.find("dbus") != std::string::npos ||
           lower.find("at-spi") != std::string::npos || lower.find("polkit") != std::string::npos ||
           lower.find("pipewire") != std::string::npos ||
           lower.find("wireplumber") != std::string::npos || lower.find("xdg") != std::string::npos;
}

static bool isMultiplexer(const std::string& lower) {
    return lower.find("tmux") != std::string::npos || lower.find("screen") != std::string::npos ||
           lower.find("byobu") != std::string::npos || lower.find("mosh") != std::string::npos;
}

std::string ProtocolManager::detectTerminalByProcessTree() {
    int pid = ::getppid();
    int max_depth = 30;

    while (pid > 1 && max_depth-- > 0) {
        std::string name = getProcessName(pid);
        if (name.empty())
            break;

        std::string lower = name;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        // --- Match known terminal emulators ---
        if (lower.find("kitty") != std::string::npos)
            return "kitty";
        if (lower.find("wezterm") != std::string::npos)
            return "wezterm";
        if (lower.find("iterm") != std::string::npos)
            return "iterm2";
        if (lower == "foot" || lower.find("foot-client") != std::string::npos)
            return "foot";
        if (lower.find("alacritty") != std::string::npos)
            return "alacritty";
        if (lower.find("contour") != std::string::npos)
            return "contour";
        if (lower == "xterm" || lower.find("xterm-") == 0)
            return "xterm";
        if (lower.find("gnome-terminal") != std::string::npos)
            return "gnome-terminal";
        if (lower.find("konsole") != std::string::npos)
            return "konsole";
        if (lower.find("mintty") != std::string::npos)
            return "mintty";
        if (lower.find("rxvt") != std::string::npos)
            return "rxvt";
        if (lower == "st" || lower.find("st-term") != std::string::npos)
            return "st";
        if (lower.find("tilix") != std::string::npos)
            return "tilix";
        if (lower.find("ghostty") != std::string::npos)
            return "ghostty";
        if (lower.find("rio") != std::string::npos)
            return "rio";
        if (lower.find("blackbox") != std::string::npos)
            return "blackbox";
        if (lower.find("terminology") != std::string::npos)
            return "terminology";
        if (lower.find("sakura") != std::string::npos)
            return "sakura";
        if (lower.find("lxterminal") != std::string::npos)
            return "lxterminal";
        if (lower.find("mate-terminal") != std::string::npos)
            return "mate-terminal";
        if (lower.find("qterminal") != std::string::npos)
            return "qterminal";
        if (lower.find("guake") != std::string::npos)
            return "guake";
        if (lower.find("yakuake") != std::string::npos)
            return "yakuake";
        if (lower.find("kmscon") != std::string::npos)
            return "kmscon";
        if (lower.find("lagrange") != std::string::npos)
            return "lagrange";

        // --- Skip known non-terminal processes and continue walking up ---
        if (isShell(lower) || isSessionOrServer(lower) || isMultiplexer(lower)) {
            pid = getParentPid(pid);
            continue;
        }

        // Unknown process — keep walking up
        pid = getParentPid(pid);
    }

    return {};
}

// ========== terminal detection (env fallback) ==========

std::string ProtocolManager::detectTerminalByEnv() {
    // KITTY_WINDOW_ID — reliable indicator
    const char* kitty_window = std::getenv("KITTY_WINDOW_ID");
    if (kitty_window && kitty_window[0] != '\0')
        return "kitty";

    // TERM_PROGRAM — set by iTerm2, WezTerm, kitty
    const char* term_prog = std::getenv("TERM_PROGRAM");
    if (term_prog) {
        std::string tp(term_prog);
        if (tp == "iTerm.app" || tp == "iTerm2")
            return "iterm2";
        if (tp.find("WezTerm") != std::string::npos)
            return "wezterm";
        if (tp == "kitty")
            return "kitty";
    }

    // WT_SESSION — Windows Terminal (common in WSL)
    const char* wt = std::getenv("WT_SESSION");
    if (wt && wt[0] != '\0')
        return "windows-terminal";

    // TERM — check for xterm-kitty
    const char* term = std::getenv("TERM");
    if (term) {
        std::string t(term);
        if (t == "xterm-kitty")
            return "kitty";
    }

    return {};
}

// ========== name → capabilities ==========

TerminalInfo ProtocolManager::nameToCapabilities(const std::string& name) {
    TerminalInfo info{};

    if (name == "kitty") {
        info.kitty = true;
        info.sixel = true;
    } else if (name == "wezterm") {
        info.kitty = true;
        info.sixel = true;
        info.iterm2 = true;
    } else if (name == "ghostty") {
        info.kitty = true;
        info.sixel = true;
    } else if (name == "foot" || name == "contour" || name == "alacritty" || name == "xterm" ||
               name == "windows-terminal") {
        info.sixel = true;
    } else if (name == "iterm2") {
        info.iterm2 = true;
    } else if (name == "terminology" || name == "st" || name == "rio" || name == "blackbox" ||
               name == "kmscon") {
        info.sixel = true;
    } else if (name == "gnome-terminal" || name == "konsole" || name == "mate-terminal" ||
               name == "lxterminal" || name == "qterminal" || name == "tilix" || name == "sakura" ||
               name == "guake" || name == "yakuake" || name == "mintty" || name == "rxvt" ||
               name == "lagrange") {
        // VTE-based or terminals with no known image protocol support
    }

    return info;
}

// ========== Kitty protocol encoding ==========

static constexpr size_t KITTY_CHUNK_SIZE = 4096;

bool ProtocolManager::encodeKitty(const std::string& filepath, int pixel_w, int /*pixel_h*/,
                                  ProtocolImageData& out) {
    LOG_DEBUG("[Proto] encodeKitty path=" + filepath + " pixel_w=" + std::to_string(pixel_w));
    int w = 0, h = 0, ch = 0;
    unsigned char* img = stbi_load(filepath.c_str(), &w, &h, &ch, STBI_rgb_alpha);
    if (!img) {
        LOG_DEBUG("[Proto] encodeKitty stbi_load FAILED");
        return false;
    }
    LOG_DEBUG("[Proto] encodeKitty loaded w=" + std::to_string(w) + " h=" + std::to_string(h) +
              " ch=" + std::to_string(ch));

    // resize if needed
    unsigned char* resized = img;
    int rw = w, rh = h;
    if (pixel_w > 0 && pixel_w != w) {
        float scale = static_cast<float>(pixel_w) / w;
        rw = pixel_w;
        rh = static_cast<int>(h * scale);
        if (rh < 1)
            rh = 1;
        resized = (unsigned char*)malloc(static_cast<size_t>(rw) * rh * 4);
        stbir_resize_uint8_srgb(img, w, h, 0, resized, rw, rh, 0, STBIR_RGBA);
    }

    // encode to PNG in memory
    int png_len = 0;
    unsigned char* png = stbi_write_png_to_mem(resized, 0, rw, rh, 4, &png_len);
    if (!png) {
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    std::string b64 = detail::Base64Encode(png, static_cast<size_t>(png_len));
    STBIW_FREE(png);

    // build APC chunks
    std::string result;
    size_t total = b64.size();
    size_t pos = 0;
    int chunk_idx = 0;
    while (pos < total) {
        size_t chunk_size = std::min(KITTY_CHUNK_SIZE, total - pos);
        std::string chunk = b64.substr(pos, chunk_size);
        bool more = (pos + chunk_size < total);
        result += "\033_G";
        if (chunk_idx == 0) {
            result += "a=T,f=100,s=" + std::to_string(rw) + ",v=" + std::to_string(rh) + ",";
        }
        result += "m=" + std::string(more ? "1" : "0") + ";";
        result += chunk;
        result += "\033\\";
        pos += chunk_size;
        ++chunk_idx;
    }

    out.data = result;
    out.term_rows = (rh + s_cell_h_px - 1) / s_cell_h_px;
    out.term_cols = (rw + s_cell_w_px - 1) / s_cell_w_px;
    out.render_w = rw;
    out.render_h = rh;
    out.valid = true;

    if (resized != img)
        free(resized);
    stbi_image_free(img);
    return true;
}

// ========== iTerm2 protocol encoding ==========

bool ProtocolManager::encodeITerm2(const std::string& filepath, int pixel_w, int /*pixel_h*/,
                                   ProtocolImageData& out) {
    int w = 0, h = 0, ch = 0;
    unsigned char* img = stbi_load(filepath.c_str(), &w, &h, &ch, STBI_rgb_alpha);
    if (!img)
        return false;

    unsigned char* resized = img;
    int rw = w, rh = h;
    if (pixel_w > 0 && pixel_w != w) {
        float scale = static_cast<float>(pixel_w) / w;
        rw = pixel_w;
        rh = static_cast<int>(h * scale);
        if (rh < 1)
            rh = 1;
        resized = (unsigned char*)malloc(static_cast<size_t>(rw) * rh * 4);
        stbir_resize_uint8_srgb(img, w, h, 0, resized, rw, rh, 0, STBIR_RGBA);
    }

    int png_len = 0;
    unsigned char* png = stbi_write_png_to_mem(resized, 0, rw, rh, 4, &png_len);
    if (!png) {
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    std::string b64 = detail::Base64Encode(png, static_cast<size_t>(png_len));
    STBIW_FREE(png);

    std::string result = "\033]1337;File=inline=1";
    result += ";size=" + std::to_string(png_len);
    result += ";width=" + std::to_string(rw) + "px";
    result += ";height=" + std::to_string(rh) + "px";
    result += ":" + b64;
    result += "\a";

    out.data = result;
    out.term_rows = (rh + s_cell_h_px - 1) / s_cell_h_px;
    out.term_cols = (rw + s_cell_w_px - 1) / s_cell_w_px;
    out.render_w = rw;
    out.render_h = rh;
    out.valid = true;

    if (resized != img)
        free(resized);
    stbi_image_free(img);
    return true;
}

// ========== Sixel protocol encoding ==========

static int sixel_write_to_string(char* data, int size, void* priv) {
    auto* str = static_cast<std::string*>(priv);
    str->append(data, static_cast<size_t>(size));
    return 0;
}

bool ProtocolManager::encodeSixel(const std::string& filepath, int pixel_w, int /*pixel_h*/,
                                  ProtocolImageData& out) {
    int w = 0, h = 0, ch = 0;
    unsigned char* img = stbi_load(filepath.c_str(), &w, &h, &ch, STBI_rgb_alpha);
    if (!img)
        return false;

    // Compute target size: scale to fit pixel_w wide, maintaining aspect ratio
    unsigned char* resized = img;
    int rw = w, rh = h;
    if (pixel_w > 0 && pixel_w != w) {
        float scale = static_cast<float>(pixel_w) / w;
        rw = pixel_w;
        rh = static_cast<int>(h * scale);
        if (rh < 1)
            rh = 1;
        resized = (unsigned char*)malloc(static_cast<size_t>(rw) * rh * 4);
        stbir_resize_uint8_srgb(img, w, h, 0, resized, rw, rh, 0, STBIR_RGBA);
    }

    std::string result;
    sixel_output_t* output = nullptr;
    sixel_dither_t* dither = nullptr;

    if (SIXEL_FAILED(sixel_output_new(&output, sixel_write_to_string, &result, nullptr))) {
        LOG_DEBUG("[Proto] sixel_output_new FAILED");
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    if (SIXEL_FAILED(sixel_dither_new(&dither, 256, nullptr))) {
        LOG_DEBUG("[Proto] sixel_dither_new FAILED");
        sixel_output_destroy(output);
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    if (SIXEL_FAILED(sixel_dither_initialize(dither, resized, rw, rh, SIXEL_PIXELFORMAT_RGBA8888,
                                             SIXEL_LARGE_AUTO, SIXEL_REP_AUTO,
                                             SIXEL_QUALITY_HIGH))) {
        LOG_DEBUG("[Proto] sixel_dither_initialize FAILED");
        sixel_dither_destroy(dither);
        sixel_output_destroy(output);
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    if (SIXEL_FAILED(sixel_encode(resized, rw, rh, 4, dither, output))) {
        LOG_DEBUG("[Proto] sixel_encode FAILED");
        sixel_dither_destroy(dither);
        sixel_output_destroy(output);
        if (resized != img)
            free(resized);
        stbi_image_free(img);
        return false;
    }

    sixel_dither_destroy(dither);
    sixel_output_destroy(output);
    if (resized != img)
        free(resized);
    stbi_image_free(img);

    out.data = std::move(result);
    out.term_rows = (rh + s_cell_h_px - 1) / s_cell_h_px;
    out.term_cols = (rw + s_cell_w_px - 1) / s_cell_w_px;
    out.render_w = rw;
    out.render_h = rh;
    out.valid = true;
    return true;
}

} // namespace features
} // namespace pnana
