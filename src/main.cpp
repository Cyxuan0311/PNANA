#include "core/editor.h"
#include "utils/logger.h"
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

void printHelp() {
    std::cout << "pnana - Modern Terminal Text Editor\n\n";
    std::cout << "Usage: pnana [OPTIONS] [FILE...]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -v, --version           Show version information\n";
    std::cout << "  -t, --theme THEME       Set theme (monokai, dracula, nord, etc.)\n";
    std::cout << "  -c, --config PATH       Specify custom configuration file path\n";
    std::cout << "  -r, --readonly          Open file in read-only mode\n";
    std::cout << "  -l, --log [FILE]        Enable logging (default: pnana.log)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  pnana                        Start with empty file\n";
    std::cout << "  pnana file.txt               Open file.txt\n";
    std::cout << "  pnana file1 file2            Open multiple files\n";
    std::cout << "  pnana -t dracula file.txt    Open with Dracula theme\n";
    std::cout << "  pnana -c ~/.config/pnana/custom.json  Use custom config file\n";
    std::cout << "  pnana -l ./pnana.log         Log to ./pnana.log\n";
    std::cout << "\nKeyboard Shortcuts:\n";
    std::cout << "  Ctrl+S    Save file\n";
    std::cout << "  Ctrl+Q    Quit\n";
    std::cout << "  Ctrl+F    Find\n";
    std::cout << "  Ctrl+H    Replace\n";
    std::cout << "  Ctrl+G    Go to line\n";
    std::cout << "  Ctrl+Z    Undo\n";
    std::cout << "  Ctrl+Y    Redo\n";
    std::cout << "\nFor more information, visit:\n";
    std::cout << "https://github.com/Cyxuan0311/PNANA.git\n";
}

void printVersion() {
    // ANSI颜色代码
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string WHITE = "\033[37m";

    std::cout << CYAN << BOLD << "  ██████╗ ███╗   ██╗ █████╗ ███╗   ██╗ █████╗ " << RESET
              << std::endl;
    std::cout << CYAN << BOLD << "  ██╔══██╗████╗  ██║██╔══██╗████╗  ██║██╔══██╗" << RESET
              << std::endl;
    std::cout << CYAN << BOLD << "  ██████╔╝██╔██╗ ██║███████║██╔██╗ ██║███████║" << RESET
              << std::endl;
    std::cout << CYAN << BOLD << "  ██╔═══╝ ██║╚██╗██║██╔══██║██║╚██╗██║██╔══██║" << RESET
              << std::endl;
    std::cout << CYAN << BOLD << "  ██║     ██║ ╚████║██║  ██║██║ ╚████║██║  ██║" << RESET
              << std::endl;
    std::cout << CYAN << BOLD << "  ╚═╝     ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝" << RESET
              << std::endl;

    std::cout << std::endl;
    std::cout << GREEN << BOLD << "  Modern Terminal Text Editor" << RESET << std::endl;
    std::cout << RED << BOLD << "  Version: " << RESET << " 0.0.5 " << std::endl;
    std::cout << YELLOW << "  Built with FTXUI and C++17" << RESET << std::endl;
    std::cout << MAGENTA << "  Latest development build" << RESET << std::endl;

    std::cout << std::endl;
    std::cout << BLUE << "  Features: LSP Support, Syntax Highlighting, Plugin System" << RESET
              << std::endl;
    std::cout << BLUE << "  Website: https://github.com/Cyxuan0311/PNANA.git" << RESET << std::endl;
}

// 屏蔽编辑器运行期间不应生效的 Unix 信号。
// 真正的 Ctrl+C / Ctrl+Z 拦截由 FTXUI 的
// ForceHandleCtrlC(false) / ForceHandleCtrlZ(false) 完成（见 Editor::run()）；
// 此处的 sigprocmask 作为兜底，防止其他路径下信号被意外投递。
void setupSignalHandlers() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    sigaction(SIGTSTP, &sa, nullptr); // Ctrl+Z：暂停
    sigaction(SIGINT, &sa, nullptr);  // Ctrl+C：中断
    sigaction(SIGQUIT, &sa, nullptr); // Ctrl+\：退出+coredump
    sigaction(SIGTTIN, &sa, nullptr); // 后台终端输入控制
    sigaction(SIGTTOU, &sa, nullptr); // 后台终端输出控制

    sigset_t block_mask;
    sigemptyset(&block_mask);
    sigaddset(&block_mask, SIGTSTP);
    sigaddset(&block_mask, SIGINT);
    sigaddset(&block_mask, SIGQUIT);
    sigaddset(&block_mask, SIGTTIN);
    sigaddset(&block_mask, SIGTTOU);
    sigprocmask(SIG_BLOCK, &block_mask, nullptr);
}

int main(int argc, char* argv[]) {
    try {
        setupSignalHandlers();

        std::vector<std::string> files;
        std::string theme = "monokai";
        std::string config_path = "";
        std::string log_file = "pnana.log";
        bool enable_logging = false;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                printHelp();
                return 0;
            } else if (arg == "-v" || arg == "--version") {
                printVersion();
                return 0;
            } else if (arg == "-t" || arg == "--theme") {
                if (i + 1 < argc) {
                    theme = argv[++i];
                } else {
                    std::cerr << "Error: --theme requires an argument\n";
                    return 1;
                }
            } else if (arg == "-c" || arg == "--config") {
                if (i + 1 < argc) {
                    config_path = argv[++i];
                } else {
                    std::cerr << "Error: --config requires an argument\n";
                    return 1;
                }
            } else if (arg == "-r" || arg == "--readonly") {
                // TODO: 实现只读模式
                std::cerr << "Warning: readonly mode not yet implemented\n";
            } else if (arg == "-l" || arg == "--log") {
                enable_logging = true;
                if (i + 1 < argc && argv[i + 1][0] != '-') {
                    log_file = argv[++i];
                }
            } else if (arg[0] == '-') {
                std::cerr << "Error: Unknown option: " << arg << "\n";
                std::cerr << "Try 'pnana --help' for more information.\n";
                return 1;
            } else {
                files.push_back(arg);
            }
        }

        if (enable_logging) {
            pnana::utils::Logger::getInstance().initialize(log_file);
            pnana::utils::Logger::getInstance().log("Logger initialized: " + log_file);
        }

        pnana::core::Editor editor;

        if (!config_path.empty()) {
            editor.loadConfig(config_path);
        }

        if (theme != "monokai") {
            editor.setTheme(theme);
        }

        if (!files.empty()) {
            editor.openFile(files[0]);
        }

        editor.run();

        if (enable_logging) {
            pnana::utils::Logger::getInstance().close();
        }

        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Fatal error: Unknown exception\n";
        return 1;
    }
}
