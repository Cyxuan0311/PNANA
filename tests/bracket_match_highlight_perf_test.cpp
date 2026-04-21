#include "utils/bracket_matcher.h"
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace pnana::utils;

struct BracketBenchmarkResult {
    std::string name;
    double match_ms;
    size_t total_lines;
    size_t total_chars;
    size_t scanned_chars;
    bool found_match;
};

class BenchmarkTimer {
  public:
    void start() {
        start_time_ = std::chrono::high_resolution_clock::now();
    }

    double stop() {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start_time_).count();
    }

  private:
    std::chrono::high_resolution_clock::time_point start_time_;
};

std::vector<std::string> generateCodeWithBrackets(size_t num_lines, size_t avg_line_length,
                                                  double bracket_density, size_t seed) {
    std::vector<std::string> lines;
    lines.reserve(num_lines);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> line_len_dis(avg_line_length / 2, avg_line_length * 3 / 2);
    std::uniform_real_distribution<> bracket_dis(0.0, 1.0);
    std::uniform_int_distribution<> bracket_type_dis(0, 5);
    std::uniform_int_distribution<> char_dis(97, 122);

    const char* brackets = "()[]{}";

    for (size_t i = 0; i < num_lines; ++i) {
        size_t len = line_len_dis(gen);
        std::string line;
        line.reserve(len);

        for (size_t j = 0; j < len; ++j) {
            if (bracket_dis(gen) < bracket_density) {
                line += brackets[bracket_type_dis(gen)];
            } else {
                line += static_cast<char>(char_dis(gen));
            }
        }
        lines.push_back(std::move(line));
    }

    return lines;
}

std::vector<std::string> generateNestedBrackets(size_t depth, size_t seed) {
    std::vector<std::string> lines;
    lines.reserve(depth * 2);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> bracket_type_dis(0, 2);
    const char* open_brackets = "([{";
    const char* close_brackets = ")]}";

    std::vector<int> bracket_types;
    bracket_types.reserve(depth);

    std::string indent;
    for (size_t i = 0; i < depth; ++i) {
        int type = bracket_type_dis(gen);
        bracket_types.push_back(type);
        std::string line = indent + std::string(1, open_brackets[type]);
        lines.push_back(std::move(line));
        indent += "  ";
    }

    for (size_t i = depth; i > 0; --i) {
        int type = bracket_types[i - 1];
        std::string line = indent.substr(0, (i - 1) * 2) + std::string(1, close_brackets[type]);
        lines.push_back(std::move(line));
    }

    return lines;
}

BracketBenchmarkResult runBracketBenchmark(const std::string& name,
                                           const std::vector<std::string>& lines,
                                           size_t cursor_line, size_t cursor_col,
                                           size_t max_scan_chars = 200000) {
    BracketBenchmarkResult result;
    result.name = name;
    result.total_lines = lines.size();

    size_t total_chars = 0;
    for (const auto& line : lines) {
        total_chars += line.size();
    }
    result.total_chars = total_chars;

    BenchmarkTimer timer;
    timer.start();

    auto bracket_result = findMatchingBracket(lines, cursor_line, cursor_col, max_scan_chars);

    result.match_ms = timer.stop();
    result.found_match = bracket_result.has_value();
    result.scanned_chars = 0;

    if (bracket_result.has_value()) {
        size_t start_line = std::min(cursor_line, bracket_result->matched.line);
        size_t end_line = std::max(cursor_line, bracket_result->matched.line);
        for (size_t l = start_line; l <= end_line; ++l) {
            result.scanned_chars += lines[l].size();
        }
    }

    return result;
}

void printResults(const std::vector<BracketBenchmarkResult>& results) {
    const int name_width = 30;

    std::cout << std::left << std::setw(name_width) << "Test Case";
    std::cout << std::right << std::setw(10) << "Lines";
    std::cout << std::setw(12) << "Total Chars";
    std::cout << std::setw(12) << "Match(ms)";
    std::cout << std::setw(10) << "Found";
    std::cout << std::setw(12) << "Scanned Chars";
    std::cout << std::endl;

    std::string separator(name_width + 10 + 12 + 12 + 10 + 12, '-');
    std::cout << separator << std::endl;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(name_width) << r.name;
        std::cout << std::right << std::setw(10) << r.total_lines;
        std::cout << std::setw(12) << r.total_chars;
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << r.match_ms;
        std::cout << std::setw(10) << (r.found_match ? "Yes" : "No");
        std::cout << std::setw(12) << r.scanned_chars;
        std::cout << std::endl;
    }
}

void runSimpleMatchTest() {
    std::cout << "\n=== Simple Bracket Match Test ===" << std::endl;

    std::vector<std::string> lines = {"function foo() {", "  if (x > 0) {", "    return x;", "  }",
                                      "}"};

    std::vector<BracketBenchmarkResult> results;

    results.push_back(runBracketBenchmark("Match { at (0,15)", lines, 0, 15));
    results.push_back(runBracketBenchmark("Match ( at (1,5)", lines, 1, 5));
    results.push_back(runBracketBenchmark("Match ) at (1,11)", lines, 1, 11));
    results.push_back(runBracketBenchmark("Match } at (4,0)", lines, 4, 0));
    results.push_back(runBracketBenchmark("No bracket at (2,10)", lines, 2, 10));

    printResults(results);
}

