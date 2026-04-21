#ifndef PNANA_FEATURES_INDENT_QUERY_H
#define PNANA_FEATURES_INDENT_QUERY_H

#include <string>
#include <vector>

#ifdef BUILD_TREE_SITTER_SUPPORT
#include <tree_sitter/api.h>
#endif

namespace pnana {
namespace features {

#ifdef BUILD_TREE_SITTER_SUPPORT

struct IndentCapture {
    enum class Type {
        IndentBegin,
        IndentEnd,
        IndentBranch,
        IndentDedent,
        IndentIgnore,
        IndentAlign,
        IndentAuto,
        IndentZero
    };

    Type type;
    uint32_t start_row;
    uint32_t end_row;
    uint32_t start_col;
    uint32_t end_col;
    std::string node_type;
};

class IndentQuery {
  public:
    IndentQuery();
    ~IndentQuery();

    IndentQuery(const IndentQuery&) = delete;
    IndentQuery& operator=(const IndentQuery&) = delete;

    bool loadForLanguage(const std::string& language, TSLanguage* ts_language);
    bool isLoaded() const;
    std::vector<IndentCapture> queryAtRow(TSTree* tree, uint32_t row) const;
    int computeIndentLevel(const std::vector<IndentCapture>& captures, uint32_t target_row) const;
    static std::string getConfigDir();

  private:
    TSQuery* query_;
    TSLanguage* ts_language_;
    bool loaded_;
    std::string language_;

    static IndentCapture::Type parseCaptureType(const std::string& name);
    static bool loadFileContent(const std::string& language, std::string& out_content);
};

#else

class IndentQuery {
  public:
    IndentQuery() : loaded_(false) {}
    ~IndentQuery() = default;

    IndentQuery(const IndentQuery&) = delete;
    IndentQuery& operator=(const IndentQuery&) = delete;

    bool loadForLanguage(const std::string&, void*) {
        loaded_ = false;
        return false;
    }
    bool isLoaded() const {
        return loaded_;
    }
    static std::string getConfigDir();

  private:
    bool loaded_;
};

#endif

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_INDENT_QUERY_H
