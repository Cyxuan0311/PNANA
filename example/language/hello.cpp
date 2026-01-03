#include <iostream>
#include <memory>
#include <string>
#include <vector>

// C++示例 - Hello World with modern features
class HelloWorld {
  private:
    std::string message_;
    int count_;

  public:
    // 构造函数
    HelloWorld(std::string msg, int count) : message_(msg), count_(count) {}

    // 成员函数
    void print() const {
        for (int i = 0; i < count_; ++i) {
            std::cout << message_ << " (iteration " << i << ")" << std::endl;
        }
    }

    // 静态方法
    static void staticMethod() {
        std::cout << "This is a static method" << std::endl;
    }
};

template <typename T>
class Container {
  private:
    std::vector<T> data_;

  public:
    void add(T item) {
        data_.push_back(item);
    }

    void print() const {
        for (const auto& item : data_) {
            std::cout << item << " ";
        }
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[]) {
    // 基础输出
    std::cout << "Hello, C++ World!" << std::endl;

    // 对象创建
    auto hello = std::make_unique<HelloWorld>("Hello", 3);
    hello->print();

    // 模板使用
    Container<int> intContainer;
    intContainer.add(1);
    intContainer.add(2);
    intContainer.add(3);
    intContainer.print();

    Container<std::string> stringContainer;
    stringContainer.add("Hello");
    stringContainer.add("World");
    stringContainer.print();

    // Lambda表达式
    auto lambda = [](int x) -> int {
        return x * x;
    };

    std::cout << "Lambda result: " << lambda(5) << std::endl;

    // 智能指针
    auto ptr = std::make_shared<int>(42);
    std::cout << "Shared pointer value: " << *ptr << std::endl;

    // 异常处理
    try {
        if (argc > 1) {
            throw std::runtime_error("Test exception");
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
