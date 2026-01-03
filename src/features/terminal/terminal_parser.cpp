#include "features/terminal/terminal_parser.h"
#include <cctype>

namespace pnana {
namespace features {
namespace terminal {

std::vector<std::string> CommandParser::parse(const std::string& command) {
    std::vector<std::string> args;

    bool in_quotes = false;
    char quote_char = '\0';
    std::string current_arg;

    for (size_t i = 0; i < command.length(); ++i) {
        char c = command[i];

        if (in_quotes) {
            if (c == quote_char) {
                in_quotes = false;
                quote_char = '\0';
                if (!current_arg.empty()) {
                    args.push_back(current_arg);
                    current_arg.clear();
                }
            } else {
                current_arg += c;
            }
        } else {
            if (c == '"' || c == '\'') {
                if (!current_arg.empty()) {
                    args.push_back(current_arg);
                    current_arg.clear();
                }
                in_quotes = true;
                quote_char = c;
            } else if (std::isspace(c)) {
                if (!current_arg.empty()) {
                    args.push_back(current_arg);
                    current_arg.clear();
                }
            } else {
                current_arg += c;
            }
        }
    }

    if (!current_arg.empty()) {
        args.push_back(current_arg);
    }

    return args;
}

bool CommandParser::hasShellFeatures(const std::string& command) {
    return (command.find('|') != std::string::npos || command.find('>') != std::string::npos ||
            command.find('<') != std::string::npos || command.find('&') != std::string::npos ||
            command.find('$') != std::string::npos || command.find('*') != std::string::npos ||
            command.find('?') != std::string::npos || command.find('(') != std::string::npos ||
            command.find('`') != std::string::npos || command.find("&&") != std::string::npos ||
            command.find("||") != std::string::npos);
}

bool CommandParser::isBackgroundCommand(const std::string& command,
                                        std::string& cmd_without_ampersand) {
    cmd_without_ampersand = command;
    if (!cmd_without_ampersand.empty() && cmd_without_ampersand.back() == '&') {
        cmd_without_ampersand.pop_back();
        // 移除末尾的空格
        while (!cmd_without_ampersand.empty() && std::isspace(cmd_without_ampersand.back())) {
            cmd_without_ampersand.pop_back();
        }
        return true;
    }
    return false;
}

} // namespace terminal
} // namespace features
} // namespace pnana
