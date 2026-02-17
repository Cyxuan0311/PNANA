#include <stdio.h>
#include <stdlib.h>

// C语言示例 - Hello World
int main(int argc, char* argv[]) {
    printf("Hello, C World!\n");

    // 变量声明
    int number = 42;
    float pi = 3.14159;
    char* message = "This is a test";

    // 条件语句
    if (number > 40) {
        printf("Number is greater than 40\n");
    } else {
        printf("Number is not greater than 40\n");
    }

    // 循环
    for (int i = 0; i < 5; i++) {
        printf("Iteration: %d\n", i);
    }

    // 函数调用
    return EXIT_SUCCESS;
}

// 函数定义
void example_function(void) {
    // 这是一个示例函数
    return;
}
