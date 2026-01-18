#include "utils/assembly_analyzer.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

namespace pnana {
namespace utils {

std::string AssemblyAnalyzer::getArchitectureName(Architecture arch) {
    switch (arch) {
        case Architecture::X86:
            return "x86";
        case Architecture::ARM:
            return "arm";
        case Architecture::RISCV:
            return "riscv";
        case Architecture::MIPS:
            return "mips";
        case Architecture::GENERIC:
            return "asm";
        default:
            return "unknown";
    }
}

std::string AssemblyAnalyzer::getCompilerName(Compiler compiler) {
    switch (compiler) {
        case Compiler::GCC:
            return "gcc";
        case Compiler::CLANG:
            return "clang";
        case Compiler::MSVC:
            return "msvc";
        case Compiler::NASM:
            return "nasm";
        case Compiler::GAS:
            return "gas";
        case Compiler::ARMASM:
            return "armasm";
        case Compiler::UNKNOWN:
            return "unknown";
        default:
            return "unknown";
    }
}

AssemblyAnalyzer::AnalysisResult AssemblyAnalyzer::analyzeFile(const std::string& filepath,
                                                               size_t maxLines) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return {Architecture::GENERIC, Compiler::UNKNOWN, 0.0f, "无法打开文件"};
        }

        std::string content;
        std::string line;
        size_t lineCount = 0;

        while (std::getline(file, line) && lineCount < maxLines) {
            content += line + "\n";
            lineCount++;
        }

        return analyzeContent(content);
    } catch (const std::exception& e) {
        return {Architecture::GENERIC, Compiler::UNKNOWN, 0.0f,
                std::string("分析失败: ") + e.what()};
    }
}

AssemblyAnalyzer::AnalysisResult AssemblyAnalyzer::analyzeContent(const std::string& content) {
    AnalysisResult result;

    // 预处理代码行
    auto lines = preprocessLines(content, 100);

    if (lines.empty()) {
        result.details = "文件为空或无有效内容";
        return result;
    }

    // 检测各种模式
    detectInstructionPatterns(lines, result);
    detectDirectivePatterns(lines, result);
    detectCommentPatterns(lines, result);
    detectArchitectureSpecificPatterns(lines, result);

    // 计算置信度
    calculateConfidence(result);

    return result;
}

std::vector<std::string> AssemblyAnalyzer::preprocessLines(const std::string& content,
                                                           size_t maxLines) {
    std::vector<std::string> lines;
    std::istringstream stream(content);
    std::string line;

    while (std::getline(stream, line) && lines.size() < maxLines) {
        // 移除前导和尾随空白
        line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) {
                       return !std::isspace(ch);
                   }));
        line.erase(std::find_if(line.rbegin(), line.rend(),
                                [](unsigned char ch) {
                                    return !std::isspace(ch);
                                })
                       .base(),
                   line.end());

        // 跳过空行和纯注释行
        if (!line.empty() && line.find('#') != 0 && line.find(';') != 0 && line.find("//") != 0 &&
            line.find("/*") != 0) {
            lines.push_back(line);
        }
    }

    return lines;
}

