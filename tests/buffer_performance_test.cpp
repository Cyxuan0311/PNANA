#include "core/buffer_factory.h"
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace pnana::core;

struct BenchmarkResult {
    std::string name;
    double insert_ms;
    double remove_ms;
    double random_access_ms;
    double sequential_access_ms;
    double line_operations_ms;
    size_t memory_bytes;
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

std::string generateText(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 \n";
    std::string result;
    result.reserve(length);

    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(0, sizeof(charset) - 2);

    for (size_t i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }

    return result;
}

std::vector<size_t> generateRandomPositions(size_t count, size_t max_pos) {
    std::vector<size_t> positions(count);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> dis(0, max_pos - 1);

    for (size_t i = 0; i < count; ++i) {
        positions[i] = dis(gen);
    }

    return positions;
}

BenchmarkResult runBenchmark(BufferBackendType type, const std::string& name) {
    BenchmarkResult result;
    result.name = name;

    auto buffer = SmartBufferFactory::create(type);

    const size_t initial_size = 100000;
    const size_t insert_ops = 10000;
    const size_t access_ops = 50000;
    const size_t line_ops = 1000;

    BenchmarkTimer timer;

    // 1. 初始填充
    std::string initial_text = generateText(initial_size);
    buffer->insert(0, initial_text);

    // 2. 插入测试
    std::vector<std::string> insert_texts(insert_ops);
    for (size_t i = 0; i < insert_ops; ++i) {
        insert_texts[i] = "INSERT_" + std::to_string(i) + "\n";
    }

    timer.start();
    for (size_t i = 0; i < insert_ops; ++i) {
        size_t pos = (buffer->length() * i) / insert_ops;
        buffer->insert(pos, insert_texts[i]);
    }
    result.insert_ms = timer.stop();

    // 3. 删除测试
    timer.start();
    for (size_t i = 0; i < insert_ops / 2; ++i) {
        size_t pos = buffer->length() / 4;
        buffer->remove(pos, insert_texts[0].length());
    }
    result.remove_ms = timer.stop();

    // 4. 随机访问测试
    size_t current_length = buffer->length();
    auto random_positions = generateRandomPositions(access_ops, current_length);

    timer.start();
    size_t checksum = 0;
    for (size_t pos : random_positions) {
        char ch = buffer->getChar(pos);
        checksum += static_cast<unsigned char>(ch);
    }
    result.random_access_ms = timer.stop();

    // 5. 顺序访问测试
    timer.start();
    std::string text = buffer->getText(0, std::min(size_t(10000), buffer->length()));
    result.sequential_access_ms = timer.stop();

    // 6. 行操作测试
    size_t line_count = buffer->lineCount();
    timer.start();
    for (size_t i = 0; i < line_ops && i < line_count; ++i) {
        std::string line = buffer->getLine(i % line_count);
        checksum += line.length();
    }
    result.line_operations_ms = timer.stop();

    // 7. 内存使用
    result.memory_bytes = buffer->getMemoryUsage();

    (void)checksum;

    return result;
}

void printResults(const std::vector<BenchmarkResult>& results) {
    const int name_width = 22;
    const int num_width = 14;

    std::cout << std::left << std::setw(name_width) << "Buffer Type";
    std::cout << std::right << std::setw(num_width) << "Insert(ms)";
    std::cout << std::setw(num_width) << "Remove(ms)";
    std::cout << std::setw(num_width) << "RandAccess(ms)";
    std::cout << std::setw(num_width) << "SeqAccess(ms)";
    std::cout << std::setw(num_width) << "LineOps(ms)";
    std::cout << std::setw(num_width) << "Memory(KB)";
    std::cout << std::endl;

    std::string separator(name_width + num_width * 6, '-');
    std::cout << separator << std::endl;

    for (const auto& r : results) {
        std::cout << std::left << std::setw(name_width) << r.name;
        std::cout << std::right << std::fixed << std::setprecision(2);
        std::cout << std::setw(num_width) << r.insert_ms;
        std::cout << std::setw(num_width) << r.remove_ms;
        std::cout << std::setw(num_width) << r.random_access_ms;
        std::cout << std::setw(num_width) << r.sequential_access_ms;
        std::cout << std::setw(num_width) << r.line_operations_ms;
        std::cout << std::setw(num_width) << (r.memory_bytes / 1024.0);
        std::cout << std::endl;
    }
}

void runScalabilityTest() {
    std::cout << "\n=== Scalability Test (PieceTable) ===" << std::endl;

    std::vector<size_t> sizes = {10000, 50000, 100000, 500000, 1000000};

    std::cout << std::left << std::setw(15) << "Size";
    std::cout << std::right << std::setw(15) << "Insert(ms)";
    std::cout << std::setw(15) << "Access(ms)";
    std::cout << std::setw(15) << "Memory(KB)";
    std::cout << std::endl;

    std::string separator(60, '-');
    std::cout << separator << std::endl;

    for (size_t size : sizes) {
        auto buffer = SmartBufferFactory::create(BufferBackendType::PIECE_TABLE);

        std::string text = generateText(size);

        BenchmarkTimer timer;
        timer.start();
        buffer->insert(0, text);
        double insert_time = timer.stop();

        timer.start();
        size_t len = buffer->length();
        for (size_t i = 0; i < 1000; ++i) {
            buffer->getChar((len * i / 1000) % len);
        }
        double access_time = timer.stop();

        size_t memory = buffer->getMemoryUsage();

        std::cout << std::left << std::setw(15) << size;
        std::cout << std::right << std::fixed << std::setprecision(2);
        std::cout << std::setw(15) << insert_time;
        std::cout << std::setw(15) << access_time;
        std::cout << std::setw(15) << (memory / 1024.0);
        std::cout << std::endl;
    }
}

void runLargeFileTest() {
    std::cout << "\n=== Large File Test (10MB insert) ===" << std::endl;

    std::vector<BufferBackendType> types = {
        BufferBackendType::GAP_BUFFER, BufferBackendType::SQRT_DECOMPOSITION,
        BufferBackendType::ROPE, BufferBackendType::PIECE_TABLE};

    std::vector<std::string> names = {"GapBuffer", "SqrtDecomposition", "Rope", "PieceTable"};

    const size_t large_size = 10 * 1024 * 1024;
    std::string large_text = generateText(large_size);

    std::cout << std::left << std::setw(22) << "Buffer Type";
    std::cout << std::right << std::setw(15) << "Load(ms)";
    std::cout << std::setw(15) << "Memory(KB)";
    std::cout << std::endl;

    std::string separator(52, '-');
    std::cout << separator << std::endl;

    for (size_t i = 0; i < types.size(); ++i) {
        auto buffer = SmartBufferFactory::create(types[i]);

        BenchmarkTimer timer;
        timer.start();
        buffer->insert(0, large_text);
        double load_time = timer.stop();

        size_t memory = buffer->getMemoryUsage();

        std::cout << std::left << std::setw(22) << names[i];
        std::cout << std::right << std::fixed << std::setprecision(2);
        std::cout << std::setw(15) << load_time;
        std::cout << std::setw(15) << (memory / 1024.0);
        std::cout << std::endl;
    }
}

int main() {
    std::cout << "=== Buffer Performance Benchmark ===" << std::endl;
    std::cout << "Testing four buffer backends with various operations..." << std::endl;

    std::vector<BufferBackendType> types = {
        BufferBackendType::GAP_BUFFER, BufferBackendType::SQRT_DECOMPOSITION,
        BufferBackendType::ROPE, BufferBackendType::PIECE_TABLE};

    std::vector<std::string> names = {"GapBuffer", "SqrtDecomposition", "Rope", "PieceTable"};

    std::vector<BenchmarkResult> results;

    for (size_t i = 0; i < types.size(); ++i) {
        std::cout << "\nRunning benchmark for " << names[i] << "..." << std::endl;
        results.push_back(runBenchmark(types[i], names[i]));
    }

    printResults(results);

    runScalabilityTest();

    runLargeFileTest();

    std::cout << "\n=== Benchmark Complete ===" << std::endl;

    return 0;
}
