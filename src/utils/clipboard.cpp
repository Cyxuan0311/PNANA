#include "utils/clipboard.h"
#include "utils/logger.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

namespace pnana {
namespace utils {

bool Clipboard::copyToSystem(const std::string& text) {
    if (text.empty()) {
        LOG_ERROR("[CLIPBOARD] copyToSystem: text is empty");
        return false;
    }

    std::string command = getCopyCommand();
    if (command.empty()) {
        LOG_ERROR(
            "[CLIPBOARD] copyToSystem: no copy command available (wl-copy or xclip not found)");
        return false;
    }

    LOG("[CLIPBOARD] copyToSystem: using command: " + command);
    LOG("[CLIPBOARD] copyToSystem: text length: " + std::to_string(text.length()));

    bool result = executeCommand(command, text);
    if (!result) {
        LOG_ERROR("[CLIPBOARD] copyToSystem: command execution failed");
    } else {
        LOG("[CLIPBOARD] copyToSystem: success");
    }

    return result;
}

std::string Clipboard::pasteFromSystem() {
    std::string command = getPasteCommand();
    if (command.empty()) {
        return "";
    }

    return executeCommandWithOutput(command);
}

bool Clipboard::isAvailable() {
    return !getCopyCommand().empty() && !getPasteCommand().empty();
}

std::string Clipboard::getCopyCommand() {
    static std::string cached_command;
    static bool checked = false;

    if (checked) {
        return cached_command;
    }

    checked = true;

#ifdef __APPLE__
    // macOS 使用 pbcopy
    cached_command = "pbcopy";
    LOG("[CLIPBOARD] getCopyCommand: macOS detected, using pbcopy");
    return cached_command;
#else
    // Linux: 优先使用 wl-clipboard (Wayland)，然后 xclip (X11)
    // 在 WSL2 中，也可以使用 Windows 的 clip.exe

    // 首先检查是否在 WSL2 环境中（通过检查 /proc/version 或 /mnt/c/Windows）
    bool is_wsl = false;
    std::ifstream version_file("/proc/version");
    if (version_file.is_open()) {
        std::string version_content((std::istreambuf_iterator<char>(version_file)),
                                    std::istreambuf_iterator<char>());
        if (version_content.find("Microsoft") != std::string::npos ||
            version_content.find("WSL") != std::string::npos) {
            is_wsl = true;
            LOG("[CLIPBOARD] getCopyCommand: WSL environment detected");
        }
        version_file.close();
    }

    // 如果在 WSL2 中，检查 Windows clip.exe
    if (is_wsl) {
        std::ifstream clip_file("/mnt/c/Windows/System32/clip.exe");
        if (clip_file.good()) {
            clip_file.close();
            cached_command = "/mnt/c/Windows/System32/clip.exe";
            LOG("[CLIPBOARD] getCopyCommand: using Windows clip.exe");
            return cached_command;
        }
        LOG("[CLIPBOARD] getCopyCommand: WSL detected but clip.exe not found");
    }

    // 检查 wl-clipboard
    LOG("[CLIPBOARD] getCopyCommand: checking for wl-copy...");
    if (system("which wl-copy > /dev/null 2>&1") == 0) {
        cached_command = "wl-copy";
        LOG("[CLIPBOARD] getCopyCommand: found wl-copy");
        return cached_command;
    }
    LOG("[CLIPBOARD] getCopyCommand: wl-copy not found, checking xclip...");
    // 检查 xclip
    if (system("which xclip > /dev/null 2>&1") == 0) {
        cached_command = "xclip -selection clipboard";
        LOG("[CLIPBOARD] getCopyCommand: found xclip");
        return cached_command;
    }
    LOG_ERROR("[CLIPBOARD] getCopyCommand: neither wl-copy nor xclip found!");
    cached_command = ""; // 标记为已检查但不可用
    return cached_command;
#endif
}

std::string Clipboard::getPasteCommand() {
    static std::string cached_command;
    static bool checked = false;

    if (checked) {
        return cached_command;
    }

    checked = true;

#ifdef __APPLE__
    // macOS 使用 pbpaste
    cached_command = "pbpaste";
    return cached_command;
#else
    // Linux: 优先使用 wl-clipboard (Wayland)，然后 xclip (X11)
    // 检查 wl-clipboard
    if (system("which wl-paste > /dev/null 2>&1") == 0) {
        cached_command = "wl-paste --no-newline";
        return cached_command;
    }
    // 检查 xclip
    if (system("which xclip > /dev/null 2>&1") == 0) {
        cached_command = "xclip -selection clipboard -o";
        return cached_command;
    }
    cached_command = ""; // 标记为已检查但不可用
    return cached_command;
#endif
}

bool Clipboard::executeCommand(const std::string& command, const std::string& input) {
    // 使用 popen 执行命令并写入输入
    LOG("[CLIPBOARD] executeCommand: opening pipe for: " + command);
    FILE* pipe = popen(command.c_str(), "w");
    if (!pipe) {
        LOG_ERROR("[CLIPBOARD] executeCommand: failed to open pipe");
        return false;
    }

    // 写入输入数据
    if (!input.empty()) {
        size_t written = fwrite(input.c_str(), 1, input.size(), pipe);
        LOG("[CLIPBOARD] executeCommand: wrote " + std::to_string(written) + " bytes");
        if (written != input.size()) {
            LOG_ERROR("[CLIPBOARD] executeCommand: failed to write all data (wrote " +
                      std::to_string(written) + " of " + std::to_string(input.size()) + ")");
        }
        fflush(pipe); // 确保数据被写入
    }

    int status = pclose(pipe);
    bool success = (status == 0);
    if (!success) {
        LOG_ERROR("[CLIPBOARD] executeCommand: command failed with status " +
                  std::to_string(status));
    } else {
        LOG("[CLIPBOARD] executeCommand: command succeeded");
    }
    return success;
}

std::string Clipboard::executeCommandWithOutput(const std::string& command) {
    std::array<char, 128> buffer;
    std::string result;

    // 使用 popen 执行命令并读取输出
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }

    // 读取输出
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }

    pclose(pipe);

    // 移除末尾的换行符（如果有）
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

} // namespace utils
} // namespace pnana
