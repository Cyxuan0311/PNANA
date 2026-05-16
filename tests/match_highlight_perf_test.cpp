#include "utils/match_highlight.h"
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

struct MatchHighlightBenchmarkResult {
    std::string name;
    double avg_ms;
    double min_ms;
    double max_ms;
    size_t text_len;
    size_t pattern_len;
    size_t num_matches;
    size_t iterations;
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

std::string generateRandomText(size_t length, size_t seed) {
    std::string text;
    text.reserve(length);
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> char_dis(97, 122);
    for (size_t i = 0; i < length; ++i) {
        text += static_cast<char>(char_dis(gen));
    }
    return text;
}

std::string generateTextWithRepeats(size_t length, const std::string& pattern, size_t seed) {
    std::string text;
    text.reserve(length);
    std::mt19937 gen(seed);
    std::uniform_real_distribution<> insert_dis(0.0, 1.0);
    std::uniform_int_distribution<> char_dis(97, 122);

    while (text.size() < length) {
        if (insert_dis(gen) < 0.1 && text.size() + pattern.size() <= length) {
            text += pattern;
        } else {
            text += static_cast<char>(char_dis(gen));
        }
    }
    return text.substr(0, length);
}

std::string generateWorstCaseText(size_t text_len, const std::string& pattern, size_t seed) {
    std::string text;
    text.reserve(text_len);
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> char_dis(0, static_cast<int>(pattern.size() - 1));

    for (size_t i = 0; i < text_len; ++i) {
        text += pattern[char_dis(gen)];
    }
    return text;
}

MatchHighlightBenchmarkResult runBenchmark(const std::string& name, const std::string& text,
                                           const std::string& pattern, size_t iterations) {
    MatchHighlightBenchmarkResult result;
    result.name = name;
    result.text_len = text.size();
    result.pattern_len = pattern.size();
    result.iterations = iterations;

    std::vector<double> times;
    times.reserve(iterations);
    size_t total_matches = 0;

    BenchmarkTimer timer;

    for (size_t i = 0; i < iterations; ++i) {
        timer.start();
        auto element = highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        double elapsed = timer.stop();
        times.push_back(elapsed);

        if (i == 0) {
            std::string text_lower = text;
            std::string pattern_lower = pattern;
            std::transform(text_lower.begin(), text_lower.end(), text_lower.begin(), ::tolower);
            std::transform(pattern_lower.begin(), pattern_lower.end(), pattern_lower.begin(),
                           ::tolower);
            size_t pos = 0;
            while ((pos = text_lower.find(pattern_lower, pos)) != std::string::npos) {
                total_matches++;
                pos++;
            }
        }
    }

    result.num_matches = total_matches;
    result.avg_ms = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    result.min_ms = *std::min_element(times.begin(), times.end());
    result.max_ms = *std::max_element(times.begin(), times.end());

    return result;
}

void printResults(const std::vector<MatchHighlightBenchmarkResult>& results) {
    std::cout << "\n" << std::string(100, '=') << "\n";
    std::cout << "  Match Highlight Performance Benchmark Results (Boyer-Moore Algorithm)\n";
    std::cout << std::string(100, '=') << "\n\n";

    std::cout << std::left << std::setw(30) << "Test Case" << std::right << std::setw(12)
              << "Text Len" << std::setw(12) << "Pattern" << std::setw(12) << "Matches"
              << std::setw(12) << "Avg (ms)" << std::setw(12) << "Min (ms)" << std::setw(12)
              << "Max (ms)" << std::setw(12) << "Iters"
              << "\n";
    std::cout << std::string(100, '-') << "\n";

    for (const auto& r : results) {
        std::cout << std::left << std::setw(30) << r.name << std::right << std::setw(12)
                  << r.text_len << std::setw(12) << r.pattern_len << std::setw(12) << r.num_matches
                  << std::setw(12) << std::fixed << std::setprecision(4) << r.avg_ms
                  << std::setw(12) << r.min_ms << std::setw(12) << r.max_ms << std::setw(12)
                  << r.iterations << "\n";
    }

    std::cout << std::string(100, '=') << "\n";
}

int main() {
    std::vector<MatchHighlightBenchmarkResult> results;
    const size_t WARMUP_ITERS = 5;
    const size_t BENCH_ITERS = 50;

    std::cout << "Running match highlight benchmarks...\n";

    std::cout << "  [1/10] Short text, short pattern...\n";
    {
        std::string text = "the quick brown fox jumps over the lazy dog";
        std::string pattern = "fox";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(runBenchmark("Short text, short pattern", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [2/10] Medium text (1KB), short pattern...\n";
    {
        std::string text = generateRandomText(1024, 42);
        std::string pattern = "abc";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Medium text (1KB), short pattern", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [3/10] Medium text (1KB), medium pattern...\n";
    {
        std::string text = generateRandomText(1024, 43);
        std::string pattern = "abcdefghij";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Medium text (1KB), medium pattern", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [4/10] Large text (10KB), short pattern...\n";
    {
        std::string text = generateRandomText(10240, 44);
        std::string pattern = "xyz";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Large text (10KB), short pattern", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [5/10] Large text (100KB), short pattern...\n";
    {
        std::string text = generateRandomText(102400, 45);
        std::string pattern = "test";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Large text (100KB), short pattern", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [6/10] Text with many repeats...\n";
    {
        std::string text = generateTextWithRepeats(10000, "hello", 46);
        std::string pattern = "hello";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Text with many repeats (10KB)", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [7/10] Worst case for naive (partial matches)...\n";
    {
        std::string pattern = "aaaaab";
        std::string text = generateWorstCaseText(10000, "aaaaa", 47);
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Worst case partial matches (10KB)", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [8/10] No match scenario...\n";
    {
        std::string text = generateRandomText(10000, 48);
        std::string pattern = "zzzzz";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(runBenchmark("No match scenario (10KB)", text, pattern, BENCH_ITERS));
    }

    std::cout << "  [9/10] Very large text (1MB), short pattern...\n";
    {
        std::string text = generateRandomText(1048576, 49);
        std::string pattern = "code";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(runBenchmark("Very large text (1MB), short pattern", text, pattern, 10));
    }

    std::cout << "  [10/10] Long pattern in large text...\n";
    {
        std::string text = generateRandomText(102400, 50);
        std::string pattern = "abcdefghijklmnopqrst";
        for (size_t i = 0; i < WARMUP_ITERS; ++i)
            highlightMatch(text, pattern, ftxui::Color::White, ftxui::Color::Yellow);
        results.push_back(
            runBenchmark("Long pattern in large text (100KB)", text, pattern, BENCH_ITERS));
    }

    printResults(results);

    return 0;
}
