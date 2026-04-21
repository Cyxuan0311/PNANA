#include "features/diff/myers_diff.h"
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace pnana::features::diff;

struct DiffBenchmarkResult {
    std::string name;
    double compute_ms;
    double apply_ms;
    size_t old_lines;
    size_t new_lines;
    size_t diff_records;
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

std::vector<std::string> generateLines(size_t count, size_t line_length, size_t seed) {
    std::vector<std::string> lines;
    lines.reserve(count);

    std::mt19937 gen(seed);
    std::uniform_int_distribution<> char_dis(32, 126);
    std::uniform_int_distribution<> line_len_dis(line_length / 2, line_length);

    for (size_t i = 0; i < count; ++i) {
        size_t len = line_len_dis(gen);
        std::string line;
        line.reserve(len);
        for (size_t j = 0; j < len; ++j) {
            line += static_cast<char>(char_dis(gen));
        }
        lines.push_back(std::move(line));
    }

    return lines;
}

std::vector<std::string> generateModifiedLines(const std::vector<std::string>& original,
                                               double modify_ratio, size_t seed) {
    std::vector<std::string> modified;
    std::mt19937 gen(seed);
    std::uniform_real_distribution<> ratio_dis(0.0, 1.0);
    std::uniform_int_distribution<> line_len_dis(10, 80);
    std::uniform_int_distribution<> char_dis(32, 126);

    for (const auto& line : original) {
        if (ratio_dis(gen) > modify_ratio) {
            modified.push_back(line);
        } else {
            double action = ratio_dis(gen);
            if (action < 0.33) {
                std::string new_line;
                size_t len = line_len_dis(gen);
                new_line.reserve(len);
                for (size_t j = 0; j < len; ++j) {
                    new_line += static_cast<char>(char_dis(gen));
                }
                modified.push_back(std::move(new_line));
            } else if (action < 0.66) {
                modified.push_back(line);
                std::string new_line;
                size_t len = line_len_dis(gen);
                new_line.reserve(len);
                for (size_t j = 0; j < len; ++j) {
                    new_line += static_cast<char>(char_dis(gen));
                }
                modified.push_back(std::move(new_line));
            }
        }
    }

    return modified;
}

DiffBenchmarkResult runDiffBenchmark(const std::string& name,
                                     const std::vector<std::string>& old_lines,
                                     const std::vector<std::string>& new_lines) {
    DiffBenchmarkResult result;
    result.name = name;
    result.old_lines = old_lines.size();
    result.new_lines = new_lines.size();

    BenchmarkTimer timer;

    timer.start();
    auto diff_records = MyersDiff::compute(old_lines, new_lines);
    result.compute_ms = timer.stop();
    result.diff_records = diff_records.size();

    timer.start();
    auto applied = MyersDiff::applyForward(old_lines, diff_records);
    result.apply_ms = timer.stop();

    (void)applied;

    return result;
}

void printResults(const std::vector<DiffBenchmarkResult>& results) {
    const int name_width = 25;
    const int num_width = 15;

    std::cout << std::left << std::setw(name_width) << "Test Case";
    std::cout << std::right << std::setw(12) << "Old Lines";
    std::cout << std::setw(12) << "New Lines";
    std::cout << std::setw(12) << "Diff Records";
    std::cout << std::setw(num_width) << "Compute(ms)";
    std::cout << std::setw(num_width) << "Apply(ms)";
    std::cout << std::endl;

    std::string separator(name_width + 12 * 3 + num_width * 2, '-');
    std::cout << separator << std::endl;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(name_width) << r.name;
        std::cout << std::right << std::setw(12) << r.old_lines;
        std::cout << std::setw(12) << r.new_lines;
        std::cout << std::setw(12) << r.diff_records;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << std::setw(num_width) << r.compute_ms;
        std::cout << std::setw(num_width) << r.apply_ms;
        std::cout << std::endl;
    }
}

void runSmallFileTest() {
    std::cout << "\n=== Small File Test (100 lines, 5% modification) ===" << std::endl;

    auto old_lines = generateLines(100, 50, 42);
    auto new_lines = generateModifiedLines(old_lines, 0.05, 123);

    auto result = runDiffBenchmark("Small (100 lines)", old_lines, new_lines);

    std::vector<DiffBenchmarkResult> results = {result};
    printResults(results);
}