void AssemblyAnalyzer::detectInstructionPatterns(const std::vector<std::string>& lines,
                                                 AnalysisResult& result) {
    std::unordered_map<Architecture, int> archScores;
    std::unordered_map<Compiler, int> compilerScores;

    // x86 指令模式
    static const std::unordered_set<std::string> x86Instructions = {
        "mov",    "add",   "sub",  "mul",   "div",   "push",  "pop",   "call",    "ret",
        "jmp",    "je",    "jne",  "jz",    "jnz",   "jg",    "jl",    "jge",     "jle",
        "cmp",    "test",  "and",  "or",    "xor",   "not",   "shl",   "shr",     "sar",
        "rol",    "ror",   "lea",  "int",   "nop",   "hlt",   "cli",   "sti",     "in",
        "out",    "inc",   "dec",  "neg",   "adc",   "sbb",   "imul",  "idiv",    "cbw",
        "cwd",    "cdq",   "cqo",  "movsx", "movzx", "sete",  "setne", "seta",    "setae",
        "setb",   "setbe", "setg", "setge", "setl",  "setle", "cmov",  "xchg",    "bsf",
        "bsr",    "bt",    "btc",  "btr",   "bts",   "cpuid", "rdtsc", "syscall", "sysenter",
        "sysexit"};

    // ARM 指令模式
    static const std::unordered_set<std::string> armInstructions = {
        "mov", "add", "sub",   "mul",   "ldr", "str",   "push",  "pop",   "bl",   "bx",
        "b",   "beq", "bne",   "bgt",   "blt", "bge",   "ble",   "cmp",   "tst",  "and",
        "orr", "eor", "mvn",   "lsl",   "lsr", "asr",   "ror",   "bic",   "adc",  "sbc",
        "rsb", "mla", "umull", "smull", "swp", "ldmia", "stmia", "ldmfd", "stmfd"};

    // RISC-V 指令模式
    static const std::unordered_set<std::string> riscvInstructions = {
        "addi",  "slti",   "sltiu", "andi", "ori",  "xori", "slli", "srli", "srai", "add",
        "sub",   "sll",    "slt",   "sltu", "xor",  "srl",  "sra",  "or",   "and",  "fence",
        "ecall", "ebreak", "lbu",   "lhu",  "lb",   "lh",   "lw",   "sb",   "sh",   "sw",
        "ld",    "sd",     "flw",   "fsw",  "fadd", "fsub", "fmul", "fdiv", "fsqrt"};

    // MIPS 指令模式
    static const std::unordered_set<std::string> mipsInstructions = {
        "add",  "addi", "addiu",  "addu",   "sub",  "subu", "mult", "multu", "div",  "divu",
        "mfhi", "mflo", "mthi",   "mtlo",   "and",  "andi", "or",   "ori",   "xor",  "xori",
        "nor",  "sll",  "srl",    "sra",    "sllv", "srlv", "srav", "lw",    "sw",   "lb",
        "lbu",  "lh",   "lhu",    "sb",     "sh",   "lui",  "beq",  "bne",   "bgez", "bgtz",
        "blez", "bltz", "bgezal", "bltzal", "j",    "jal",  "jr"};

    for (const auto& line : lines) {
        // 转换为小写以便匹配
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        // 分割指令和操作数
        size_t spacePos = lowerLine.find(' ');
        std::string instruction =
            (spacePos != std::string::npos) ? lowerLine.substr(0, spacePos) : lowerLine;

        // 移除标签（如果有）
        if (instruction.back() == ':') {
            continue; // 这是标签，不是指令
        }

        // 检查指令是否匹配不同架构
        if (x86Instructions.count(instruction) > 0) {
            archScores[Architecture::X86]++;
        }
        if (armInstructions.count(instruction) > 0) {
            archScores[Architecture::ARM]++;
        }
        if (riscvInstructions.count(instruction) > 0) {
            archScores[Architecture::RISCV]++;
        }
        if (mipsInstructions.count(instruction) > 0) {
            archScores[Architecture::MIPS]++;
        }

        // 检测编译器特定的语法
        if (lowerLine.find(".intel_syntax") != std::string::npos ||
            lowerLine.find(".att_syntax") != std::string::npos) {
            compilerScores[Compiler::GAS]++;
        }
        if (lowerLine.find("use64") != std::string::npos ||
            lowerLine.find("use32") != std::string::npos) {
            compilerScores[Compiler::NASM]++;
        }
    }

    // 确定最可能的架构
    Architecture bestArch = Architecture::GENERIC;
    int maxScore = 0;
    for (const auto& [arch, score] : archScores) {
        if (score > maxScore) {
            maxScore = score;
            bestArch = arch;
        }
    }

    if (maxScore > 0) {
        result.arch = bestArch;
    }

    // 确定最可能的编译器
    Compiler bestCompiler = Compiler::UNKNOWN;
    int maxCompilerScore = 0;
    for (const auto& [compiler, score] : compilerScores) {
        if (score > maxCompilerScore) {
            maxCompilerScore = score;
            bestCompiler = compiler;
        }
    }

    if (maxCompilerScore > 0) {
        result.compiler = bestCompiler;
    }
}

