#ifndef PNANA_FEATURES_AUTO_INDENT_ENGINE_H
#define PNANA_FEATURES_AUTO_INDENT_ENGINE_H

#include "core/config_manager.h"
#include "features/indent/indent_query.h"
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef BUILD_TREE_SITTER_SUPPORT
#include <tree_sitter/api.h>
#endif

namespace pnana {
namespace features {

class AutoIndentEngine {
  public:
    explicit AutoIndentEngine(const core::ConfigManager& config_manager);
    ~AutoIndentEngine();

    AutoIndentEngine(const AutoIndentEngine&) = delete;
    AutoIndentEngine& operator=(const AutoIndentEngine&) = delete;

    void setFileType(const std::string& file_type);
    void setFileType(const std::string& file_type, const core::LanguageIndentConfig& user_config);
    void setIndentConfig(const core::LanguageIndentConfig& config);

    std::string computeIndent(const std::vector<std::string>& lines, size_t cursor_row,
                              size_t cursor_col) const;

    std::string computeIndentAfterNewline(const std::vector<std::string>& lines, size_t cursor_row,
                                          size_t cursor_col) const;

    core::LanguageIndentConfig getDefaultConfigForLanguage(const std::string& language_id) const;

    bool isTreeSitterEnabled() const;

  private:
    const core::ConfigManager& config_manager_;
    std::string file_type_;
    core::LanguageIndentConfig indent_config_;

#ifdef BUILD_TREE_SITTER_SUPPORT
    TSParser* parser_;
    TSLanguage* current_language_;
    std::map<std::string, TSLanguage*> language_map_;
    std::map<std::string, IndentQuery> indent_query_map_;

    void initializeLanguages();
    TSLanguage* getLanguageForFileType(const std::string& file_type);
    IndentQuery* getIndentQueryForFileType(const std::string& file_type);

    int computeIndentFromTree(const std::vector<std::string>& lines, size_t cursor_row,
                              size_t cursor_col) const;
#endif

    int computeIndentFallback(const std::vector<std::string>& lines, size_t cursor_row,
                              size_t cursor_col) const;

    std::string indentToString(int level) const;
    int countLeadingSpaces(const std::string& line) const;
    int spacesToIndentLevel(int spaces) const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_AUTO_INDENT_ENGINE_H
