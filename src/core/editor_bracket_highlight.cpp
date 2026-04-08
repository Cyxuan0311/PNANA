#include "core/editor.h"
#include "utils/bracket_matcher.h"

namespace pnana {
namespace core {

void Editor::clearBracketHighlight() {
    bracket_highlight_active_ = false;
    bracket_current_line_ = 0;
    bracket_current_col_ = 0;
    bracket_match_line_ = 0;
    bracket_match_col_ = 0;
}

void Editor::updateBracketHighlight() {
    Document* doc = getDocumentForActiveRegion();
    if (!doc) {
        clearBracketHighlight();
        bracket_cache_cursor_row_ = cursor_row_;
        bracket_cache_cursor_col_ = cursor_col_;
        bracket_cache_file_path_.clear();
        bracket_cache_document_index_.reset();
        return;
    }

    const size_t current_doc_index = getDocumentIndexForActiveRegion();
    const std::string file_path = doc->getFilePath();

    // 光标与文档都没变时不重复计算，避免频繁渲染时重复扫描
    if (bracket_cache_cursor_row_ == cursor_row_ && bracket_cache_cursor_col_ == cursor_col_ &&
        bracket_cache_document_index_.has_value() &&
        bracket_cache_document_index_.value() == current_doc_index &&
        bracket_cache_file_path_ == file_path) {
        return;
    }

    bracket_cache_cursor_row_ = cursor_row_;
    bracket_cache_cursor_col_ = cursor_col_;
    bracket_cache_document_index_ = current_doc_index;
    bracket_cache_file_path_ = file_path;

    if (cursor_row_ >= doc->lineCount()) {
        clearBracketHighlight();
        return;
    }

    const auto result =
        pnana::utils::findMatchingBracket(doc->getLines(), cursor_row_, cursor_col_);
    if (!result.has_value()) {
        clearBracketHighlight();
        return;
    }

    bracket_highlight_active_ = true;
    bracket_current_line_ = result->current.line;
    bracket_current_col_ = result->current.column;
    bracket_match_line_ = result->matched.line;
    bracket_match_col_ = result->matched.column;
}

} // namespace core
} // namespace pnana