void AssemblyAnalyzer::detectDirectivePatterns(const std::vector<std::string>& lines,
                                               AnalysisResult& result) {
    std::unordered_map<Compiler, int> compilerScores;

    for (const auto& line : lines) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        // GCC/GAS 伪指令
        if (lowerLine.find(".global") != std::string::npos ||
            lowerLine.find(".globl") != std::string::npos ||
            lowerLine.find(".data") != std::string::npos ||
            lowerLine.find(".text") != std::string::npos ||
            lowerLine.find(".bss") != std::string::npos ||
            lowerLine.find(".align") != std::string::npos ||
            lowerLine.find(".byte") != std::string::npos ||
            lowerLine.find(".word") != std::string::npos ||
            lowerLine.find(".long") != std::string::npos ||
            lowerLine.find(".quad") != std::string::npos ||
            lowerLine.find(".ascii") != std::string::npos ||
            lowerLine.find(".asciz") != std::string::npos ||
            lowerLine.find(".string") != std::string::npos) {
            compilerScores[Compiler::GAS]++;
            compilerScores[Compiler::GCC]++;
        }

        // NASM 伪指令
        if (lowerLine.find("global") != std::string::npos ||
            lowerLine.find("extern") != std::string::npos ||
            lowerLine.find("section") != std::string::npos ||
            lowerLine.find("db") != std::string::npos ||
            lowerLine.find("dw") != std::string::npos ||
            lowerLine.find("dd") != std::string::npos ||
            lowerLine.find("dq") != std::string::npos ||
            lowerLine.find("resb") != std::string::npos ||
            lowerLine.find("resw") != std::string::npos ||
            lowerLine.find("resd") != std::string::npos ||
            lowerLine.find("resq") != std::string::npos) {
            compilerScores[Compiler::NASM]++;
        }

        // ARM 伪指令
        if (lowerLine.find(".arm") != std::string::npos ||
            lowerLine.find(".thumb") != std::string::npos ||
            lowerLine.find(".code") != std::string::npos ||
            lowerLine.find(".ltorg") != std::string::npos ||
            lowerLine.find(".pool") != std::string::npos) {
            compilerScores[Compiler::ARMASM]++;
        }

        // MSVC/ML 伪指令
        if (lowerLine.find("public") != std::string::npos ||
            lowerLine.find("extern") != std::string::npos ||
            lowerLine.find("proc") != std::string::npos ||
            lowerLine.find("endp") != std::string::npos ||
            lowerLine.find("db") != std::string::npos ||
            lowerLine.find("dw") != std::string::npos ||
            lowerLine.find("dd") != std::string::npos) {
            compilerScores[Compiler::MSVC]++;
        }
    }

    // 确定最可能的编译器
    Compiler bestCompiler = result.compiler; // 保持之前的结果
    int maxScore = 0;
    for (const auto& [compiler, score] : compilerScores) {
        if (score > maxScore) {
            maxScore = score;
            bestCompiler = compiler;
        }
    }

    if (maxScore > 0) {
        result.compiler = bestCompiler;
    }
}

