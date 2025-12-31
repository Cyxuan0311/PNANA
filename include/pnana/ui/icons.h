#ifndef PNANA_UI_ICONS_H
#define PNANA_UI_ICONS_H

#include <string>

namespace pnana {
namespace ui {
namespace icons {

// Nerd Font图标常量
// 使用 JetBrains Nerd Font 的 Unicode 字符
// 参考: https://www.nerdfonts.com/cheat-sheet

// 文件和文件夹
constexpr const char* FOLDER = "\uf07b";           // nf-fa-folder
constexpr const char* FOLDER_OPEN = "\uf07c";      // nf-fa-folder_open
constexpr const char* FOLDER_UP = "\uf062";        // nf-fa-arrow_up (上级目录)
constexpr const char* FILE = "\uf15b";             // nf-fa-file
constexpr const char* FILE_TEXT = "\uf15c";        // nf-fa-file_text_o

// 编程语言 (使用 Devicons 或 Font Awesome)
constexpr const char* CPP = "\ue61d";              // nf-dev-cplusplus
constexpr const char* C = "\ue61e";                // nf-dev-c
constexpr const char* PYTHON = "\ue63c";           // nf-dev-python
constexpr const char* JAVASCRIPT = "\ue74e";       // nf-dev-javascript
constexpr const char* TYPESCRIPT = "\ue628";       // nf-dev-typescript
constexpr const char* JAVA = "\ue256";             // nf-dev-java
constexpr const char* GO = "\ue627";               // nf-dev-go
constexpr const char* RUST = "\ue7a8";             // nf-dev-rust
constexpr const char* RUBY = "\ue739";             // nf-dev-ruby
constexpr const char* PHP = "\ue73d";              // nf-dev-php
constexpr const char* LUA = "\ue620";              // nf-dev-lua
constexpr const char* HTML = "\ue736";             // nf-dev-html5
constexpr const char* CSS = "\ue749";              // nf-dev-css3
constexpr const char* JSON = "\ue60b";             // nf-dev-json
constexpr const char* MARKDOWN = "\ue73e";         // nf-dev-markdown
constexpr const char* YAML = "\uf481";             // nf-mdi-code_braces
constexpr const char* XML = "\ue72a";              // nf-dev-xml
constexpr const char* SQL = "\uf1c0";              // nf-fa-database
constexpr const char* SHELL = "\uf489";            // nf-mdi-console
constexpr const char* DOCKER = "\ue7b0";           // nf-dev-docker
constexpr const char* GIT = "\ue702";              // nf-dev-git
constexpr const char* GITIGNORE = "\ue702";        // nf-dev-git
constexpr const char* CMAKE = "\uf489";            // nf-mdi-console
constexpr const char* MAKEFILE = "\uf489";         // nf-mdi-console

// 状态图标
constexpr const char* MODIFIED = "\uf111";     // nf-fa-circle (修改标记)
constexpr const char* SAVED = "\uf00c";        // nf-fa-check (已保存)
constexpr const char* UNSAVED = "\uf12a";      // nf-fa-exclamation (未保存)
constexpr const char* CLOSE = "\uf00d";        // nf-fa-times (关闭)

// 操作图标
constexpr const char* SEARCH = "\uf002";        // nf-fa-search (搜索)
constexpr const char* REPLACE = "\uf0e7";       // nf-fa-exchange (替换)
constexpr const char* SAVE = "\uf0c7";         // nf-fa-floppy_o (保存)
constexpr const char* OPEN = "\uf07c";         // nf-fa-folder_open (打开)
constexpr const char* NEW = "\uf016";          // nf-fa-file_o (新建)
constexpr const char* UNDO = "\uf0e2";         // nf-fa-undo (撤销)
constexpr const char* REDO = "\uf01e";         // nf-fa-repeat (重做)
constexpr const char* COPY = "\uf0c5";         // nf-fa-files_o (复制)
constexpr const char* CUT = "\uf0c4";          // nf-fa-cut (剪切)
constexpr const char* PASTE = "\uf0ea";        // nf-fa-clipboard (粘贴)

// 导航图标
constexpr const char* ARROW_UP = "\uf062";      // nf-fa-arrow_up
constexpr const char* ARROW_DOWN = "\uf063";    // nf-fa-arrow_down
constexpr const char* ARROW_LEFT = "\uf060";    // nf-fa-arrow_left
constexpr const char* ARROW_RIGHT = "\uf061";   // nf-fa-arrow_right
constexpr const char* GO_TO = "\uf0ac";         // nf-fa-external_link (跳转)

// UI元素
constexpr const char* THEME = "\uf1fc";         // nf-fa-paint_brush (主题)
constexpr const char* SETTINGS = "\uf013";     // nf-fa-cog (设置)
constexpr const char* HELP = "\uf059";         // nf-fa-question_circle (帮助)
constexpr const char* INFO = "\uf05a";          // nf-fa-info_circle (信息)
constexpr const char* WARNING = "\uf071";      // nf-fa-exclamation_triangle (警告)
constexpr const char* ERROR = "\uf06a";         // nf-fa-exclamation_circle (错误)
constexpr const char* SUCCESS = "\uf00c";       // nf-fa-check_circle (成功)

// 编辑器功能
constexpr const char* LINE_NUMBER = "\uf0cb";   // nf-fa-list_alt (行号)
constexpr const char* WORD_WRAP = "\uf0ea";     // nf-fa-arrows_alt (自动换行)
constexpr const char* FULLSCREEN = "\uf065";     // nf-fa-expand (全屏)
constexpr const char* SPLIT = "\uf0c9";          // nf-fa-columns (分屏)
constexpr const char* CODE = "\uf121";           // nf-fa-code (代码)
constexpr const char* FUNCTION = "\uf1c0";       // nf-fa-cube (函数)
constexpr const char* TAB = "\uf02e";            // nf-fa-tag (标签)
constexpr const char* SELECT = "\uf0b2";         // nf-fa-mouse_pointer (选择)
constexpr const char* HIGHLIGHT = "\uf0eb";      // nf-fa-lightbulb_o (语法高亮)
constexpr const char* LOCATION = "\uf041";       // nf-fa-map_marker (位置)

// 文件浏览器
constexpr const char* REFRESH = "\uf021";        // nf-fa-refresh (刷新)
constexpr const char* HOME = "\uf015";           // nf-fa-home (主目录)

// 其他文件类型图标
constexpr const char* IMAGE = "\uf1c5";          // nf-fa-file_image_o (图片)
constexpr const char* PDF = "\uf1c1";            // nf-fa-file_pdf_o (PDF)
constexpr const char* ARCHIVE = "\uf1c6";       // nf-fa-file_archive_o (压缩包)
constexpr const char* VIDEO = "\uf1c8";         // nf-fa-file_video_o (视频)
constexpr const char* AUDIO = "\uf1c7";         // nf-fa-file_audio_o (音频)
constexpr const char* DATABASE = "\uf1c0";      // nf-fa-database (数据库)
constexpr const char* CONFIG = "\uf013";        // nf-fa-cog (配置文件)
constexpr const char* LOCK = "\uf023";          // nf-fa-lock (锁定文件)
constexpr const char* EXECUTABLE = "\uf292";    // nf-fa-terminal (可执行文件)

// 构建和依赖管理文件
constexpr const char* PACKAGE_JSON = "\ue60e";   // nf-dev-npm (npm/package.json)
constexpr const char* PACKAGE_LOCK = "\ue60e";   // nf-dev-npm (package-lock.json)
constexpr const char* YARN = "\uf1e6";           // nf-fa-yarn (yarn.lock)
constexpr const char* CARGO = "\ue7a8";          // nf-dev-rust (Cargo.toml)
constexpr const char* PIP = "\ue63c";            // nf-dev-python (requirements.txt)
constexpr const char* MAVEN = "\ue256";          // nf-dev-java (pom.xml)
constexpr const char* GRADLE = "\ue256";         // nf-dev-java (build.gradle)
constexpr const char* GEMFILE = "\ue739";        // nf-dev-ruby (Gemfile)
constexpr const char* COMPOSER = "\ue73d";       // nf-dev-php (composer.json)
constexpr const char* GO_MOD = "\ue627";         // nf-dev-go (go.mod)
constexpr const char* GO_SUM = "\ue627";         // nf-dev-go (go.sum)

// 环境配置文件
constexpr const char* ENV = "\uf462";            // nf-mdi-key (环境变量文件)
constexpr const char* DOCKERIGNORE = "\ue7b0";   // nf-dev-docker (.dockerignore)
constexpr const char* EDITORCONFIG = "\uf013";   // nf-fa-cog (.editorconfig)
constexpr const char* PRETTIER = "\ue60b";       // nf-dev-json (.prettierrc)
constexpr const char* ESLINT = "\ue60b";         // nf-dev-json (.eslintrc)
constexpr const char* BABEL = "\ue60b";         // nf-dev-json (.babelrc)
constexpr const char* TSCONFIG = "\ue628";       // nf-dev-typescript (tsconfig.json)

// 文档文件
constexpr const char* README = "\ue73e";         // nf-dev-markdown (README)
constexpr const char* LICENSE = "\uf1c9";        // nf-fa-file_text_o (LICENSE)
constexpr const char* CHANGELOG = "\uf1c9";      // nf-fa-file_text_o (CHANGELOG)
constexpr const char* CONTRIBUTING = "\uf1c9";   // nf-fa-file_text_o (CONTRIBUTING)
constexpr const char* AUTHORS = "\uf1c9";        // nf-fa-file_text_o (AUTHORS)
constexpr const char* TODO = "\uf0ae";           // nf-fa-tasks (TODO)

// 测试文件
constexpr const char* TEST = "\uf188";           // nf-fa-flask (测试文件)
constexpr const char* SPEC = "\uf188";           // nf-fa-flask (spec文件)

// 数据文件
constexpr const char* CSV = "\uf1c3";            // nf-fa-file_text_o (CSV)
constexpr const char* TSV = "\uf1c3";            // nf-fa-file_text_o (TSV)
constexpr const char* EXCEL = "\uf1c3";          // nf-fa-file_excel_o (Excel)

// 特殊配置文件
constexpr const char* TRAVIS = "\ue77e";         // nf-dev-travis (.travis.yml)
constexpr const char* JENKINS = "\ue767";        // nf-dev-jenkins (Jenkinsfile)
constexpr const char* GITHUB_ACTIONS = "\ue702"; // nf-dev-git (GitHub Actions)
constexpr const char* CI = "\uf013";             // nf-fa-cog (CI配置文件)

// 锁文件和清单文件
constexpr const char* YARN_LOCK = "\uf1e6";      // nf-fa-yarn (yarn.lock)
constexpr const char* PNPM_LOCK = "\ue60e";      // nf-dev-npm (pnpm-lock.yaml)
constexpr const char* GEMFILE_LOCK = "\ue739";  // nf-dev-ruby (Gemfile.lock)
constexpr const char* POETRY = "\ue63c";        // nf-dev-python (poetry.lock)

// 特殊目录
constexpr const char* NODE_MODULES = "\ue60e";  // nf-dev-npm (node_modules)
constexpr const char* VENV = "\ue63c";           // nf-dev-python (虚拟环境)
constexpr const char* DOTFILES = "\uf013";       // nf-fa-cog (隐藏配置文件)

// 日志和临时文件
constexpr const char* LOG = "\uf1c9";             // nf-fa-file_text_o (日志文件)
constexpr const char* TEMP = "\uf016";            // nf-fa-file_o (临时文件)
constexpr const char* CACHE = "\uf0eb";           // nf-fa-lightbulb_o (缓存文件)

// 证书和密钥文件
constexpr const char* CERTIFICATE = "\uf023";     // nf-fa-lock (证书文件)
constexpr const char* KEY = "\uf084";             // nf-fa-key (密钥文件)
constexpr const char* PEM = "\uf023";             // nf-fa-lock (PEM文件)

// 字体和样式文件
constexpr const char* FONT = "\uf031";            // nf-fa-font (字体文件)
constexpr const char* STYLUS = "\ue749";         // nf-dev-css3 (Stylus)
constexpr const char* SASS = "\ue749";           // nf-dev-css3 (SASS/SCSS)

// 其他特殊文件
constexpr const char* DOCKER_COMPOSE = "\ue7b0"; // nf-dev-docker (docker-compose.yml)
constexpr const char* KUBERNETES = "\ufd31";     // nf-mdi-kubernetes (Kubernetes)
constexpr const char* TERRAFORM = "\uf1c0";      // nf-fa-cube (Terraform)
constexpr const char* ANSIBLE = "\uf013";        // nf-fa-cog (Ansible)
constexpr const char* VAGRANT = "\uf1c0";        // nf-fa-cube (Vagrant)

// 欢迎界面
constexpr const char* ROCKET = "\uf135";        // nf-fa-rocket (快速开始)
constexpr const char* STAR = "\uf005";          // nf-fa-star (特性)
constexpr const char* BULB = "\uf0eb";          // nf-fa-lightbulb_o (提示)
constexpr const char* BOOK = "\uf02d";          // nf-fa-book (文档)

// 终端
constexpr const char* TERMINAL = "\uf120";      // nf-fa-terminal (终端)

} // namespace icons
} // namespace ui
} // namespace pnana

#endif // PNANA_UI_ICONS_H

