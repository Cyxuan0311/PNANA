# FindTreeSitterLanguages.cmake
# 查找 Tree-sitter 语言库的 CMake 模块
#
# 此模块查找常用的 Tree-sitter 语言库
# 如果某个语言库未找到，对应的语言将使用原生语法高亮器
#
# 使用方式:
#   include(FindTreeSitterLanguages)
#
# 定义变量:
#   对于每个语言，定义以下变量:
#   TreeSitter_<LANG>_FOUND     - 如果找到语言库则为 TRUE
#   TreeSitter_<LANG>_LIBRARY   - 库文件路径
#
# 支持的语言:
#   CPP, C, PYTHON, JAVASCRIPT, TYPESCRIPT, JSON, MARKDOWN, BASH, RUST, GO, JAVA,
#   CMAKE, TCL, FORTRAN, HASKELL, LUA, YAML, XML, CSS, SQL, RUBY, PHP, SWIFT,
#   KOTLIN, SCALA, R, PERL, DOCKERFILE, MAKEFILE, VIM, POWERSHELL

# 查找函数
function(find_tree_sitter_language lang_name lib_names display_name)
    find_library(TreeSitter_${lang_name}_LIBRARY
        NAMES ${lib_names}
        PATHS
            /usr/lib
            /usr/lib64
            /usr/local/lib
            /usr/local/lib64
            /usr/lib/x86_64-linux-gnu
            /opt/local/lib
            /opt/homebrew/lib
            /usr/local/opt
            ${CMAKE_SOURCE_DIR}/third-party
            ~/.local/lib
            ${CMAKE_INSTALL_PREFIX}/lib
            ${CMAKE_INSTALL_PREFIX}/lib64
        PATH_SUFFIXES
            lib
            lib64
    )
    
    if(TreeSitter_${lang_name}_LIBRARY)
        set(TreeSitter_${lang_name}_FOUND TRUE PARENT_SCOPE)
        set(TreeSitter_${lang_name}_LIBRARY ${TreeSitter_${lang_name}_LIBRARY} PARENT_SCOPE)
        if(NOT TreeSitterLanguages_FIND_QUIETLY)
            message(STATUS "  ✓ Found Tree-sitter ${display_name} language library: ${TreeSitter_${lang_name}_LIBRARY}")
        endif()
    else()
        set(TreeSitter_${lang_name}_FOUND FALSE PARENT_SCOPE)
        if(NOT TreeSitterLanguages_FIND_QUIETLY)
            message(STATUS "  ✗ Tree-sitter ${display_name} language library not found")
        endif()
    endif()
endfunction()

# 查找各个语言库
if(NOT TreeSitterLanguages_FIND_QUIETLY)
    message(STATUS "Searching for Tree-sitter language libraries...")
endif()

find_tree_sitter_language(CPP "tree-sitter-cpp;libtree-sitter-cpp" "C++")
find_tree_sitter_language(C "tree-sitter-c;libtree-sitter-c" "C")
find_tree_sitter_language(PYTHON "tree-sitter-python;libtree-sitter-python" "Python")
find_tree_sitter_language(JAVASCRIPT "tree-sitter-javascript;libtree-sitter-javascript" "JavaScript")
find_tree_sitter_language(TYPESCRIPT "tree-sitter-typescript;libtree-sitter-typescript" "TypeScript")
find_tree_sitter_language(JSON "tree-sitter-json;libtree-sitter-json" "JSON")
find_tree_sitter_language(MARKDOWN "tree-sitter-markdown;libtree-sitter-markdown" "Markdown")
find_tree_sitter_language(BASH "tree-sitter-bash;libtree-sitter-bash" "Bash")
find_tree_sitter_language(RUST "tree-sitter-rust;libtree-sitter-rust" "Rust")
find_tree_sitter_language(GO "tree-sitter-go;libtree-sitter-go" "Go")
find_tree_sitter_language(JAVA "tree-sitter-java;libtree-sitter-java" "Java")
find_tree_sitter_language(CMAKE "tree-sitter-cmake;libtree-sitter-cmake" "CMake")
find_tree_sitter_language(TCL "tree-sitter-tcl;libtree-sitter-tcl" "TCL")
find_tree_sitter_language(FORTRAN "tree-sitter-fortran;libtree-sitter-fortran" "Fortran")
find_tree_sitter_language(HASKELL "tree-sitter-haskell;libtree-sitter-haskell" "Haskell")
find_tree_sitter_language(LUA "tree-sitter-lua;libtree-sitter-lua" "Lua")

