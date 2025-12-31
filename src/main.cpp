#include "core/editor.h"
#include "utils/logger.h"
#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <cstdlib>
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
    std::cout << "pnana version 0.0.3\n";
    std::cout << "Modern Terminal Text Editor\n";
    std::cout << "Built with FTXUI and C++17\n";
}

// 信号处理：忽略 SIGTSTP (Ctrl+Z)，让应用程序处理它
void setupSignalHandlers() {
    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    // 忽略 SIGTSTP (Ctrl+Z)，这样 Ctrl+Z 可以被应用程序捕获用于撤销
    if (sigaction(SIGTSTP, &sa, nullptr) != 0) {
        // 如果 sigaction 失败，回退到 signal()
        signal(SIGTSTP, SIG_IGN);
    }
    
    // 可选：也可以忽略 SIGTTIN 和 SIGTTOU（终端输入/输出控制信号）
    sigaction(SIGTTIN, &sa, nullptr);
    sigaction(SIGTTOU, &sa, nullptr);
}

int main(int argc, char* argv[]) {
    try {
        // 设置信号处理，忽略 SIGTSTP (Ctrl+Z)
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
        // 如果没有指定配置文件，Editor 构造函数已经加载了默认配置
        // 此时配置文件应该已经存在于 ~/.config/pnana/config.json
        
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
