#include "features/terminal/terminal_completion.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

namespace fs = std::filesystem;

namespace pnana {
namespace features {
namespace terminal {

bool TerminalCompletion::complete(const std::string& input, size_t cursor_pos,
                                  const std::string& current_directory, std::string& output,
                                  size_t& new_cursor_pos) {
    if (input.empty()) {
        return false;
    }

    // 找到光标位置之前的最后一个单词（命令或路径）
    size_t start_pos = cursor_pos;

    // 向前查找单词的开始位置
    while (start_pos > 0 && input[start_pos - 1] != ' ' && input[start_pos - 1] != '\t') {
        start_pos--;
    }

    // 提取要补全的前缀
    std::string prefix = input.substr(start_pos, cursor_pos - start_pos);

    if (prefix.empty()) {
        return false;
    }

    std::string completion;
    bool success = false;

    // 判断是命令补全还是路径补全
    if (start_pos == 0) {
        // 第一个单词 -> 可能是命令或路径
        // 先尝试路径补全（如果看起来像路径）
        if (isPath(prefix) || prefix.find('/') != std::string::npos) {
            success = completePath(prefix, current_directory, completion);
        }

        // 如果不是路径或路径补全失败，尝试命令补全
        if (!success) {
            success = completeCommand(prefix, completion);
        }
    } else {
        // 不是第一个单词，通常是参数（文件/目录路径）
        success = completePath(prefix, current_directory, completion);
    }

    if (success && !completion.empty()) {
        // 构建补全后的字符串
        output = input.substr(0, start_pos) + completion + input.substr(cursor_pos);
        new_cursor_pos = start_pos + completion.length();
        return true;
    }

    return false;
}

bool TerminalCompletion::completeCommand(const std::string& prefix, std::string& result) {
    std::vector<std::string> executables = getExecutablesFromPath();
    std::vector<std::string> matches = getMatches(prefix, executables);

    if (matches.empty()) {
        return false;
    }

    if (matches.size() == 1) {
        // 唯一匹配，直接补全
        result = matches[0];
        return true;
    }

    // 多个匹配，找到公共前缀
    std::string common = getCommonPrefix(matches);
    if (common.length() > prefix.length()) {
        // 有公共前缀，补全到公共前缀
        result = common;
        return true;
    }

    // 没有公共前缀，但至少有一个匹配，返回第一个匹配（用户需要继续输入）
    result = matches[0];
    return true;
}

bool TerminalCompletion::completePath(const std::string& prefix,
                                      const std::string& current_directory, std::string& result) {
    // 展开路径（处理 ~ 和相对路径）
    std::string expanded = expandPath(prefix, current_directory);

    // 分离目录和文件名
    fs::path path(expanded);
    std::string dir_str = path.parent_path().string();
    std::string filename_prefix = path.filename().string();

    if (dir_str.empty()) {
        dir_str = current_directory;
    }

    // 如果路径以 / 结尾，说明要补全目录
    bool is_directory = (prefix.back() == '/');

    if (is_directory && !expanded.empty() && expanded.back() != '/') {
        dir_str = expanded;
        filename_prefix = "";
    }

    // 确保目录存在
    if (!fs::exists(dir_str) || !fs::is_directory(dir_str)) {
        return false;
    }

    // 列出目录内容
    std::vector<std::string> items = listDirectory(dir_str);

    // 过滤匹配项
    std::vector<std::string> matches;
    for (const auto& item : items) {
        if (item.find(filename_prefix) == 0) {
            matches.push_back(item);
        }
    }

    if (matches.empty()) {
        return false;
    }

    if (matches.size() == 1) {
        // 唯一匹配
        std::string match = matches[0];
        fs::path full_path = fs::path(dir_str) / match;

        // 如果是目录，添加 /
        if (fs::is_directory(full_path)) {
            match += "/";
        }

        // 计算相对于原始前缀的补全部分
        if (prefix.find('/') != std::string::npos) {
            // 有路径分隔符，保留原始路径部分
            size_t last_slash = prefix.find_last_of('/');
            result = prefix.substr(0, last_slash + 1) + match;
        } else {
            result = match;
        }

        return true;
    }

    // 多个匹配，找到公共前缀
    std::string common = getCommonPrefix(matches);
    if (common.length() > filename_prefix.length()) {
        // 计算相对于原始前缀的补全部分
        if (prefix.find('/') != std::string::npos) {
            size_t last_slash = prefix.find_last_of('/');
            result = prefix.substr(0, last_slash + 1) + common;
        } else {
            result = common;
        }
        return true;
    }

    // 没有公共前缀，但至少有一个匹配，返回第一个匹配
    std::string match = matches[0];
    fs::path full_path = fs::path(dir_str) / match;

    // 如果是目录，添加 /
    if (fs::is_directory(full_path)) {
        match += "/";
    }

    // 计算相对于原始前缀的补全部分
    if (prefix.find('/') != std::string::npos) {
        size_t last_slash = prefix.find_last_of('/');
        result = prefix.substr(0, last_slash + 1) + match;
    } else {
        result = match;
    }

    return true;
}

std::vector<std::string> TerminalCompletion::getMatches(
    const std::string& prefix, const std::vector<std::string>& candidates) {
    std::vector<std::string> matches;
    for (const auto& candidate : candidates) {
        if (candidate.find(prefix) == 0) {
            matches.push_back(candidate);
        }
    }
    return matches;
}

std::string TerminalCompletion::getCommonPrefix(const std::vector<std::string>& matches) {
    if (matches.empty()) {
        return "";
    }

    if (matches.size() == 1) {
        return matches[0];
    }

    std::string common = matches[0];
    for (size_t i = 1; i < matches.size(); ++i) {
        size_t j = 0;
        while (j < common.length() && j < matches[i].length() && common[j] == matches[i][j]) {
            j++;
        }
        common = common.substr(0, j);
        if (common.empty()) {
            break;
        }
    }

    return common;
}

bool TerminalCompletion::isPath(const std::string& token) {
    return token[0] == '/' || token[0] == '~' || token.find("./") == 0 || token.find("../") == 0;
}

std::string TerminalCompletion::expandPath(const std::string& path,
                                           const std::string& current_directory) {
    if (path.empty()) {
        return current_directory;
    }

    std::string expanded = path;

    // 展开 ~
    if (expanded[0] == '~') {
        const char* home = getenv("HOME");
        if (!home) {
            struct passwd* pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            }
        }
        if (home) {
            if (expanded.length() == 1) {
                expanded = home;
            } else if (expanded[1] == '/') {
                expanded = std::string(home) + expanded.substr(1);
            } else {
                // ~username 格式，暂时不支持
            }
        }
    }

