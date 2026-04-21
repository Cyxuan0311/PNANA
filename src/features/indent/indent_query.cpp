#include "features/indent/indent_query.h"
#include "utils/logger.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace pnana {
namespace features {

#ifdef BUILD_TREE_SITTER_SUPPORT

IndentQuery::IndentQuery() : query_(nullptr), ts_language_(nullptr), loaded_(false) {}

IndentQuery::~IndentQuery() {
    if (query_) {
        ts_query_delete(query_);
        query_ = nullptr;
    }
}

std::string IndentQuery::getConfigDir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    return std::string(home) + "/.config/pnana";
}

bool IndentQuery::loadFileContent(const std::string& language, std::string& out_content) {
    std::string path = getConfigDir() + "/queries/" + language + "/indents.scm";

    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    out_content = buffer.str();
    return true;
}

IndentCapture::Type IndentQuery::parseCaptureType(const std::string& name) {
    if (name == "indent.begin")
        return IndentCapture::Type::IndentBegin;
    if (name == "indent.end")
        return IndentCapture::Type::IndentEnd;
    if (name == "indent.branch")
        return IndentCapture::Type::IndentBranch;
    if (name == "indent.dedent")
        return IndentCapture::Type::IndentDedent;
    if (name == "indent.ignore")
        return IndentCapture::Type::IndentIgnore;
    if (name == "indent.align")
        return IndentCapture::Type::IndentAlign;
    if (name == "indent.auto")
        return IndentCapture::Type::IndentAuto;
    if (name == "indent.zero")
        return IndentCapture::Type::IndentZero;
    return IndentCapture::Type::IndentIgnore;
}

static std::string preprocessScmContent(const std::string& content) {
    // 直接返回原始内容，避免破坏 SCM 语法结构
    return content;
}

bool IndentQuery::loadForLanguage(const std::string& language, TSLanguage* ts_language) {
    language_ = language;
    ts_language_ = ts_language;

    if (query_) {
        ts_query_delete(query_);
        query_ = nullptr;
    }

    std::string content;
    if (!loadFileContent(language, content)) {
        loaded_ = false;
        return false;
    }

    std::string processed = preprocessScmContent(content);

    uint32_t error_offset = 0;
    TSQueryError error_type = TSQueryErrorNone;

    query_ = ts_query_new(ts_language, processed.c_str(), static_cast<uint32_t>(processed.size()),
                          &error_offset, &error_type);

    if (!query_) {
        loaded_ = false;
        return false;
    }

    loaded_ = true;
    return true;
}

bool IndentQuery::isLoaded() const {
    return loaded_;
}

std::vector<IndentCapture> IndentQuery::queryAtRow(TSTree* tree, uint32_t row) const {
    std::vector<IndentCapture> captures;

    if (!query_ || !tree || !loaded_) {
        return captures;
    }

    TSNode root = ts_tree_root_node(tree);
    if (ts_node_is_null(root)) {
        return captures;
    }

    TSQueryCursor* cursor = ts_query_cursor_new();
    if (!cursor) {
        return captures;
    }

    ts_query_cursor_exec(cursor, query_, root);

    TSPoint start_point = {0, 0};
    TSPoint end_point = {row + 1, 0};
    ts_query_cursor_set_point_range(cursor, start_point, end_point);

    TSQueryMatch match;
    while (ts_query_cursor_next_match(cursor, &match)) {
        for (uint16_t i = 0; i < match.capture_count; i++) {
            TSQueryCapture capture = match.captures[i];

            uint32_t name_len = 0;
            const char* name = ts_query_capture_name_for_id(query_, capture.index, &name_len);
            if (!name)
                continue;

            std::string capture_name(name, name_len);
            IndentCapture::Type type = parseCaptureType(capture_name);

            if (type == IndentCapture::Type::IndentIgnore) {
                continue;
            }

            TSNode node = capture.node;
            if (ts_node_is_null(node)) {
                continue;
            }

            TSPoint start = ts_node_start_point(node);
            TSPoint end = ts_node_end_point(node);

            bool is_relevant = false;
            switch (type) {
                case IndentCapture::Type::IndentBegin:
                    is_relevant = (start.row <= row && end.row >= row);
                    break;
                case IndentCapture::Type::IndentEnd:
                    is_relevant = (end.row <= row);
                    break;
                case IndentCapture::Type::IndentDedent:
                case IndentCapture::Type::IndentZero:
                    is_relevant = (start.row <= row && end.row >= row);
                    break;
                default:
                    is_relevant = true;
                    break;
            }

            if (!is_relevant) {
                continue;
            }

            IndentCapture ic;
            ic.type = type;
            ic.start_row = start.row;
            ic.end_row = end.row;
            ic.start_col = start.column;
            ic.end_col = end.column;
            ic.node_type = ts_node_type(node);

            captures.push_back(ic);
        }
    }

    ts_query_cursor_delete(cursor);

    return captures;
}

int IndentQuery::computeIndentLevel(const std::vector<IndentCapture>& captures,
                                    uint32_t target_row) const {
    int indent_level = 0;

    for (const auto& capture : captures) {
        switch (capture.type) {
            case IndentCapture::Type::IndentBegin:
                if (capture.start_row <= target_row && capture.end_row >= target_row) {
                    indent_level++;
                }
                break;

            case IndentCapture::Type::IndentEnd:
                if (capture.end_row <= target_row) {
                    indent_level = std::max(0, indent_level - 1);
                }
                break;

            case IndentCapture::Type::IndentBranch:
                break;

            case IndentCapture::Type::IndentDedent:
                if (capture.start_row < target_row && capture.end_row >= target_row) {
                    indent_level = std::max(0, indent_level - 1);
                }
                break;

            case IndentCapture::Type::IndentZero:
                if (capture.start_row < target_row && capture.end_row >= target_row) {
                    indent_level = 0;
                }
                break;

            case IndentCapture::Type::IndentAlign:
            case IndentCapture::Type::IndentAuto:
            case IndentCapture::Type::IndentIgnore:
                break;
        }
    }

    return indent_level;
}

#else

std::string IndentQuery::getConfigDir() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    return std::string(home) + "/.config/pnana";
}

#endif

} // namespace features
} // namespace pnana