void runNestedBracketsTest() {
    std::cout << "\n=== Nested Brackets Test ===" << std::endl;

    std::vector<size_t> depths = {10, 50, 100, 200, 500};

    std::cout << std::left << std::setw(15) << "Depth";
    std::cout << std::right << std::setw(12) << "Lines";
    std::cout << std::setw(12) << "Match(ms)";
    std::cout << std::setw(10) << "Found";
    std::cout << std::endl;

    std::string separator(49, '-');
    std::cout << separator << std::endl;

    for (size_t depth : depths) {
        auto lines = generateNestedBrackets(depth, 42);
        auto result = runBracketBenchmark("", lines, 0, lines[0].size() - 1);

        std::cout << std::left << std::setw(15) << depth;
        std::cout << std::right << std::setw(12) << lines.size();
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << result.match_ms;
        std::cout << std::setw(10) << (result.found_match ? "Yes" : "No");
        std::cout << std::endl;
    }
}

void runLargeFileTest() {
    std::cout << "\n=== Large File Bracket Match Test ===" << std::endl;

    std::vector<size_t> line_counts = {100, 500, 1000, 5000, 10000};

    std::vector<BracketBenchmarkResult> results;

    for (size_t num_lines : line_counts) {
        auto lines = generateCodeWithBrackets(num_lines, 50, 0.05, 42);
        std::string name = "Code (" + std::to_string(num_lines) + " lines, 5% brackets)";
        results.push_back(runBracketBenchmark(name, lines, 0, 0));
    }

    printResults(results);
}

void runBracketDensityTest() {
    std::cout << "\n=== Bracket Density Test (1000 lines) ===" << std::endl;

    std::vector<double> densities = {0.01, 0.05, 0.10, 0.20, 0.50};

    std::cout << std::left << std::setw(15) << "Density";
    std::cout << std::right << std::setw(12) << "Match(ms)";
    std::cout << std::setw(10) << "Found";
    std::cout << std::setw(12) << "Total Chars";
    std::cout << std::endl;

    std::string separator(49, '-');
    std::cout << separator << std::endl;

    for (double density : densities) {
        auto lines = generateCodeWithBrackets(1000, 50, density, 42);
        auto result = runBracketBenchmark("", lines, 0, 0);

        std::cout << std::left << std::fixed << std::setprecision(0);
        std::cout << std::setw(15) << (density * 100) << "%";
        std::cout << std::right << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << result.match_ms;
        std::cout << std::setw(10) << (result.found_match ? "Yes" : "No");
        std::cout << std::setw(12) << result.total_chars;
        std::cout << std::endl;
    }
}

void runMaxScanLimitTest() {
    std::cout << "\n=== Max Scan Limit Test ===" << std::endl;

    auto lines = generateCodeWithBrackets(10000, 50, 0.05, 42);

    std::vector<size_t> max_scans = {1000, 10000, 50000, 100000, 200000};

    std::cout << std::left << std::setw(15) << "Max Scan";
    std::cout << std::right << std::setw(12) << "Match(ms)";
    std::cout << std::setw(10) << "Found";
    std::cout << std::endl;

    std::string separator(37, '-');
    std::cout << separator << std::endl;

    for (size_t max_scan : max_scans) {
        auto result = runBracketBenchmark("", lines, 0, 0, max_scan);

        std::cout << std::left << std::setw(15) << max_scan;
        std::cout << std::right << std::fixed << std::setprecision(3);
        std::cout << std::setw(12) << result.match_ms;
        std::cout << std::setw(10) << (result.found_match ? "Yes" : "No");
        std::cout << std::endl;
    }
}

void runRepeatedMatchTest() {
    std::cout << "\n=== Repeated Match Test (1000 iterations) ===" << std::endl;

    auto lines = generateCodeWithBrackets(1000, 50, 0.05, 42);

    BenchmarkTimer timer;
    timer.start();

    const int iterations = 1000;
    int found_count = 0;

    for (int i = 0; i < iterations; ++i) {
        size_t line = (i * 7) % lines.size();
        size_t col = 0;
        if (line < lines.size() && !lines[line].empty()) {
            col = (i * 3) % lines[line].size();
        }
        auto result = findMatchingBracket(lines, line, col);
        if (result.has_value()) {
            ++found_count;
        }
    }

    double total_ms = timer.stop();

    std::cout << "Total time for " << iterations << " matches: " << std::fixed
              << std::setprecision(2) << total_ms << " ms" << std::endl;
    std::cout << "Average per match: " << std::fixed << std::setprecision(4)
              << (total_ms / iterations) << " ms" << std::endl;
    std::cout << "Found matches: " << found_count << "/" << iterations << std::endl;
}

void runNoBracketTest() {
    std::cout << "\n=== No Bracket Test ===" << std::endl;

    std::vector<std::string> lines;
    for (size_t i = 0; i < 1000; ++i) {
        lines.push_back("this is a line without any brackets at all line number " +
                        std::to_string(i));
    }

    std::vector<BracketBenchmarkResult> results;

    results.push_back(runBracketBenchmark("No brackets (1000 lines)", lines, 500, 10));

    std::vector<std::string> large_lines;
    large_lines.reserve(5000);
    for (int i = 0; i < 5; ++i) {
        large_lines.insert(large_lines.end(), lines.begin(), lines.end());
    }
    results.push_back(runBracketBenchmark("No brackets (5000 lines)", large_lines, 2500, 10));

    printResults(results);
}

int main() {
    std::cout << "=== Bracket Matching Performance Benchmark ===" << std::endl;
    std::cout << "Testing bracket matching algorithm performance..." << std::endl;

    runSimpleMatchTest();
    runNestedBracketsTest();
    runLargeFileTest();
    runBracketDensityTest();
    runMaxScanLimitTest();
    runRepeatedMatchTest();
    runNoBracketTest();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    return 0;
}
