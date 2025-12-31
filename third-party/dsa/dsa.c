/**
 * dsa - 终端图片查看器
 * 在终端中显示JPEG/PNG图片的ASCII艺术版本
 * 
 * 使用方法: ./dsa image.jpg [width]
 * 
 * 作者: Linux Command Pro Team
 * 版本: 1.0.0
 */

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

// 字符集模式枚举
typedef enum {
    CHARSET_UNICODE_BLOCKS,      // Unicode块状字符（默认，原有模式）
    CHARSET_UNICODE_BLOCKS_FULL, // Unicode完整块状字符集
    CHARSET_ASCII_SIMPLE,        // ASCII简单字符
    CHARSET_ASCII_DETAILED,      // ASCII详细字符
    CHARSET_ASCII_NUMBERS,       // ASCII数字
    CHARSET_ASCII_LETTERS,       // ASCII字母
    CHARSET_ASCII_MIXED          // ASCII混合字符
} charset_mode_t;

// Unicode方块字符集，按亮度从暗到亮排列，提供更好的视觉效果（原有模式）
static const char UNICODE_CHARS[] = "█▓▒░";

// Unicode完整块状字符集（更多层次）
static const char UNICODE_CHARS_FULL[] = "█▓▒░▄▀";

// ASCII简单字符集（从暗到亮）
static const char ASCII_SIMPLE[] = " .:-=+*#%@$";

// ASCII详细字符集（从暗到亮）
static const char ASCII_DETAILED[] = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";

// ASCII数字字符集（从暗到亮）
static const char ASCII_NUMBERS[] = "0123456789";

