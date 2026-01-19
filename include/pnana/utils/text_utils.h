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

} // namespace utils
} // namespace pnana