# 新增语言支持
find_tree_sitter_language(YAML "tree-sitter-yaml;libtree-sitter-yaml" "YAML")
find_tree_sitter_language(XML "tree-sitter-xml;libtree-sitter-xml" "XML")
find_tree_sitter_language(CSS "tree-sitter-css;libtree-sitter-css" "CSS")
find_tree_sitter_language(SQL "tree-sitter-sql;libtree-sitter-sql" "SQL")
find_tree_sitter_language(RUBY "tree-sitter-ruby;libtree-sitter-ruby" "Ruby")
find_tree_sitter_language(PHP "tree-sitter-php;libtree-sitter-php" "PHP")
find_tree_sitter_language(SWIFT "tree-sitter-swift;libtree-sitter-swift" "Swift")
find_tree_sitter_language(KOTLIN "tree-sitter-kotlin;libtree-sitter-kotlin" "Kotlin")
find_tree_sitter_language(SCALA "tree-sitter-scala;libtree-sitter-scala" "Scala")
find_tree_sitter_language(R "tree-sitter-r;libtree-sitter-r" "R")
find_tree_sitter_language(PERL "tree-sitter-perl;libtree-sitter-perl" "Perl")
find_tree_sitter_language(DOCKERFILE "tree-sitter-dockerfile;libtree-sitter-dockerfile" "Dockerfile")
find_tree_sitter_language(MAKEFILE "tree-sitter-makefile;libtree-sitter-makefile" "Makefile")
find_tree_sitter_language(VIM "tree-sitter-vim;libtree-sitter-vim" "Vim")
find_tree_sitter_language(POWERSHELL "tree-sitter-powershell;libtree-sitter-powershell" "PowerShell")

# 新增优化语言支持
find_tree_sitter_language(MESON "tree-sitter-meson;libtree-sitter-meson" "Meson")
find_tree_sitter_language(TOML "tree-sitter-toml;libtree-sitter-toml" "TOML")
find_tree_sitter_language(NIM "tree-sitter-nim;libtree-sitter-nim" "Nim")

# 标记为高级变量
mark_as_advanced(
    TreeSitter_CPP_LIBRARY
    TreeSitter_C_LIBRARY
    TreeSitter_PYTHON_LIBRARY
    TreeSitter_JAVASCRIPT_LIBRARY
    TreeSitter_TYPESCRIPT_LIBRARY
    TreeSitter_JSON_LIBRARY
    TreeSitter_MARKDOWN_LIBRARY
    TreeSitter_BASH_LIBRARY
    TreeSitter_RUST_LIBRARY
    TreeSitter_GO_LIBRARY
    TreeSitter_JAVA_LIBRARY
    TreeSitter_CMAKE_LIBRARY
    TreeSitter_TCL_LIBRARY
    TreeSitter_FORTRAN_LIBRARY
    TreeSitter_HASKELL_LIBRARY
    TreeSitter_LUA_LIBRARY
    TreeSitter_YAML_LIBRARY
    TreeSitter_XML_LIBRARY
    TreeSitter_CSS_LIBRARY
    TreeSitter_SQL_LIBRARY
    TreeSitter_RUBY_LIBRARY
    TreeSitter_PHP_LIBRARY
    TreeSitter_SWIFT_LIBRARY
    TreeSitter_KOTLIN_LIBRARY
    TreeSitter_SCALA_LIBRARY
    TreeSitter_R_LIBRARY
    TreeSitter_PERL_LIBRARY
    TreeSitter_DOCKERFILE_LIBRARY
    TreeSitter_MAKEFILE_LIBRARY
    TreeSitter_VIM_LIBRARY
    TreeSitter_POWERSHELL_LIBRARY
    TreeSitter_MESON_LIBRARY
    TreeSitter_TOML_LIBRARY
    TreeSitter_NIM_LIBRARY
)

