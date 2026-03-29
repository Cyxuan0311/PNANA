#ifdef BUILD_LUA_SUPPORT

#include "plugins/icon_api.h"
#include "plugins/lua_api.h"
#include "utils/logger.h"
#include <algorithm>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace pnana {
namespace plugins {

namespace {
// 优化：使用 unordered_map 替代 100+ if 语句，O(1) 查找
static const std::unordered_map<std::string_view, const char*> kIconMap = {
    {"FOLDER", pnana::ui::icons::FOLDER},
    {"FOLDER_OPEN", pnana::ui::icons::FOLDER_OPEN},
    {"FOLDER_UP", pnana::ui::icons::FOLDER_UP},
    {"FILE", pnana::ui::icons::FILE},
    {"FILE_TEXT", pnana::ui::icons::FILE_TEXT},

    {"CPP", pnana::ui::icons::CPP},
    {"C", pnana::ui::icons::C},
    {"PYTHON", pnana::ui::icons::PYTHON},
    {"JAVASCRIPT", pnana::ui::icons::JAVASCRIPT},
    {"TYPESCRIPT", pnana::ui::icons::TYPESCRIPT},
    {"JAVA", pnana::ui::icons::JAVA},
    {"GO", pnana::ui::icons::GO},
    {"RUST", pnana::ui::icons::RUST},
    {"RUBY", pnana::ui::icons::RUBY},
    {"PHP", pnana::ui::icons::PHP},
    {"LUA", pnana::ui::icons::LUA},
    {"HTML", pnana::ui::icons::HTML},
    {"CSS", pnana::ui::icons::CSS},
    {"JSON", pnana::ui::icons::JSON},
    {"MARKDOWN", pnana::ui::icons::MARKDOWN},
    {"YAML", pnana::ui::icons::YAML},
    {"XML", pnana::ui::icons::XML},
    {"SQL", pnana::ui::icons::SQL},
    {"SHELL", pnana::ui::icons::SHELL},
    {"DOCKER", pnana::ui::icons::DOCKER},
    {"GIT", pnana::ui::icons::GIT},
    {"GIT_BRANCH", pnana::ui::icons::GIT_BRANCH},
    {"GITIGNORE", pnana::ui::icons::GITIGNORE},
    {"CMAKE", pnana::ui::icons::CMAKE},
    {"MAKEFILE", pnana::ui::icons::MAKEFILE},

    {"SEARCH", pnana::ui::icons::SEARCH},
    {"REPLACE", pnana::ui::icons::REPLACE},
    {"SAVE", pnana::ui::icons::SAVE},
    {"OPEN", pnana::ui::icons::OPEN},
    {"NEW", pnana::ui::icons::NEW},
    {"UNDO", pnana::ui::icons::UNDO},
    {"REDO", pnana::ui::icons::REDO},
    {"COPY", pnana::ui::icons::COPY},
    {"CUT", pnana::ui::icons::CUT},
    {"PASTE", pnana::ui::icons::PASTE},

    {"ARROW_UP", pnana::ui::icons::ARROW_UP},
    {"ARROW_DOWN", pnana::ui::icons::ARROW_DOWN},
    {"ARROW_LEFT", pnana::ui::icons::ARROW_LEFT},
    {"ARROW_RIGHT", pnana::ui::icons::ARROW_RIGHT},
    {"GO_TO", pnana::ui::icons::GO_TO},

    {"THEME", pnana::ui::icons::THEME},
    {"SETTINGS", pnana::ui::icons::SETTINGS},
    {"HELP", pnana::ui::icons::HELP},
    {"INFO", pnana::ui::icons::INFO},
    {"WARNING", pnana::ui::icons::WARNING},
    {"ERROR", pnana::ui::icons::ERROR},
    {"SUCCESS", pnana::ui::icons::SUCCESS},

    {"MODIFIED", pnana::ui::icons::MODIFIED},
    {"SAVED", pnana::ui::icons::SAVED},
    {"UNSAVED", pnana::ui::icons::UNSAVED},
    {"CLOSE", pnana::ui::icons::CLOSE},

    {"REFRESH", pnana::ui::icons::REFRESH},
    {"HOME", pnana::ui::icons::HOME},

    {"IMAGE", pnana::ui::icons::IMAGE},
    {"PDF", pnana::ui::icons::PDF},
    {"ARCHIVE", pnana::ui::icons::ARCHIVE},
    {"VIDEO", pnana::ui::icons::VIDEO},
    {"AUDIO", pnana::ui::icons::AUDIO},
    {"DATABASE", pnana::ui::icons::DATABASE},
    {"CONFIG", pnana::ui::icons::CONFIG},
    {"LOCK", pnana::ui::icons::LOCK},
    {"EXECUTABLE", pnana::ui::icons::EXECUTABLE},

    {"TEST", pnana::ui::icons::TEST},
    {"SPEC", pnana::ui::icons::SPEC},

    {"LOG", pnana::ui::icons::LOG},
    {"TEMP", pnana::ui::icons::TEMP},
    {"CACHE", pnana::ui::icons::CACHE},

    {"GIT_REPO", pnana::ui::icons::GIT_REPO},
    {"GIT_COMMIT", pnana::ui::icons::GIT_COMMIT},
    {"GIT_MERGE", pnana::ui::icons::GIT_MERGE},
    {"GIT_PULL_REQUEST", pnana::ui::icons::GIT_PULL_REQUEST},
    {"GIT_TAG", pnana::ui::icons::GIT_TAG},
    {"GIT_STASH", pnana::ui::icons::GIT_STASH},
    {"GIT_CONFLICT", pnana::ui::icons::GIT_CONFLICT},
    {"GIT_DIFF", pnana::ui::icons::GIT_DIFF},
    {"GIT_HISTORY", pnana::ui::icons::GIT_HISTORY},
    {"GIT_REMOTE", pnana::ui::icons::GIT_REMOTE},

    {"CHECK_CIRCLE", pnana::ui::icons::CHECK_CIRCLE},
    {"EXCLAMATION_CIRCLE", pnana::ui::icons::EXCLAMATION_CIRCLE},
    {"QUESTION_CIRCLE", pnana::ui::icons::QUESTION_CIRCLE},
    {"INFO_CIRCLE", pnana::ui::icons::INFO_CIRCLE},
    {"BELL", pnana::ui::icons::BELL},
    {"FLAG", pnana::ui::icons::FLAG},

    {"LINE_NUMBER", pnana::ui::icons::LINE_NUMBER},
    {"WORD_WRAP", pnana::ui::icons::WORD_WRAP},
    {"FULLSCREEN", pnana::ui::icons::FULLSCREEN},
    {"SPLIT", pnana::ui::icons::SPLIT},
    {"CODE", pnana::ui::icons::CODE},
    {"FUNCTION", pnana::ui::icons::FUNCTION},

    {"LSP_METHOD", pnana::ui::icons::LSP_METHOD},
    {"LSP_FUNCTION", pnana::ui::icons::LSP_FUNCTION},
    {"LSP_CONSTRUCTOR", pnana::ui::icons::LSP_CONSTRUCTOR},
    {"LSP_FIELD", pnana::ui::icons::LSP_FIELD},
    {"LSP_VARIABLE", pnana::ui::icons::LSP_VARIABLE},
    {"LSP_CLASS", pnana::ui::icons::LSP_CLASS},
    {"LSP_INTERFACE", pnana::ui::icons::LSP_INTERFACE},
    {"LSP_MODULE", pnana::ui::icons::LSP_MODULE},
    {"LSP_PROPERTY", pnana::ui::icons::LSP_PROPERTY},
    {"LSP_UNIT", pnana::ui::icons::LSP_UNIT},
    {"LSP_VALUE", pnana::ui::icons::LSP_VALUE},
    {"LSP_ENUM", pnana::ui::icons::LSP_ENUM},
    {"LSP_KEYWORD", pnana::ui::icons::LSP_KEYWORD},
    {"LSP_SNIPPET", pnana::ui::icons::LSP_SNIPPET},
    {"LSP_COLOR", pnana::ui::icons::LSP_COLOR},
    {"LSP_REFERENCE", pnana::ui::icons::LSP_REFERENCE},
    {"LSP_ENUMMEMBER", pnana::ui::icons::LSP_ENUMMEMBER},
    {"LSP_CONSTANT", pnana::ui::icons::LSP_CONSTANT},
    {"LSP_STRUCT", pnana::ui::icons::LSP_STRUCT},
    {"LSP_EVENT", pnana::ui::icons::LSP_EVENT},
    {"LSP_OPERATOR", pnana::ui::icons::LSP_OPERATOR},
    {"LSP_TYPEPARAM", pnana::ui::icons::LSP_TYPEPARAM},

    {"TERMINAL", pnana::ui::icons::TERMINAL},
    {"PACKAGE", pnana::ui::icons::PACKAGE},
};

// 优化：辅助函数，不区分大小写的比较
inline bool case_insensitive_equals(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(a[i])) !=
            std::toupper(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// 优化：查找 icon，O(1) 复杂度
const char* find_icon_fast(std::string_view name) {
    // 先尝试直接查找（最快）
    auto it = kIconMap.find(name);
    if (it != kIconMap.end()) {
        return it->second;
    }

    // 如果找不到，尝试不区分大小写的查找
    for (const auto& [key, icon] : kIconMap) {
        if (case_insensitive_equals(name, key)) {
            return icon;
        }
    }

    return nullptr;
}

} // anonymous namespace

IconAPI::IconAPI() : lua_api_(nullptr) {}

IconAPI::~IconAPI() {}

void IconAPI::registerFunctions(lua_State* L) {
    lua_getglobal(L, "vim");

    lua_newtable(L);

    lua_pushcfunction(L, lua_icon_get);
    lua_setfield(L, -2, "get");

    lua_pushcfunction(L, lua_icon_list);
    lua_setfield(L, -2, "list");

    lua_pushcfunction(L, lua_icon_has);
    lua_setfield(L, -2, "has");

    lua_pushcfunction(L, lua_icon_category);
    lua_setfield(L, -2, "category");

    lua_setfield(L, -2, "icon");
    lua_pop(L, 1);
}

// 优化：O(1) 查找替代 O(n)
const char* IconAPI::getIconByName(const std::string& name) {
    return find_icon_fast(name);
}

int IconAPI::lua_icon_get(lua_State* L) {
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushnil(L);
        return 1;
    }

