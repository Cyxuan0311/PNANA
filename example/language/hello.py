#!/usr/bin/env python3
"""
Python示例文件 - 展示各种语法高亮特性
"""
 
import sys
import os
from typing import List, Dict, Optional
from dataclasses import dataclass

# 全局变量
GLOBAL_VAR = "Hello World"

@dataclass
class Person:
    """数据类示例"""
    name: str
    age: int
    email: Optional[str] = None

    def greet(self) -> str:
        return f"Hello, my name is {self.name}"    

class Calculator:
    """计算器类"""

    def __init__(self, initial_value: float = 0.0):
        self.value = initial_value

    def add(self, number: float) -> float:
        self.value += number
        return self.value

    def multiply(self, number: float) -> float:
        self.value *= number
        return self.value

    @staticmethod
    def factorial(n: int) -> int:
        if n <= 1:
            return 1
        return n * Calculator.factorial(n - 1)

def fibonacci(n: int) -> int:
    """递归计算斐波那契数"""
    if n <= 1:
        return n
    return fibonacci(n-1) + fibonacci(n-2)

def main():
    """主函数"""
    print("Hello, Python World!")

    # 变量和数据类型
    integer_var = 42
    float_var = 3.14159
    string_var = "Python is awesome"
    boolean_var = True
    list_var = [1, 2, 3, 4, 5]
    dict_var = {"key": "value", "number": 123}
    tuple_var = (1, 2, "three")

    # 字符串操作
    formatted_string = f"Integer: {integer_var}, Float: {float_var:.2f}"
    print(formatted_string)

    # 条件语句
    if integer_var > 40:
        print("Integer is greater than 40")
    elif integer_var == 42:
        print("The answer to everything")
    else:
        print("Integer is small")

    # 循环
    print("List iteration:")
    for item in list_var:
        print(f"  Item: {item}")

    print("Range iteration:")
    for i in range(3):
        print(f"  Iteration {i}")

    # 列表推导式
    squares = [x**2 for x in range(10) if x % 2 == 0]
    print(f"Squares: {squares}")

    # 字典推导式
    square_dict = {x: x**2 for x in range(5)}
    print(f"Square dict: {square_dict}")

    # 异常处理
    try:
        result = 10 / 0
    except ZeroDivisionError as e:
        print(f"Caught exception: {e}")
    finally:
        print("This always executes")

    # 类使用
    calc = Calculator(10)
    calc.add(5)
    calc.multiply(2)
    print(f"Calculator result: {calc.value}")

    person = Person("Alice", 30, "alice@example.com")
    print(person.greet())

    # 函数调用
    fib_10 = fibonacci(10)
    fact_5 = Calculator.factorial(5)
    print(f"Fibonacci(10) = {fib_10}")
    print(f"Factorial(5) = {fact_5}")

    # 装饰器示例（简单版本）
    def simple_decorator(func):
        def wrapper(*args, **kwargs):
            print(f"Calling {func.__name__}")
            return func(*args, **kwargs)
        return wrapper

    @simple_decorator
    def decorated_function():
        return "This function is decorated"

    print(decorated_function())

if __name__ == "__main__":
    main()