    // 处理相对路径
    if (expanded[0] != '/') {
        if (expanded.find("./") == 0) {
            expanded = current_directory + "/" + expanded.substr(2);
        } else if (expanded.find("../") == 0) {
            fs::path current(current_directory);
            expanded = (current.parent_path() / expanded.substr(3)).string();
        } else {
            // 相对路径，基于当前目录
            expanded = current_directory + "/" + expanded;
        }
    }

    return expanded;
}

std::vector<std::string> TerminalCompletion::getExecutablesFromPath() {
    std::vector<std::string> executables;

    const char* path_env = getenv("PATH");
    if (!path_env) {
        return executables;
    }

    std::istringstream path_stream(path_env);
    std::string path_dir;

    while (std::getline(path_stream, path_dir, ':')) {
        if (!fs::exists(path_dir) || !fs::is_directory(path_dir)) {
            continue;
        }

        try {
            for (const auto& entry : fs::directory_iterator(path_dir)) {
                if (entry.is_regular_file()) {
                    // 检查文件是否可执行（简化检查，实际应该检查权限）
                    // 在 Linux 上，我们假设所有在 PATH 中的文件都是可执行的
                    std::string filename = entry.path().filename().string();
                    // 避免重复
                    if (std::find(executables.begin(), executables.end(), filename) ==
                        executables.end()) {
                        executables.push_back(filename);
                    }
                }
            }
        } catch (const std::exception&) {
            // 忽略无法访问的目录
            continue;
        }
    }

    std::sort(executables.begin(), executables.end());
    return executables;
}

std::vector<std::string> TerminalCompletion::listDirectory(const std::string& dir_path) {
    std::vector<std::string> items;

    try {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            return items;
        }

        for (const auto& entry : fs::directory_iterator(dir_path)) {
            std::string name = entry.path().filename().string();
            items.push_back(name);
        }

        std::sort(items.begin(), items.end());
    } catch (const std::exception&) {
        // 忽略错误
    }

    return items;
}

} // namespace terminal
} // namespace features
} // namespace pnana