void AssemblyAnalyzer::detectCommentPatterns(const std::vector<std::string>& lines,
                                             AnalysisResult& result) {
    int gasComments = 0;  // /* */ 和 //
    int nasmComments = 0; // ; 注释
    int armComments = 0;  // @ 注释

    for (const auto& line : lines) {
        if (line.find("//") != std::string::npos || line.find("/*") != std::string::npos) {
            gasComments++;
        }
        if (line.find(";") != std::string::npos) {
            nasmComments++;
        }
        if (line.find("@") != std::string::npos) {
            armComments++;
        }
    }

    // 根据注释模式推断编译器
    if (gasComments > nasmComments && gasComments > armComments) {
        if (result.compiler == Compiler::UNKNOWN) {
            result.compiler = Compiler::GAS;
        }
    } else if (nasmComments > gasComments && nasmComments > armComments) {
        if (result.compiler == Compiler::UNKNOWN) {
            result.compiler = Compiler::NASM;
        }
    } else if (armComments > gasComments && armComments > nasmComments) {
        if (result.compiler == Compiler::UNKNOWN) {
            result.compiler = Compiler::ARMASM;
        }
    }
}

void AssemblyAnalyzer::detectArchitectureSpecificPatterns(const std::vector<std::string>& lines,
                                                          AnalysisResult& result) {
    int x86Patterns = 0;
    int armPatterns = 0;
    int riscvPatterns = 0;
    int mipsPatterns = 0;

    for (const auto& line : lines) {
        std::string lowerLine = line;
        std::transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);

        // x86 特定模式
        if (lowerLine.find("%rax") != std::string::npos ||
            lowerLine.find("%rbx") != std::string::npos ||
            lowerLine.find("%rsp") != std::string::npos ||
            lowerLine.find("$0x") != std::string::npos) {
            x86Patterns++;
        }

        // ARM 特定模式
        if (lowerLine.find("r0") != std::string::npos ||
            lowerLine.find("r1") != std::string::npos ||
            lowerLine.find("sp") != std::string::npos ||
            lowerLine.find("lr") != std::string::npos ||
            lowerLine.find("pc") != std::string::npos) {
            armPatterns++;
        }

        // RISC-V 特定模式
        if (lowerLine.find("a0") != std::string::npos ||
            lowerLine.find("a1") != std::string::npos ||
            lowerLine.find("zero") != std::string::npos ||
            lowerLine.find("ra") != std::string::npos ||
            lowerLine.find("sp") != std::string::npos) {
            riscvPatterns++;
        }

        // MIPS 特定模式
        if (lowerLine.find("$a0") != std::string::npos ||
            lowerLine.find("$a1") != std::string::npos ||
            lowerLine.find("$v0") != std::string::npos ||
            lowerLine.find("$v1") != std::string::npos ||
            lowerLine.find("$zero") != std::string::npos) {
            mipsPatterns++;
        }
    }

    // 根据模式数量确定架构
    std::unordered_map<Architecture, int> patternScores = {{Architecture::X86, x86Patterns},
                                                           {Architecture::ARM, armPatterns},
                                                           {Architecture::RISCV, riscvPatterns},
                                                           {Architecture::MIPS, mipsPatterns}};

    Architecture bestArch = result.arch; // 保持之前的结果
    int maxScore = 0;
    for (const auto& [arch, score] : patternScores) {
        if (score > maxScore) {
            maxScore = score;
            bestArch = arch;
        }
    }

    if (maxScore > 0) {
        result.arch = bestArch;
    }
}

void AssemblyAnalyzer::calculateConfidence(AnalysisResult& result) {
    // 基于检测到的证据计算置信度
    float confidence = 0.0f;
    std::string details;

    if (result.arch != Architecture::GENERIC) {
        confidence += 0.5f; // 架构检测成功加0.5
        details += "检测到" + getArchitectureName(result.arch) + "架构; ";
    }

    if (result.compiler != Compiler::UNKNOWN) {
        confidence += 0.3f; // 编译器检测成功加0.3
        details += "检测到" + getCompilerName(result.compiler) + "编译器; ";
    }

    // 如果置信度太低，降级到通用类型
    if (confidence < 0.3f) {
        result.arch = Architecture::GENERIC;
        result.compiler = Compiler::UNKNOWN;
        confidence = 0.1f;
        details = "置信度不足，使用通用汇编类型";
    }

    result.confidence = std::min(confidence, 1.0f);
    result.details = details.empty() ? "分析完成" : details;
}

} // namespace utils
} // namespace pnana