// ASCII字母字符集（从暗到亮）
static const char ASCII_LETTERS[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

// ASCII混合字符集（从暗到亮）
static const char ASCII_MIXED[] = " .:;+=xX$&";

// 颜色代码
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// 默认宽度
#define DEFAULT_WIDTH 120

// 默认启用颜色
#define DEFAULT_COLOR 1

// 默认分辨率倍数
#define DEFAULT_RESOLUTION_SCALE 1.0f

// 颜色模式
#define COLOR_MODE_8BIT  0  // 8/16色模式
#define COLOR_MODE_24BIT 1  // 24位真彩色模式

// 检测终端是否支持24位真彩色
int detect_truecolor_support() {
    const char *term = getenv("TERM");
    const char *colorterm = getenv("COLORTERM");
    
    // 检查 COLORTERM 环境变量
    if (colorterm) {
        if (strstr(colorterm, "truecolor") || strstr(colorterm, "24bit")) {
            return 1;
        }
    }
    
    // 检查常见的支持真彩色的终端
    if (term) {
        const char *truecolor_terms[] = {
            "xterm-256color", "screen-256color", "tmux-256color",
            "rxvt-unicode-256color", "alacritty", "kitty", "wezterm",
            "vscode", "gnome-terminal", "konsole", "terminator"
        };
        
        size_t num_terms = sizeof(truecolor_terms) / sizeof(truecolor_terms[0]);
        for (size_t i = 0; i < num_terms; i++) {
            if (strstr(term, truecolor_terms[i])) {
                return 1;
            }
        }
    }
    
    // 尝试通过查询终端能力来检测（更可靠的方法）
    // 发送查询序列并检查响应
    if (isatty(STDOUT_FILENO)) {
        // 大多数现代终端都支持，默认返回1
        // 如果终端不支持，会显示错误的颜色，但不会崩溃
        return 1;
    }
    
    return 0;
}

// 帮助信息
void print_help(const char *program_name) {
    printf("🐧 dsa - 终端图片查看器\n");
    printf("========================\n\n");
    printf("使用方法: %s <图片文件> [宽度]\n\n", program_name);
    printf("参数:\n");
    printf("  图片文件    要显示的图片文件路径 (支持JPG, PNG格式)\n");
    printf("  宽度        可选，ASCII图片的宽度 (默认: %d)\n\n", DEFAULT_WIDTH);
    printf("选项:\n");
    printf("  -h, --help        显示此帮助信息\n");
    printf("  -v, --version     显示版本信息\n");
    printf("  -c, --color       启用颜色显示 (默认)\n");
    printf("  -n, --no-color    禁用颜色显示\n");
    printf("  -w, --width       指定宽度\n");
    printf("  -m, --mode        指定字符集模式 (默认: unicode)\n");
    printf("  -r, --resolution  分辨率倍数 (默认: 1.0, 建议: 1.5-3.0)\n\n");
    printf("字符集模式:\n");
    printf("  unicode         Unicode块状字符 (默认，原有模式) █▓▒░\n");
    printf("  unicode-full    Unicode完整块状字符集 █▓▒░▄▀\n");
    printf("  ascii-simple    ASCII简单字符  .:-=+*#%%@$\n");
    printf("  ascii-detailed  ASCII详细字符 (更多层次)\n");
    printf("  ascii-numbers   ASCII数字 0123456789\n");
    printf("  ascii-letters   ASCII字母 a-z A-Z\n");
    printf("  ascii-mixed     ASCII混合字符  .:;+=xX$&\n\n");
    printf("示例:\n");
    printf("  %s image.jpg\n", program_name);
    printf("  %s image.png 120\n", program_name);
    printf("  %s -c image.jpg\n", program_name);
    printf("  %s -n image.jpg\n", program_name);
    printf("  %s --width 100 image.png\n", program_name);
    printf("  %s --mode ascii-simple image.jpg\n", program_name);
    printf("  %s --mode ascii-numbers image.png\n", program_name);
    printf("  %s --mode unicode-full image.jpg\n", program_name);
    printf("  %s --resolution 2.0 image.jpg\n", program_name);
    printf("  %s -r 1.5 --width 150 image.png\n", program_name);
}

// 版本信息
void print_version() {
    printf("dsa version 1.0.0\n");
    printf("Copyright (c) 2025 Linux Command Pro Team\n");
    printf("MIT License\n");
}

// 将RGB值转换为灰度值
unsigned char rgb_to_gray(unsigned char r, unsigned char g, unsigned char b) {
    return (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
}

// 获取字符（根据字符集模式和灰度值）
char* get_char_for_gray(unsigned char gray_value, charset_mode_t charset_mode) {
    static char result[8]; // 支持多字节Unicode字符
    const char *charset = NULL;
    int charset_len = 0;
    
    // 根据字符集模式选择字符集
    switch(charset_mode) {
        case CHARSET_UNICODE_BLOCKS:
            charset = UNICODE_CHARS;
            charset_len = 4; // "█▓▒░"
            break;
        case CHARSET_UNICODE_BLOCKS_FULL:
            charset = UNICODE_CHARS_FULL;
            charset_len = 6; // "█▓▒░▄▀"
            break;
        case CHARSET_ASCII_SIMPLE:
            charset = ASCII_SIMPLE;
            charset_len = strlen(ASCII_SIMPLE);
            break;
        case CHARSET_ASCII_DETAILED:
            charset = ASCII_DETAILED;
            charset_len = strlen(ASCII_DETAILED);
            break;
        case CHARSET_ASCII_NUMBERS:
            charset = ASCII_NUMBERS;
            charset_len = strlen(ASCII_NUMBERS);
            break;
        case CHARSET_ASCII_LETTERS:
            charset = ASCII_LETTERS;
            charset_len = strlen(ASCII_LETTERS);
            break;
        case CHARSET_ASCII_MIXED:
            charset = ASCII_MIXED;
            charset_len = strlen(ASCII_MIXED);
            break;
        default:
            charset = UNICODE_CHARS;
            charset_len = 4;
            break;
    }
    
    if (charset_len <= 0) {
        strcpy(result, " ");
        return result;
    }
    
    // 计算字符索引（灰度值越大，字符越暗）
    // 对于ASCII字符集，需要反转映射（ASCII字符集中，前面的字符较暗）
    int index;
    if (charset_mode == CHARSET_UNICODE_BLOCKS || charset_mode == CHARSET_UNICODE_BLOCKS_FULL) {
        // Unicode块状字符：灰度值越大，使用越暗的字符（索引越大）
        index = (gray_value * (charset_len - 1)) / 255;
    } else {
        // ASCII字符：灰度值越大，使用越亮的字符（索引越大）
        // 但ASCII字符集中前面的字符较暗，所以需要反转
        index = ((255 - gray_value) * (charset_len - 1)) / 255;
    }
    
    // 确保索引在有效范围内
    if (index < 0) index = 0;
    if (index >= charset_len) index = charset_len - 1;
    
    // 复制字符到结果
    if (charset_mode == CHARSET_UNICODE_BLOCKS || charset_mode == CHARSET_UNICODE_BLOCKS_FULL) {
        // Unicode字符可能是多字节的，使用预定义的字符数组
        const char* unicode_chars_array[] = {
            "░", "▒", "▓", "█"  // CHARSET_UNICODE_BLOCKS
        };
        const char* unicode_chars_full_array[] = {
            "░", "▒", "▓", "█", "▄", "▀"  // CHARSET_UNICODE_BLOCKS_FULL
        };
        
        const char* selected_char;
        if (charset_mode == CHARSET_UNICODE_BLOCKS) {
            selected_char = unicode_chars_array[index];
        } else {
            selected_char = unicode_chars_full_array[index];
        }
        
        // 复制Unicode字符（最多4字节）
        int i = 0;
        while (selected_char[i] != '\0' && i < 7) {
            result[i] = selected_char[i];
            i++;
        }
        result[i] = '\0';
    } else {
        // ASCII字符，单字节
        result[0] = charset[index];
        result[1] = '\0';
    }
    
    return result;
}

// 获取Unicode字符（保留原有函数以兼容）
char* get_unicode_char(unsigned char gray_value) {
    return get_char_for_gray(gray_value, CHARSET_UNICODE_BLOCKS);
}

// 获取24位真彩色代码
void get_truecolor_code(unsigned char r, unsigned char g, unsigned char b, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size, "\033[38;2;%d;%d;%dm", r, g, b);
}

// 将RGB转换为256色模式（8位颜色）
int rgb_to_256color(unsigned char r, unsigned char g, unsigned char b) {
    // 使用标准256色映射算法
    // 前16色是系统颜色，跳过
    // 216色是6x6x6的RGB立方体 (16-231)
    // 最后24色是灰度 (232-255)
    
    // 如果颜色接近灰度，使用灰度色阶
    int max_val = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    int min_val = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    
    if (max_val - min_val < 32) {
        // 使用灰度色阶 (232-255)
        int gray = (r + g + b) / 3;
        return 232 + (gray * 23) / 255;
    }
    
    // 使用RGB立方体 (16-231)
    // 每个分量映射到0-5
    int r6 = (r * 5) / 255;
    int g6 = (g * 5) / 255;
    int b6 = (b * 5) / 255;
    
    return 16 + r6 * 36 + g6 * 6 + b6;
}

// 获取颜色代码 - 支持24位真彩色和256色模式
void get_color_code(unsigned char r, unsigned char g, unsigned char b, 
                    int color_mode, char *buffer, size_t buffer_size) {
    if (color_mode == COLOR_MODE_24BIT) {
        // 使用24位真彩色
        get_truecolor_code(r, g, b, buffer, buffer_size);
    } else {
        // 使用256色模式
        int color_code = rgb_to_256color(r, g, b);
        snprintf(buffer, buffer_size, "\033[38;5;%dm", color_code);
    }
}

// 获取颜色代码（旧版本兼容，用于8/16色模式）
const char* get_color_code_8bit(unsigned char r, unsigned char g, unsigned char b) {
    // 计算亮度和饱和度
    int brightness = (r + g + b) / 3;
    int max_val = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    int min_val = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    int saturation = max_val - min_val;
    
    // 改进的颜色映射算法
    // 使用更精确的阈值和更丰富的颜色判断
    
    // 如果饱和度很低，使用灰度
    if (saturation < 25) {
        if (brightness > 220) return "\033[97m";      // 很亮白
        else if (brightness > 180) return "\033[37m"; // 亮白
        else if (brightness > 140) return "\033[90m"; // 中亮灰
        else if (brightness > 100) return "\033[90m"; // 中灰
        else if (brightness > 60) return "\033[90m";  // 暗灰
        else if (brightness > 30) return "\033[30m";  // 很暗灰
        else return "\033[30m";                       // 黑色
    }
    
    // 计算各颜色分量的相对强度
    int r_ratio = (r * 100) / (max_val + 1);
    int g_ratio = (g * 100) / (max_val + 1);
    int b_ratio = (b * 100) / (max_val + 1);
    
    // 根据主色调和亮度确定颜色
    if (r > g + 30 && r > b + 30) {
        // 红色系（红色明显占优）
        if (brightness > 200) return "\033[91m";      // 亮红
        else if (brightness > 140) return "\033[31m"; // 红
        else if (brightness > 80) return "\033[31m";  // 中红
        else return "\033[31m";                       // 暗红
    } else if (g > r + 30 && g > b + 30) {
        // 绿色系（绿色明显占优）
        if (brightness > 200) return "\033[92m";      // 亮绿
        else if (brightness > 140) return "\033[32m"; // 绿
        else if (brightness > 80) return "\033[32m";  // 中绿
        else return "\033[32m";                       // 暗绿
    } else if (b > r + 30 && b > g + 30) {
        // 蓝色系（蓝色明显占优）
        if (brightness > 200) return "\033[94m";      // 亮蓝
        else if (brightness > 140) return "\033[34m"; // 蓝
        else if (brightness > 80) return "\033[34m";  // 中蓝
        else return "\033[34m";                       // 暗蓝
    } else if (r > 180 && g > 180 && b < 120) {
        // 黄色系（红+绿，蓝少）
        if (brightness > 200) return "\033[93m";      // 亮黄
        else return "\033[33m";                       // 黄
    } else if (r > 180 && g < 120 && b > 180) {
        // 洋红色系（红+蓝，绿少）
        if (brightness > 200) return "\033[95m";      // 亮洋红
        else return "\033[35m";                       // 洋红
    } else if (r < 120 && g > 180 && b > 180) {
        // 青色系（绿+蓝，红少）
        if (brightness > 200) return "\033[96m";      // 亮青
        else return "\033[36m";                       // 青
    } else if (r > 140 && g > 140 && b > 140) {
        // 白色系（所有颜色都较高）
        if (brightness > 220) return "\033[97m";      // 很亮白
        else if (brightness > 180) return "\033[37m"; // 亮白
        else return "\033[37m";                       // 白
    }
    
    // 混合色：根据主要颜色分量选择
    if (r_ratio > g_ratio && r_ratio > b_ratio) {
        // 偏红
        if (brightness > 150) return "\033[91m";
        else return "\033[31m";
    } else if (g_ratio > r_ratio && g_ratio > b_ratio) {
        // 偏绿
        if (brightness > 150) return "\033[92m";
        else return "\033[32m";
    } else if (b_ratio > r_ratio && b_ratio > g_ratio) {
        // 偏蓝
        if (brightness > 150) return "\033[94m";
        else return "\033[34m";
    }
    
    // 默认返回基于亮度的颜色
    if (brightness > 200) return "\033[97m";
    else if (brightness > 150) return "\033[37m";
    else if (brightness > 100) return "\033[90m";
    else if (brightness > 50) return "\033[90m";
    else return "\033[30m";
}

// 解析字符集模式
charset_mode_t parse_charset_mode(const char *mode_str) {
    if (strcmp(mode_str, "unicode") == 0) {
        return CHARSET_UNICODE_BLOCKS;
    } else if (strcmp(mode_str, "unicode-full") == 0) {
        return CHARSET_UNICODE_BLOCKS_FULL;
    } else if (strcmp(mode_str, "ascii-simple") == 0) {
        return CHARSET_ASCII_SIMPLE;
    } else if (strcmp(mode_str, "ascii-detailed") == 0) {
        return CHARSET_ASCII_DETAILED;
    } else if (strcmp(mode_str, "ascii-numbers") == 0) {
        return CHARSET_ASCII_NUMBERS;
    } else if (strcmp(mode_str, "ascii-letters") == 0) {
        return CHARSET_ASCII_LETTERS;
    } else if (strcmp(mode_str, "ascii-mixed") == 0) {
        return CHARSET_ASCII_MIXED;
    } else {
        return CHARSET_UNICODE_BLOCKS; // 默认模式
    }
}

// 获取字符集模式名称
const char* get_charset_mode_name(charset_mode_t mode) {
    switch(mode) {
        case CHARSET_UNICODE_BLOCKS: return "Unicode块状字符";
        case CHARSET_UNICODE_BLOCKS_FULL: return "Unicode完整块状字符";
        case CHARSET_ASCII_SIMPLE: return "ASCII简单字符";
        case CHARSET_ASCII_DETAILED: return "ASCII详细字符";
        case CHARSET_ASCII_NUMBERS: return "ASCII数字";
        case CHARSET_ASCII_LETTERS: return "ASCII字母";
        case CHARSET_ASCII_MIXED: return "ASCII混合字符";
        default: return "Unicode块状字符";
    }
}

// 显示图片
int display_image(const char *filename, int width, int use_color, charset_mode_t charset_mode, float resolution_scale) {
    int x, y, n;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
    
    if (!data) {
        fprintf(stderr, "❌ 错误: 无法加载图片 '%s'\n", filename);
        fprintf(stderr, "   请检查文件是否存在且格式正确 (支持JPG, PNG)\n");
        return 1;
    }
    
    // 检测颜色模式
    int color_mode = COLOR_MODE_8BIT;
    const char *color_mode_str = "8/16色";
    if (use_color) {
        if (detect_truecolor_support()) {
            color_mode = COLOR_MODE_24BIT;
            color_mode_str = "24位真彩色";
        } else {
            color_mode = COLOR_MODE_8BIT;
            color_mode_str = "256色";
        }
    }
    
    printf("🖼️  图片信息: %dx%d, %d通道\n", x, y, n);
    printf("📏 显示宽度: %d 字符\n", width);
    printf("🎨 颜色模式: %s", use_color ? color_mode_str : "禁用");
    if (use_color && color_mode == COLOR_MODE_24BIT) {
        printf(" ✨");
    }
    printf("\n");
    printf("🔤 字符集模式: %s\n", get_charset_mode_name(charset_mode));
    if (resolution_scale != 1.0f) {
        printf("🔍 分辨率倍数: %.1fx\n", resolution_scale);
    }
    printf("\n");
    
    // 应用分辨率倍数到宽度
    int effective_width = (int)(width * resolution_scale);
    
    // 计算缩放比例 - 提高分辨率
    float scale = (float)effective_width / x;
    int new_height = (int)(y * scale * 0.6); // 字符高度约为宽度的0.6倍
    
    if (new_height <= 0) new_height = 1;
    
    printf("📐 缩放后尺寸: %dx%d (有效宽度: %d)\n\n", effective_width, new_height, effective_width);
    
    // 生成ASCII艺术 - 使用改进的采样算法
    // 根据分辨率倍数调整采样区域大小
    int base_sample_size = 2;
    int sample_size = (int)(base_sample_size * resolution_scale);
    if (sample_size < 1) sample_size = 1;
    if (sample_size > 5) sample_size = 5; // 限制最大采样区域，避免性能问题
    
    for (int i = 0; i < new_height; i++) {
        for (int j = 0; j < effective_width; j++) {
            // 计算原始图片中的对应位置（使用双线性插值提高质量）
            float orig_x_f = (float)j / scale;
            float orig_y_f = (float)i / scale / 0.6;
            
            int orig_x = (int)orig_x_f;
            int orig_y = (int)orig_y_f;
            
            if (orig_x >= x) orig_x = x - 1;
            if (orig_y >= y) orig_y = y - 1;
            
            // 使用区域采样提高质量（根据分辨率倍数调整采样区域）
            int r_sum = 0, g_sum = 0, b_sum = 0, count = 0;
            
            // 如果分辨率倍数较高，使用双线性插值
            if (resolution_scale > 1.5f) {
                // 双线性插值
                int x1 = orig_x;
                int y1 = orig_y;
                int x2 = (x1 + 1 < x) ? x1 + 1 : x1;
                int y2 = (y1 + 1 < y) ? y1 + 1 : y1;
                
                // 确保坐标在有效范围内
                if (x1 < 0) x1 = 0;
                if (x1 >= x) x1 = x - 1;
                if (x2 < 0) x2 = 0;
                if (x2 >= x) x2 = x - 1;
                if (y1 < 0) y1 = 0;
                if (y1 >= y) y1 = y - 1;
                if (y2 < 0) y2 = 0;
                if (y2 >= y) y2 = y - 1;
                
                float fx = orig_x_f - (int)orig_x_f;
                float fy = orig_y_f - (int)orig_y_f;
                
                // 边界处理：如果接近边界，fx或fy可能为负或大于1
                if (fx < 0) fx = 0;
                if (fx > 1) fx = 1;
                if (fy < 0) fy = 0;
                if (fy > 1) fy = 1;
                
                // 获取四个角点的颜色
                int idx11 = (y1 * x + x1) * n;
                int idx12 = (y1 * x + x2) * n;
                int idx21 = (y2 * x + x1) * n;
                int idx22 = (y2 * x + x2) * n;
                
                // 确保索引有效
                if (idx11 >= 0 && idx11 < x * y * n &&
                    idx12 >= 0 && idx12 < x * y * n &&
                    idx21 >= 0 && idx21 < x * y * n &&
                    idx22 >= 0 && idx22 < x * y * n) {
                    
                    // 双线性插值计算
                    unsigned char r1 = (unsigned char)(data[idx11] * (1 - fx) + data[idx12] * fx);
                    unsigned char g1 = (unsigned char)(data[idx11 + 1] * (1 - fx) + data[idx12 + 1] * fx);
                    unsigned char b1 = (unsigned char)(data[idx11 + 2] * (1 - fx) + data[idx12 + 2] * fx);
                    
                    unsigned char r2 = (unsigned char)(data[idx21] * (1 - fx) + data[idx22] * fx);
                    unsigned char g2 = (unsigned char)(data[idx21 + 1] * (1 - fx) + data[idx22 + 1] * fx);
                    unsigned char b2 = (unsigned char)(data[idx21 + 2] * (1 - fx) + data[idx22 + 2] * fx);
                    
                    unsigned char r = (unsigned char)(r1 * (1 - fy) + r2 * fy);
                    unsigned char g = (unsigned char)(g1 * (1 - fy) + g2 * fy);
                    unsigned char b = (unsigned char)(b1 * (1 - fy) + b2 * fy);
                    
                    // 转换为灰度
                    unsigned char gray = rgb_to_gray(r, g, b);
                    
                    // 根据字符集模式获取字符
                    char* display_char = get_char_for_gray(gray, charset_mode);
                    
                    // 输出字符
                    if (use_color) {
                        char color_buffer[64];
                        get_color_code(r, g, b, color_mode, color_buffer, sizeof(color_buffer));
                        printf("%s%s%s", color_buffer, display_char, RESET);
                    } else {
                        printf("%s", display_char);
                    }
                } else {
                    // 如果索引无效，使用最近邻采样
                    int pixel_index = (orig_y * x + orig_x) * n;
                    if (pixel_index >= 0 && pixel_index < x * y * n) {
                        unsigned char r = data[pixel_index];
                        unsigned char g = data[pixel_index + 1];
                        unsigned char b = data[pixel_index + 2];
                        unsigned char gray = rgb_to_gray(r, g, b);
                        char* display_char = get_char_for_gray(gray, charset_mode);
                        
                        if (use_color) {
                            char color_buffer[64];
                            get_color_code(r, g, b, color_mode, color_buffer, sizeof(color_buffer));
                            printf("%s%s%s", color_buffer, display_char, RESET);
                        } else {
                            printf("%s", display_char);
                        }
                    } else {
                        printf(" ");
                    }
                }
            } else {
                // 使用区域采样（原有方法，适合低分辨率倍数）
                for (int dy = -sample_size/2; dy <= sample_size/2; dy++) {
                    for (int dx = -sample_size/2; dx <= sample_size/2; dx++) {
                        int sample_x = orig_x + dx;
                        int sample_y = orig_y + dy;
                        
                        if (sample_x >= 0 && sample_x < x && sample_y >= 0 && sample_y < y) {
                            int pixel_index = (sample_y * x + sample_x) * n;
                            r_sum += data[pixel_index];
                            g_sum += data[pixel_index + 1];
                            b_sum += data[pixel_index + 2];
                            count++;
                        }
                    }
                }
                
                if (count > 0) {
                    unsigned char r = r_sum / count;
                    unsigned char g = g_sum / count;
                    unsigned char b = b_sum / count;
                    
                    // 转换为灰度
                    unsigned char gray = rgb_to_gray(r, g, b);
                    
                    // 根据字符集模式获取字符
                    char* display_char = get_char_for_gray(gray, charset_mode);
                    
                    // 输出字符
                    if (use_color) {
                        char color_buffer[64];
                        get_color_code(r, g, b, color_mode, color_buffer, sizeof(color_buffer));
                        printf("%s%s%s", color_buffer, display_char, RESET);
                    } else {
                        printf("%s", display_char);
                    }
                } else {
                    printf(" ");
                }
            }
            
        }
        printf("\n");
    }
    
    printf("\n✨ 图片显示完成!\n");
    
    // 释放内存
    stbi_image_free(data);
    return 0;
}

int main(int argc, char *argv[]) {
    int width = DEFAULT_WIDTH;
    int use_color = DEFAULT_COLOR; // 默认启用颜色
    charset_mode_t charset_mode = CHARSET_UNICODE_BLOCKS; // 默认使用Unicode块状字符（原有模式）
    float resolution_scale = DEFAULT_RESOLUTION_SCALE; // 默认分辨率倍数
    char *filename = NULL;
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) {
            use_color = 1;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-color") == 0) {
            use_color = 0;
        } else if (strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--width") == 0) {
            if (i + 1 < argc) {
                width = atoi(argv[++i]);
                if (width <= 0) {
                    fprintf(stderr, "❌ 错误: 宽度必须大于0\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "❌ 错误: --width 需要指定数值\n");
                return 1;
            }
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mode") == 0) {
            if (i + 1 < argc) {
                charset_mode = parse_charset_mode(argv[++i]);
            } else {
                fprintf(stderr, "❌ 错误: --mode 需要指定字符集模式\n");
                fprintf(stderr, "使用 '%s --help' 查看可用的字符集模式\n", argv[0]);
                return 1;
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--resolution") == 0) {
            if (i + 1 < argc) {
                resolution_scale = (float)atof(argv[++i]);
                if (resolution_scale <= 0.0f || resolution_scale > 5.0f) {
                    fprintf(stderr, "❌ 错误: 分辨率倍数必须在 0.1 到 5.0 之间\n");
                    fprintf(stderr, "建议使用 1.5-3.0 之间的值以获得最佳效果\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "❌ 错误: --resolution 需要指定倍数\n");
                fprintf(stderr, "使用 '%s --help' 查看帮助信息\n", argv[0]);
                return 1;
            }
        } else if (argv[i][0] != '-') {
            if (!filename) {
                filename = argv[i];
            } else if (width == DEFAULT_WIDTH) {
                // 如果已经设置了文件名，且宽度还是默认值，则第二个参数是宽度
                width = atoi(argv[i]);
                if (width <= 0) {
                    fprintf(stderr, "❌ 错误: 宽度必须大于0\n");
                    return 1;
                }
            }
        } else {
            fprintf(stderr, "❌ 错误: 未知选项 '%s'\n", argv[i]);
            fprintf(stderr, "使用 '%s --help' 查看帮助信息\n", argv[0]);
            return 1;
        }
    }
    
    // 检查是否指定了图片文件
    if (!filename) {
        fprintf(stderr, "❌ 错误: 请指定图片文件\n");
        fprintf(stderr, "使用 '%s --help' 查看帮助信息\n", argv[0]);
        return 1;
    }
    
    // 检查文件是否存在
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "❌ 错误: 文件 '%s' 不存在或无法访问\n", filename);
        return 1;
    }
    fclose(file);
    
    // 显示图片
    return display_image(filename, width, use_color, charset_mode, resolution_scale);
}