void runMediumFileTest() {
    std::cout << "\n=== Medium File Test (1000 lines, 10% modification) ===" << std::endl;

    auto old_lines = generateLines(1000, 60, 42);
    auto new_lines = generateModifiedLines(old_lines, 0.10, 123);

    auto result = runDiffBenchmark("Medium (1000 lines)", old_lines, new_lines);

    std::vector<DiffBenchmarkResult> results = {result};
    printResults(results);
}

void runLargeFileTest() {
    std::cout << "\n=== Large File Test (5000 lines, 15% modification) ===" << std::endl;

    auto old_lines = generateLines(5000, 70, 42);
    auto new_lines = generateModifiedLines(old_lines, 0.15, 123);

    auto result = runDiffBenchmark("Large (5000 lines)", old_lines, new_lines);

    std::vector<DiffBenchmarkResult> results = {result};
    printResults(results);
}

void runModificationRatioTest() {
    std::cout << "\n=== Modification Ratio Test (1000 lines) ===" << std::endl;

    std::vector<double> ratios = {0.01, 0.05, 0.10, 0.20, 0.50};

    std::cout << std::left << std::setw(15) << "Modify Ratio";
    std::cout << std::right << std::setw(15) << "Compute(ms)";
    std::cout << std::setw(15) << "Diff Records";
    std::cout << std::setw(15) << "Apply(ms)";
    std::cout << std::endl;

    std::string separator(60, '-');
    std::cout << separator << std::endl;

    auto base_lines = generateLines(1000, 60, 42);

    for (double ratio : ratios) {
        auto new_lines = generateModifiedLines(base_lines, ratio, 123);
        auto result = runDiffBenchmark("", base_lines, new_lines);

        std::cout << std::left << std::fixed << std::setprecision(0);
        std::cout << std::setw(15) << (ratio * 100) << "%";
        std::cout << std::right << std::fixed << std::setprecision(2);
        std::cout << std::setw(15) << result.compute_ms;
        std::cout << std::setw(15) << result.diff_records;
        std::cout << std::setw(15) << result.apply_ms;
        std::cout << std::endl;
    }
}

void runScalabilityTest() {
    std::cout << "\n=== Scalability Test (10% modification) ===" << std::endl;

    std::vector<size_t> sizes = {100, 500, 1000, 2000, 5000};

    std::cout << std::left << std::setw(15) << "Lines";
    std::cout << std::right << std::setw(15) << "Compute(ms)";
    std::cout << std::setw(15) << "Diff Records";
    std::cout << std::setw(15) << "Apply(ms)";
    std::cout << std::endl;

    std::string separator(60, '-');
    std::cout << separator << std::endl;

    for (size_t size : sizes) {
        auto old_lines = generateLines(size, 60, 42);
        auto new_lines = generateModifiedLines(old_lines, 0.10, 123);
        auto result = runDiffBenchmark("", old_lines, new_lines);

        std::cout << std::left << std::setw(15) << size;
        std::cout << std::right << std::fixed << std::setprecision(2);
        std::cout << std::setw(15) << result.compute_ms;
        std::cout << std::setw(15) << result.diff_records;
        std::cout << std::setw(15) << result.apply_ms;
        std::cout << std::endl;
    }
}

void runIdenticalFilesTest() {
    std::cout << "\n=== Identical Files Test ===" << std::endl;

    auto lines = generateLines(1000, 60, 42);

    auto result = runDiffBenchmark("Identical (1000 lines)", lines, lines);

    std::vector<DiffBenchmarkResult> results = {result};
    printResults(results);
}

void runCompletelyDifferentTest() {
    std::cout << "\n=== Completely Different Files Test ===" << std::endl;

    auto old_lines = generateLines(500, 60, 42);
    auto new_lines = generateLines(500, 60, 999);

    auto result = runDiffBenchmark("Different (500 lines)", old_lines, new_lines);

    std::vector<DiffBenchmarkResult> results = {result};
    printResults(results);
}

int main() {
    std::cout << "=== Myers Diff Algorithm Performance Benchmark ===" << std::endl;
    std::cout << "Testing LCS-based diff computation and application..." << std::endl;

    runSmallFileTest();
    runMediumFileTest();
    runLargeFileTest();
    runModificationRatioTest();
    runScalabilityTest();
    runIdenticalFilesTest();
    runCompletelyDifferentTest();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    return 0;
}
