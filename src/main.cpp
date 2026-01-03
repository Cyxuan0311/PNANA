#include "core/editor.h"
#include "utils/logger.h"
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>
#include <csignal>
#include <cstring>

void printHelp() {
    std::cout << "pnana - Modern Terminal Text Editor\n\n";
    std::cout << "Usage: pnana [OPTIONS] [FILE...]\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -v, --version           Show version information\n";
    std::cout << "  -t, --theme THEME       Set theme (monokai, dracula, nord, etc.)\n";
    std::cout << "  -c, --config PATH       Specify custom configuration file path\n";
    std::cout << "  -r, --readonly          Open file in read-only mode\n";
    std::cout << "  -l, --log               Enable logging to pnana.log file\n";
    std::cout << "\nExamples:\n";
    std::cout << "  pnana                        Start with empty file\n";
    std::cout << "  pnana file.txt               Open file.txt\n";
    std::cout << "  pnana file1 file2            Open multiple files\n";
    std::cout << "  pnana -t dracula file.txt    Open with Dracula theme\n";
    std::cout << "  pnana -c ~/.config/pnana/custom.json  Use custom config file\n";
    std::cout << "  pnana -l file.txt            Open file with logging enabled\n";
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
    
    std::cout << CYAN << BOLD << "  ██████╗ ███╗   ██╗ █████╗ ███╗   ██╗ █████╗ " << RESET << std::endl;
    std::cout << CYAN << BOLD << "  ██╔══██╗████╗  ██║██╔══██╗████╗  ██║██╔══██╗" << RESET << std::endl;
    std::cout << CYAN << BOLD << "  ██████╔╝██╔██╗ ██║███████║██╔██╗ ██║███████║" << RESET << std::endl;
    std::cout << CYAN << BOLD << "  ██╔═══╝ ██║╚██╗██║██╔══██║██║╚██╗██║██╔══██║" << RESET << std::endl;
    std::cout << CYAN << BOLD << "  ██║     ██║ ╚████║██║  ██║██║ ╚████║██║  ██║" << RESET << std::endl;
    std::cout << CYAN << BOLD << "  ╚═╝     ╚═╝  ╚═══╝╚═╝  ╚═╝╚═╝  ╚═══╝╚═╝  ╚═╝" << RESET << std::endl;
    
    std::cout << std::endl;
    std::cout << GREEN << BOLD << "  Modern Terminal Text Editor" << RESET << std::endl;
    std::cout << YELLOW << "  Built with FTXUI and C++17" << RESET << std::endl;
    std::cout << MAGENTA << "  Latest development build" << RESET << std::endl;
    
    std::cout << std::endl;
    std::cout << BLUE << "  Features: LSP Support, Syntax Highlighting, Plugin System" << RESET << std::endl;
    std::cout << BLUE << "  Website: https://github.com/Cyxuan0311/PNANA.git" << RESET << std::endl;
}

// 空的信号处理器，用于屏蔽系统信号
static void ignoreSignal(int sig) {
    (void)sig;  // 避免未使用参数警告
    // 不执行任何操作，屏蔽信号
}

// 设置信号处理，屏蔽 Ctrl+Z (SIGTSTP) 和 Ctrl+C (SIGINT)
void setupSignalHandlers() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = ignoreSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    // 屏蔽 SIGTSTP (Ctrl+Z) - 防止程序被暂停
    sigaction(SIGTSTP, &sa, nullptr);
    
    // 屏蔽 SIGINT (Ctrl+C) - 防止程序被终止（Ctrl+C 在编辑器中用于复制）
    sigaction(SIGINT, &sa, nullptr);
    
    // 可选：也屏蔽其他可能干扰的信号
    sigaction(SIGTTIN, &sa, nullptr);  // 终端输入控制
    sigaction(SIGTTOU, &sa, nullptr);  // 终端输出控制
}


int main(int argc, char* argv[]) {
    try {
        // 首先设置信号处理，屏蔽 Ctrl+Z 和 Ctrl+C 的系统默认行为
        setupSignalHandlers();
        
        std::vector<std::string> files;
        std::string theme = "monokai";
        std::string config_path = "";
        bool enable_logging = false;
        
        // 解析命令行参数
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
            } else if (arg[0] == '-') {
                std::cerr << "Error: Unknown option: " << arg << "\n";
                std::cerr << "Try 'pnana --help' for more information.\n";
                return 1;
            } else {
                files.push_back(arg);
            }
        }
        
        // 只有在指定 --log 选项时才初始化日志系统
        if (enable_logging) {
            pnana::utils::Logger::getInstance().initialize("pnana.log");
        }
        
        // 创建编辑器（会自动加载默认配置）
        pnana::core::Editor editor;
        
        // 如果指定了自定义配置文件路径，加载它（如果不存在会自动创建）
        if (!config_path.empty()) {
            editor.loadConfig(config_path);
        }
        
        // 设置主题（如果通过命令行指定，会覆盖配置文件中的主题）
        if (theme != "monokai") {
            editor.setTheme(theme);
        }
        
        // 打开文件
        if (!files.empty()) {
            editor.openFile(files[0]);
        }
        
        // 运行编辑器
        editor.run();
        
        // 关闭日志（如果已启用）
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