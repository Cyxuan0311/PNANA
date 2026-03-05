#ifndef PNANA_FEATURES_COMMENT_COMMENT_SYNTAX_H
#define PNANA_FEATURES_COMMENT_COMMENT_SYNTAX_H

#include <string>
#include <utility>

namespace pnana {
namespace features {
namespace comment {

// 根据文件类型获取行注释前缀（单行注释）
// 返回: (prefix, suffix)，suffix 为空表示纯行注释如 // # --
// 对于 HTML/XML 返回 ("<!-- ", " -->")
std::pair<std::string, std::string> getCommentSyntax(const std::string& file_type);

// 切换行注释状态：若行已注释则取消，否则添加
// 返回: (新行内容, 光标列偏移量)
std::pair<std::string, int> toggleLineComment(const std::string& line, const std::string& prefix,
                                              const std::string& suffix);

// 根据文件类型切换单行注释（封装 getCommentSyntax + toggleLineComment）
// 供 core 层调用，features 负责注释类型判断与字符添加
// 返回: (新行内容, 光标列偏移量)
std::pair<std::string, int> toggleCommentForLine(const std::string& line,
                                                 const std::string& file_type);

} // namespace comment
} // namespace features
} // namespace pnana

#endif
