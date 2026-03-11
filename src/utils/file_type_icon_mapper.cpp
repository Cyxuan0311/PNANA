#include "utils/file_type_icon_mapper.h"
#include "ui/icons.h"
#include "utils/file_type_detector.h"
#include <algorithm>
#include <cctype>

namespace pnana {
namespace utils {

namespace {

std::string getExtensionFromFilename(const std::string& filename) {
    size_t pos = filename.find_last_of('.');
    if (pos != std::string::npos && pos > 0 && pos + 1 < filename.size()) {
        return filename.substr(pos + 1);
    }
    return "";
}

} // namespace

FileTypeIconMapper::FileTypeIconMapper() {
    // 初始化时可以加载默认配置
}

std::string FileTypeIconMapper::getIcon(const std::string& file_type) const {
    // 首先检查自定义图标
    auto custom_it = custom_icons_.find(file_type);
    if (custom_it != custom_icons_.end()) {
        return custom_it->second;
    }

    // 检查默认图标（"default" 键）
    auto default_it = custom_icons_.find("default");
    if (default_it != custom_icons_.end()) {
        return default_it->second;
    }

    // 使用 icons.h 中的内置映射
    return ui::icons::getFileTypeIcon(file_type);
}

void FileTypeIconMapper::setCustomIcon(const std::string& file_type, const std::string& icon) {
    custom_icons_[file_type] = icon;
}

void FileTypeIconMapper::clearCustomIcons() {
    custom_icons_.clear();
}

std::string getIconForFile(const std::string& filename, std::string ext,
                           const FileTypeIconMapper* mapper) {
    using namespace pnana::ui;
    if (ext.empty())
        ext = getExtensionFromFilename(filename);
    std::string file_type_for_icon = FileTypeDetector::getFileTypeForIcon(filename, ext);
    std::string name_lower = filename;
    std::transform(name_lower.begin(), name_lower.end(), name_lower.begin(), ::tolower);

    // 与文件列表、状态栏共用的特殊文件名图标逻辑
    if (ext == "json" || ext == "jsonc") {
        if (name_lower == "package.json")
            return icons::PACKAGE_JSON;
        if (name_lower == "package-lock.json")
            return icons::PACKAGE_LOCK;
        if (name_lower == "composer.json")
            return icons::COMPOSER;
        if (name_lower == "tsconfig.json" || name_lower == "tsconfig.base.json")
            return icons::TSCONFIG;
        if (name_lower == ".prettierrc" || name_lower == ".prettierrc.json")
            return icons::PRETTIER;
        if (name_lower == ".eslintrc" || name_lower == ".eslintrc.json" ||
            name_lower == "eslint.config.json")
            return icons::ESLINT;
        if (name_lower == ".babelrc" || name_lower == ".babelrc.json")
            return icons::BABEL;
    }
    if (ext == "xml" || ext == "xsd" || ext == "xsl") {
        if (name_lower == "pom.xml")
            return icons::MAVEN;
    }
    if (ext == "yml" || ext == "yaml") {
        if (name_lower == ".travis.yml")
            return icons::TRAVIS;
        if (name_lower == "docker-compose.yml" || name_lower == "docker-compose.yaml")
            return icons::DOCKER_COMPOSE;
        if (name_lower.find(".github/workflows") != std::string::npos ||
            name_lower.find("workflows") != std::string::npos)
            return icons::GITHUB_ACTIONS;
    }
    if (ext == "toml") {
        if (name_lower == "cargo.toml")
            return icons::CARGO;
        if (name_lower == "pyproject.toml")
            return icons::POETRY;
    }
    if (name_lower == "cargo.lock")
        return icons::CARGO;
    if (name_lower == "poetry.lock")
        return icons::POETRY;
    if (ext == "md" || ext == "markdown") {
        if (name_lower == "readme.md" || name_lower == "readme")
            return icons::README;
        if (name_lower == "changelog.md" || name_lower == "changelog")
            return icons::CHANGELOG;
        if (name_lower == "contributing.md" || name_lower == "contributing")
            return icons::CONTRIBUTING;
    }
    if (ext == "txt") {
        if (name_lower == "license" || name_lower == "license.txt")
            return icons::LICENSE;
        if (name_lower == "authors" || name_lower == "authors.txt")
            return icons::AUTHORS;
        if (name_lower == "todo" || name_lower == "todo.txt")
            return icons::TODO;
        return icons::FILE_TEXT;
    }
    if (name_lower == ".env" || name_lower == ".env.local" || name_lower == ".env.development" ||
        name_lower == ".env.production" || name_lower == ".env.test" ||
        (name_lower.length() > 5 && name_lower.substr(0, 5) == ".env."))
        return icons::ENV;
    if (ext == "conf" || ext == "config" || ext == "ini" || ext == "cfg" || ext == "properties") {
        if (name_lower == ".editorconfig")
            return icons::EDITORCONFIG;
        return icons::CONFIG;
    }
    if (name_lower == ".gitignore" || name_lower == ".gitattributes" ||
        name_lower == ".gitmodules" || name_lower == ".gitconfig" || name_lower == ".gitkeep")
        return icons::GITIGNORE;
    if (name_lower == "dockerfile" ||
        (name_lower.length() > 11 && name_lower.substr(0, 11) == "dockerfile.") ||
        ext == "dockerignore" || name_lower == ".dockerignore")
        return icons::DOCKER;
    if (name_lower == "requirements.txt" || name_lower == "requirements-dev.txt" ||
        name_lower == "requirements-prod.txt" || name_lower == "setup.py" ||
        name_lower == "setup.cfg" || name_lower == "pipfile" || name_lower == "pipfile.lock")
        return icons::PIP;
    if (name_lower == "gemfile" || name_lower == "gemfile.lock") {
        return name_lower == "gemfile.lock" ? icons::GEMFILE_LOCK : icons::GEMFILE;
    }
    if (name_lower == "go.mod")
        return icons::GO_MOD;
    if (name_lower == "go.sum")
        return icons::GO_SUM;
    if (name_lower == "build.gradle" || name_lower == "build.gradle.kts" ||
        name_lower == "settings.gradle" || name_lower == "gradlew" ||
        name_lower == "gradle.properties")
        return icons::GRADLE;
    if (name_lower == "yarn.lock")
        return icons::YARN_LOCK;
    if (name_lower == "pnpm-lock.yaml")
        return icons::PNPM_LOCK;
    if (name_lower.find("test") != std::string::npos ||
        name_lower.find("spec") != std::string::npos || ext == "test" || ext == "spec") {
        return (ext == "spec" || name_lower.find(".spec.") != std::string::npos) ? icons::SPEC
                                                                                 : icons::TEST;
    }
    if (ext == "csv")
        return icons::CSV;
    if (ext == "tsv")
        return icons::TSV;
    if (ext == "xls" || ext == "xlsx" || ext == "xlsm")
        return icons::EXCEL;
    if (name_lower == "jenkinsfile" || name_lower == "jenkinsfile.groovy")
        return icons::JENKINS;
    if (name_lower == ".circleci" || name_lower.find("circle") != std::string::npos)
        return icons::CI;
    if (ext == "pem" || ext == "key" || ext == "crt" || ext == "cer" || ext == "cert") {
        return ext == "key" ? icons::KEY : icons::CERTIFICATE;
    }
    if (ext == "ttf" || ext == "otf" || ext == "woff" || ext == "woff2" || ext == "eot")
        return icons::FONT;
    if (ext == "tmp" || ext == "temp" || (name_lower.length() > 0 && name_lower[0] == '~') ||
        (name_lower.length() > 4 && name_lower.substr(0, 4) == ".swp"))
        return icons::TEMP;
    if (name_lower.find("cache") != std::string::npos || ext == "cache")
        return icons::CACHE;
    if (ext == "exe" || ext == "bin" || ext == "out" || ext == "app")
        return icons::EXECUTABLE;

    auto resolveByType = [&](const std::string& ft) -> std::string {
        return mapper ? mapper->getIcon(ft) : icons::getFileTypeIcon(ft);
    };
    std::string icon = resolveByType(file_type_for_icon);
    if (icon != icons::FILE)
        return icon;
    std::string base_file_type = FileTypeDetector::detectFileType(filename, ext);
    icon = resolveByType(base_file_type);
    if (icon != icons::FILE)
        return icon;
    if (file_type_for_icon == "x86" || file_type_for_icon == "arm" ||
        file_type_for_icon == "riscv" || file_type_for_icon == "mips" ||
        file_type_for_icon == "asm")
        return icons::ASSEMBLY;
    if (file_type_for_icon == "spirv")
        return icons::ASSEMBLY;
    if (file_type_for_icon == "text")
        return icons::FILE_TEXT;
    return icons::FILE;
}

} // namespace utils
} // namespace pnana
