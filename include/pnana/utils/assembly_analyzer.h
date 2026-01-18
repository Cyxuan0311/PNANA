#pragma once

#include <string>
#include <vector>

namespace pnana {
namespace utils {

/**
 * @brief 汇编文件分析器
 *
 * 通过分析汇编文件内容来识别不同的架构和编译器类型
 */
class AssemblyAnalyzer {
  public:
    /**
     * @brief 支持的汇编架构类型
     */
    enum class Architecture {
        X86,    // x86/x64 架构
        ARM,    // ARM 架构
        RISCV,  // RISC-V 架构
        MIPS,   // MIPS 架构
        GENERIC // 通用汇编
    };

    /**
     * @brief 编译器类型
     */
    enum class Compiler {
        GCC,    // GNU Compiler Collection
        CLANG,  // LLVM Clang
        MSVC,   // Microsoft Visual C++
        NASM,   // Netwide Assembler
        GAS,    // GNU Assembler
        ARMASM, // ARM Assembler
        UNKNOWN // 未知编译器
    };

    /**
     * @brief 分析结果
     */
    struct AnalysisResult {
        Architecture arch = Architecture::GENERIC;
        Compiler compiler = Compiler::UNKNOWN;
        float confidence = 0.0f; // 0.0 到 1.0 之间的置信度
        std::string details;     // 分析详情
    };

    /**
     * @brief 分析汇编文件
     *
     * @param filepath 文件路径
     * @param maxLines 最大分析行数（防止大文件影响性能）
     * @return AnalysisResult 分析结果
     */
    static AnalysisResult analyzeFile(const std::string& filepath, size_t maxLines = 100);

    /**
     * @brief 分析汇编文本内容
     *
     * @param content 汇编文本内容
     * @return AnalysisResult 分析结果
     */
    static AnalysisResult analyzeContent(const std::string& content);

    /**
     * @brief 获取架构名称字符串
     */
    static std::string getArchitectureName(Architecture arch);

    /**
     * @brief 获取编译器名称字符串
     */
    static std::string getCompilerName(Compiler compiler);

  private:
    /**
     * @brief 检测指令模式
     */
    static void detectInstructionPatterns(const std::vector<std::string>& lines,
                                          AnalysisResult& result);

    /**
     * @brief 检测伪指令模式
     */
    static void detectDirectivePatterns(const std::vector<std::string>& lines,
                                        AnalysisResult& result);

    /**
     * @brief 检测注释和语法模式
     */
    static void detectCommentPatterns(const std::vector<std::string>& lines,
                                      AnalysisResult& result);

    /**
     * @brief 检测架构特定的模式
     */
    static void detectArchitectureSpecificPatterns(const std::vector<std::string>& lines,
                                                   AnalysisResult& result);

    /**
     * @brief 计算最终置信度
     */
    static void calculateConfidence(AnalysisResult& result);

    /**
     * @brief 清理和预处理代码行
     */
    static std::vector<std::string> preprocessLines(const std::string& content, size_t maxLines);
};

} // namespace utils
} // namespace pnana
