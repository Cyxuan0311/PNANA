#ifndef PNANA_FEATURES_SEARCH_H
#define PNANA_FEATURES_SEARCH_H

#include <regex>
#include <string>
#include <vector>

namespace pnana {
namespace features {

// 搜索匹配结果
struct SearchMatch {
    size_t line;
    size_t column;
    size_t length;

    SearchMatch(size_t l, size_t c, size_t len) : line(l), column(c), length(len) {}
};

// 搜索选项
struct SearchOptions {
    bool case_sensitive = false;
    bool whole_word = false;
    bool regex = false;
    bool wrap_around = true;

    SearchOptions() = default;
};

// 搜索引擎
class SearchEngine {
  public:
    SearchEngine();

    // 执行搜索
    void search(const std::string& pattern, const std::vector<std::string>& lines,
                const SearchOptions& options = SearchOptions());

    // 查找下一个/上一个
    bool findNext();
    bool findPrevious();

    // 跳转到匹配
    bool jumpToMatch(size_t index);

    // 替换
    bool replaceCurrentMatch(const std::string& replacement, std::vector<std::string>& lines);
    size_t replaceAll(const std::string& replacement, std::vector<std::string>& lines);

    // 获取匹配信息
    const SearchMatch* getCurrentMatch() const;
    size_t getCurrentMatchIndex() const {
        return current_match_index_;
    }
    size_t getTotalMatches() const {
        return matches_.size();
    }
    const std::vector<SearchMatch>& getAllMatches() const {
        return matches_;
    }

    // 搜索状态
    bool hasMatches() const {
        return !matches_.empty();
    }
    std::string getPattern() const {
        return pattern_;
    }
    void clearSearch();

    // 高亮检查
    bool isHighlightPosition(size_t line, size_t col) const;

  private:
    std::string pattern_;
    SearchOptions options_;
    std::vector<SearchMatch> matches_;
    size_t current_match_index_;

    // 内部搜索方法
    void searchLiteral(const std::string& pattern, const std::vector<std::string>& lines);
    void searchRegex(const std::string& pattern, const std::vector<std::string>& lines);
    void searchWholeWord(const std::string& pattern, const std::vector<std::string>& lines);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SEARCH_H
