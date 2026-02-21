#pragma once

#include <string>

namespace pnana {
namespace utils {

// 获取UTF-8字符的辅助函数
// 从字符串的指定位置提取一个完整的UTF-8字符
// @param str: 输入字符串
// @param pos: 起始位置
// @return: UTF-8字符字符串，如果位置无效则返回空格
std::string getUtf8CharAt(const std::string& str, size_t pos);

// 检查字符是否为中文字符
// 检测UTF-8编码的中文字符（基本汉字范围）
// @param ch: 待检测的字符字符串
// @return: 如果是中文字符返回true，否则返回false
bool isChineseChar(const std::string& ch);

// 计算UTF-8字符的字节数
// 从指定位置向前查找，返回前一个完整UTF-8字符的字节数
// @param str: 输入字符串
// @param pos: 当前位置（字节位置）
// @return: 前一个UTF-8字符的字节数，如果位置无效或没有前一个字符则返回1
size_t getUtf8CharBytesBefore(const std::string& str, size_t pos);

// 计算UTF-8字符的字节数
// 从指定位置向后查找，返回下一个完整UTF-8字符的字节数
// @param str: 输入字符串
// @param pos: 当前位置（字节位置）
// @return: 下一个UTF-8字符的字节数，如果位置无效或没有下一个字符则返回1
size_t getUtf8CharBytesAfter(const std::string& str, size_t pos);

} // namespace utils
} // namespace pnana