    const char* icon = getIconByName(name);
    if (icon) {
        lua_pushstring(L, icon);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

int IconAPI::lua_icon_list(lua_State* L) {
    lua_newtable(L);

    std::vector<std::string> icon_names = {"FOLDER",
                                           "FOLDER_OPEN",
                                           "FOLDER_UP",
                                           "FILE",
                                           "FILE_TEXT",
                                           "CPP",
                                           "C",
                                           "PYTHON",
                                           "JAVASCRIPT",
                                           "TYPESCRIPT",
                                           "JAVA",
                                           "GO",
                                           "RUST",
                                           "RUBY",
                                           "PHP",
                                           "LUA",
                                           "HTML",
                                           "CSS",
                                           "JSON",
                                           "MARKDOWN",
                                           "YAML",
                                           "XML",
                                           "SQL",
                                           "SHELL",
                                           "DOCKER",
                                           "GIT",
                                           "GIT_BRANCH",
                                           "SEARCH",
                                           "REPLACE",
                                           "SAVE",
                                           "OPEN",
                                           "NEW",
                                           "UNDO",
                                           "REDO",
                                           "COPY",
                                           "CUT",
                                           "PASTE",
                                           "ARROW_UP",
                                           "ARROW_DOWN",
                                           "ARROW_LEFT",
                                           "ARROW_RIGHT",
                                           "GO_TO",
                                           "THEME",
                                           "SETTINGS",
                                           "HELP",
                                           "INFO",
                                           "WARNING",
                                           "ERROR",
                                           "SUCCESS",
                                           "MODIFIED",
                                           "SAVED",
                                           "UNSAVED",
                                           "CLOSE",
                                           "REFRESH",
                                           "HOME",
                                           "IMAGE",
                                           "PDF",
                                           "ARCHIVE",
                                           "VIDEO",
                                           "AUDIO",
                                           "DATABASE",
                                           "CONFIG",
                                           "LOCK",
                                           "EXECUTABLE",
                                           "TEST",
                                           "SPEC",
                                           "LOG",
                                           "TEMP",
                                           "CACHE",
                                           "GIT_REPO",
                                           "GIT_COMMIT",
                                           "GIT_MERGE",
                                           "GIT_PULL_REQUEST",
                                           "GIT_TAG",
                                           "GIT_STASH",
                                           "GIT_CONFLICT",
                                           "GIT_DIFF",
                                           "GIT_HISTORY",
                                           "GIT_REMOTE",
                                           "CHECK_CIRCLE",
                                           "EXCLAMATION_CIRCLE",
                                           "QUESTION_CIRCLE",
                                           "INFO_CIRCLE",
                                           "BELL",
                                           "FLAG",
                                           "LINE_NUMBER",
                                           "WORD_WRAP",
                                           "FULLSCREEN",
                                           "SPLIT",
                                           "CODE",
                                           "FUNCTION",
                                           "LSP_METHOD",
                                           "LSP_FUNCTION",
                                           "LSP_CONSTRUCTOR",
                                           "LSP_FIELD",
                                           "LSP_VARIABLE",
                                           "LSP_CLASS",
                                           "LSP_INTERFACE",
                                           "LSP_MODULE",
                                           "LSP_PROPERTY",
                                           "LSP_UNIT",
                                           "LSP_VALUE",
                                           "LSP_ENUM",
                                           "LSP_KEYWORD",
                                           "LSP_SNIPPET",
                                           "LSP_COLOR",
                                           "LSP_REFERENCE",
                                           "LSP_ENUMMEMBER",
                                           "LSP_CONSTANT",
                                           "LSP_STRUCT",
                                           "LSP_EVENT",
                                           "LSP_OPERATOR",
                                           "LSP_TYPEPARAM",
                                           "TERMINAL",
                                           "PACKAGE"};

    int index = 1;
    for (const auto& name : icon_names) {
        lua_pushstring(L, name.c_str());
        lua_rawseti(L, -2, index++);
    }

    return 1;
}

int IconAPI::lua_icon_has(lua_State* L) {
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushboolean(L, 0);
        return 1;
    }

    const char* icon = getIconByName(name);
    lua_pushboolean(L, icon ? 1 : 0);
    return 1;
}

int IconAPI::lua_icon_category(lua_State* L) {
    const char* name = lua_tostring(L, 1);
    if (!name) {
        lua_pushnil(L);
        return 1;
    }

    // 优化：直接使用 string_view，避免字符串拷贝
    std::string_view name_view(name);

    // 优化：预定义类别前缀检查
    if (name_view.size() >= 4) {
        // 检查 LSP_ 前缀
        if ((name_view[0] == 'L' || name_view[0] == 'l') &&
            (name_view[1] == 'S' || name_view[1] == 's') &&
            (name_view[2] == 'P' || name_view[2] == 'p') && (name_view[3] == '_')) {
            lua_pushstring(L, "lsp");
            return 1;
        }

        // 检查 GIT_ 前缀
        if ((name_view[0] == 'G' || name_view[0] == 'g') &&
            (name_view[1] == 'I' || name_view[1] == 'i') &&
            (name_view[2] == 'T' || name_view[2] == 't') && (name_view[3] == '_')) {
            lua_pushstring(L, "git");
            return 1;
        }
    }

    // 优化：使用哈希集合快速判断类别
    static const std::unordered_set<std::string_view> kLanguageIcons = {
        "CPP", "C",    "PYTHON", "JAVASCRIPT", "TYPESCRIPT", "JAVA",
        "GO",  "RUST", "RUBY",   "PHP",        "LUA"};

    static const std::unordered_set<std::string_view> kFileIcons = {"FOLDER", "FOLDER_OPEN", "FILE",
                                                                    "FILE_TEXT"};

    static const std::unordered_set<std::string_view> kActionIcons = {"SEARCH", "REPLACE", "SAVE",
                                                                      "OPEN", "NEW"};

    static const std::unordered_set<std::string_view> kNavigationIcons = {
        "ARROW_UP", "ARROW_DOWN", "ARROW_LEFT", "ARROW_RIGHT"};

    static const std::unordered_set<std::string_view> kStatusIcons = {
        "ERROR", "WARNING", "INFO", "SUCCESS", "CHECK_CIRCLE", "EXCLAMATION_CIRCLE"};

    // 辅助 Lambda：不区分大小写查找
    auto find_in_set = [&name_view](const auto& icon_set) -> bool {
        for (const auto& icon : icon_set) {
            if (icon.size() == name_view.size()) {
                bool match = true;
                for (size_t i = 0; i < icon.size(); ++i) {
                    if (std::toupper(static_cast<unsigned char>(icon[i])) !=
                        std::toupper(static_cast<unsigned char>(name_view[i]))) {
                        match = false;
                        break;
                    }
                }
                if (match)
                    return true;
            }
        }
        return false;
    };

    if (find_in_set(kLanguageIcons)) {
        lua_pushstring(L, "language");
    } else if (find_in_set(kFileIcons)) {
        lua_pushstring(L, "file");
    } else if (find_in_set(kActionIcons)) {
        lua_pushstring(L, "action");
    } else if (find_in_set(kNavigationIcons)) {
        lua_pushstring(L, "navigation");
    } else if (find_in_set(kStatusIcons)) {
        lua_pushstring(L, "status");
    } else {
        lua_pushstring(L, "other");
    }

    return 1;
}

} // namespace plugins
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
