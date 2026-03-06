// C3 语言示例 - Hello World
// C3 是 C 的继任语言，见 https://c3-lang.org

import std::io;

fn void main()
{
    io::printn("Hello, C3 World!");

    // 变量声明
    int number = 42;
    double pi = 3.14159;

    // 条件语句
    if (number > 40)
    {
        io::printn("Number is greater than 40");
    }
    else
    {
        io::printn("Number is not greater than 40");
    }

    // 循环
    for (int i = 0; i < 5; i++)
    {
        io::printf("Iteration: %d\n", i);
    }

    // 函数调用
    example_function();
}

// 函数定义
fn void example_function()
{
    // 这是一个示例函数
    return;
}
