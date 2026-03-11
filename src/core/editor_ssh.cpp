#include "core/editor.h"
#include "features/cursor/cursor_renderer.h"
#include "features/package_manager/package_manager_registry.h"
#include "features/ssh/ssh_client.h"
#include <ftxui/component/event.hpp>
#include <regex>
#include <sstream>
#include <thread>

namespace pnana {
namespace core {

// 规范化远程路径：去掉末尾 slash（根目录保留 /）
static std::string normalizeRemotePath(const std::string& path) {
    if (path.empty() || path == "/")
        return path;
    std::string p = path;
    while (p.size() > 1 && p.back() == '/')
        p.pop_back();
    return p;
}

// 获取父目录路径
static std::string parentRemotePath(const std::string& path) {
    if (path.empty() || path == "/")
        return "/";
    std::string p = normalizeRemotePath(path);
    size_t pos = p.rfind('/');
    if (pos == std::string::npos)
        return "/";
    if (pos == 0)
        return "/";
    return p.substr(0, pos);
}

// 辅助函数：解析SSH路径 (ssh://user@host:port/path)
bool parseSSHPath(const std::string& path, pnana::ui::SSHConfig& config) {
    if (path.find("ssh://") != 0) {
        return false;
    }

    // 移除 ssh:// 前缀
    std::string remaining = path.substr(6);

    // 解析格式: user@host:port/path 或 user@host/path
    std::regex ssh_regex(R"(^([^@]+)@([^:/]+)(?::(\d+))?(/.*)$)");
    std::smatch match;

    if (std::regex_match(remaining, match, ssh_regex)) {
        config.user = match[1].str();
        config.host = match[2].str();
        if (match[3].matched && !match[3].str().empty()) {
            try {
                config.port = std::stoi(match[3].str());
            } catch (...) {
                config.port = 22;
            }
        } else {
            config.port = 22;
        }
        config.remote_path = match[4].str();
        return true;
    }

    return false;
}

void Editor::showSSHDialog() {
    // 将编辑器当前光标配置同步到对话框，确保弹窗内光标样式/颜色一致
    pnana::ui::CursorConfig cursor_cfg;
    cursor_cfg.style = getCursorStyle();
    cursor_cfg.color = getCursorColor();
    cursor_cfg.smooth = getCursorSmooth();
    cursor_cfg.blink_enabled = cursor_config_dialog_.getBlinkEnabled();
    ssh_dialog_.setCursorConfig(cursor_cfg);

    ssh_dialog_.show(
        [this](const pnana::ui::SSHConfig& config) {
            handleSSHConnect(config);
        },
        [this]() {
            setStatusMessage("SSH connection cancelled");
        },
        &current_ssh_config_,
        [this]() {
            disconnectSSH();
        });
}

// 构建 ssh:// URI
static std::string buildSSHUri(const pnana::ui::SSHConfig& c, const std::string& remotePath) {
    std::ostringstream o;
    o << "ssh://" << c.user << "@" << c.host;
    if (c.port != 22)
        o << ":" << c.port;
    o << (remotePath.empty() || remotePath[0] != '/' ? "/" : "") << remotePath;
    return o.str();
}

void Editor::handleSSHConnect(const pnana::ui::SSHConfig& config) {
    if (config.host.empty() || config.user.empty() || config.remote_path.empty()) {
        setStatusMessage("SSH: Missing required fields (host, user, or remote path)");
        return;
    }
    if (config.password.empty() && config.key_path.empty()) {
        setStatusMessage("SSH: Password or key path required");
        return;
    }

    setStatusMessage("SSH: Connecting to " + config.host + "...");
    current_ssh_config_ = config;

    // 获取远端 HOME 目录（用于将 ~/ 路径展开为绝对路径）
    {
        features::ssh::Client home_client;
        features::ssh::Result home_result =
            home_client.runCommand(current_ssh_config_, ".", "echo $HOME");
        ssh_remote_home_ = home_result.content;
        while (!ssh_remote_home_.empty() &&
               (ssh_remote_home_.back() == '\n' || ssh_remote_home_.back() == '\r'))
            ssh_remote_home_.pop_back();
        if (ssh_remote_home_.empty()) {
            // 回退：用 /home/user
            ssh_remote_home_ = "/home/" + config.user;
        }
    }

    // 为所有包管理器和 git 面板注入 SSH remote executor（cwd 用 "." 避免 '~' 引号转义问题）
    {
        std::string remote_label = config.user + "@" + config.host;
        std::string remote_path = config.remote_path.empty() ? "." : config.remote_path;

        auto make_ssh_executor = [this]() {
            return [this](const std::string& cmd) -> std::pair<bool, std::string> {
                features::ssh::Client client;
                features::ssh::Result r = client.runCommand(current_ssh_config_, ".", cmd);
                return {r.success, r.content};
            };
        };

        pnana::features::package_manager::PackageManagerRegistry::getInstance()
            .setRemoteExecutorForAll(make_ssh_executor(), remote_label);

        git_panel_.setRemoteExecutor(make_ssh_executor(), remote_label, remote_path);

        // TUI 配置：设置远程路径检测器，并异步批量预检测，避免打开弹窗时逐个 SSH 连接
        tui_config_manager_.setRemotePathChecker([this](const std::string& path) -> bool {
            // 回退用：缓存命中后不再调用此函数
            std::string path_arg = path;
            if (path_arg.size() >= 2 && path_arg.compare(0, 2, "~/") == 0)
                path_arg = path_arg.substr(2);
            else if (path_arg == "~")
                path_arg = ".";
            auto shellQuote = [](const std::string& s) -> std::string {
                std::string q = "'";
                for (char c : s) {
                    if (c == '\'')
                        q += "'\\''";
                    else
                        q += c;
                }
                return q + "'";
            };
            std::string cmd =
                "test -f " + shellQuote(path_arg) + " || test -d " + shellQuote(path_arg);
            features::ssh::Client client;
            features::ssh::Result r = client.runCommand(current_ssh_config_, ".", cmd);
            return r.success;
        });

        // resolver：将 ~/ 展开为远端真实绝对路径，再构造 ssh:// URI
        tui_config_manager_.setRemotePathResolver([this](const std::string& path) {
            // 展开 ~/
            std::string resolved = path;
            if (resolved.size() >= 2 && resolved.compare(0, 2, "~/") == 0) {
                resolved = ssh_remote_home_ + resolved.substr(1); // ~/xxx -> /home/user/xxx
            } else if (resolved == "~") {
                resolved = ssh_remote_home_;
            }
            std::ostringstream o;
            o << "ssh://" << current_ssh_config_.user << "@" << current_ssh_config_.host;
            if (current_ssh_config_.port != 22)
                o << ":" << current_ssh_config_.port;
            o << (resolved.empty() || resolved[0] != '/' ? "/" : "") << resolved;
            return o.str();
        });

        // 后台线程批量预检测所有 TUI 配置路径（一次 SSH 命令）
        std::thread([this, executor = make_ssh_executor()]() {
            tui_config_manager_.prefetchAvailableRemoteConfigs(executor);
        }).detach();

        // File picker 注入远程目录列举器
        // 使用 ssh listDir 接口（与 file_browser 共享相同逻辑）
        file_picker_.setRemoteLoader(
            [this](const std::string& path) -> std::vector<std::pair<std::string, bool>> {
                pnana::ui::SSHConfig c = current_ssh_config_;
                c.remote_path = path;
                features::ssh::Client client;
                features::ssh::Result r = client.listDir(c);
                if (!r.success)
                    return {};
                std::vector<std::pair<std::string, bool>> result;
                for (const auto& e : features::ssh::parseListDirContent(r.content)) {
                    result.emplace_back(e.name, e.is_directory);
                }
                return result;
            },
            remote_path);
    }

    if (!file_browser_.isRemoteMode()) {
        last_local_directory_ = file_browser_.getCurrentDirectory();
        if (last_local_directory_.empty())
            last_local_directory_ = ".";
    }

    file_browser_.setRemoteLoader([this](const std::string& ssh_uri) {
        pnana::ui::SSHConfig c;
        if (!parseSSHPath(ssh_uri, c))
            return std::vector<features::FileItem>{};
        features::ssh::Client client;
        features::ssh::Result r = client.listDir(c);
        if (!r.success)
            return std::vector<features::FileItem>{};
        std::vector<features::ssh::RemoteDirEntry> entries =
            features::ssh::parseListDirContent(r.content);
        std::string base = buildSSHUri(current_ssh_config_, c.remote_path);
        if (!base.empty() && base.back() == '/')
            base.pop_back();
        std::vector<features::FileItem> items;
        if (c.remote_path != "/" && !c.remote_path.empty()) {
            std::string parent = parentRemotePath(c.remote_path);
            items.push_back(
                features::FileItem("..", buildSSHUri(current_ssh_config_, parent), true, 0));
        }
        for (const auto& e : entries) {
            std::string full = (c.remote_path == "/" ? "/" : c.remote_path + "/") + e.name;
            items.push_back(features::FileItem(e.name, buildSSHUri(current_ssh_config_, full),
                                               e.is_directory, 0));
        }
        return items;
    });

    // 注入文件操作执行器（删除/重命名/创建目录/粘贴等均走 SSH）
    file_browser_.setRemoteFileOpExecutor(
        [this](const std::string& cmd) -> std::pair<bool, std::string> {
            features::ssh::Client client;
            features::ssh::Result r = client.runCommand(current_ssh_config_, ".", cmd);
            return {r.success, r.content};
        });

    features::ssh::Client ssh_client;
    std::string path = normalizeRemotePath(config.remote_path);
    pnana::ui::SSHConfig type_config = config;
    type_config.remote_path = path;
    features::ssh::Result type_result = ssh_client.getPathType(type_config);
    if (!type_result.success) {
        setStatusMessage("SSH Error: " + type_result.error);
        file_browser_.clearRemoteLoader();
        file_browser_.clearRemoteFileOpExecutor();
        return;
    }

    std::string path_type = type_result.content;
    while (!path_type.empty() && (path_type.back() == '\r' || path_type.back() == '\n'))
        path_type.pop_back();

    std::string ssh_uri = buildSSHUri(config, path);

    if (path_type == "dir") {
        file_browser_.openDirectory(ssh_uri);
        file_browser_.setVisible(true);
        terminal_.startSSHSession(config.host, config.user, config.port, config.key_path,
                                  config.password);
        setStatusMessage("SSH: " + config.user + "@" + config.host + " | Del: disconnect");
        return;
    }

    if (path_type == "file") {
        features::ssh::Result result = ssh_client.readFile(type_config);
        if (!result.success) {
            setStatusMessage("SSH Error: " + result.error);
            file_browser_.clearRemoteLoader();
            file_browser_.clearRemoteFileOpExecutor();
            return;
        }
        std::ostringstream local_filename;
        local_filename << "ssh://" << config.user << "@" << config.host;
        if (config.port != 22)
            local_filename << ":" << config.port;
        local_filename << path;
        size_t doc_index = document_manager_.createNewDocument();
        Document* doc = document_manager_.getDocument(doc_index);
        if (!doc) {
            setStatusMessage("SSH: Failed to create document");
            file_browser_.clearRemoteLoader();
            file_browser_.clearRemoteFileOpExecutor();
            return;
        }
        doc->setFilePath(local_filename.str());
        std::istringstream iss(result.content);
        std::string line;
        std::vector<std::string> lines;
        while (std::getline(iss, line))
            lines.push_back(line);
        if (lines.empty())
            lines.push_back("");
        doc->getLines() = lines;
        doc->setModified(true);
        document_manager_.switchToDocument(document_manager_.getDocumentCount() - 1);
        cursor_row_ = 0;
        cursor_col_ = 0;
        view_offset_row_ = 0;
        view_offset_col_ = 0;
        syntax_highlighter_.setFileType(getFileType());
        document_ssh_configs_[doc_index] = config;

        std::string parent = parentRemotePath(path);
        file_browser_.openDirectory(buildSSHUri(config, parent));
        file_browser_.setVisible(true);
        terminal_.startSSHSession(config.host, config.user, config.port, config.key_path,
                                  config.password);
        setStatusMessage("SSH: " + config.user + "@" + config.host + " | Del: disconnect");
        return;
    }

    setStatusMessage("SSH: Path not found or not accessible: " + path);
    file_browser_.clearRemoteLoader();
    file_browser_.clearRemoteFileOpExecutor();
}

void Editor::disconnectSSH() {
    if (current_ssh_config_.host.empty())
        return;
    current_ssh_config_ = pnana::ui::SSHConfig{};
    ssh_remote_home_.clear();
    document_ssh_configs_.clear();
    file_browser_.clearRemoteLoader();
    file_browser_.clearRemoteFileOpExecutor();
    tui_config_manager_.clearRemoteContext();
    pnana::features::package_manager::PackageManagerRegistry::getInstance()
        .clearRemoteContextForAll();
    git_panel_.clearRemoteContext(last_local_directory_.empty() ? "." : last_local_directory_);
    file_picker_.clearRemoteLoader();
    file_browser_.openDirectory(last_local_directory_.empty() ? "." : last_local_directory_);
    terminal_.restoreLocalShell();
    setStatusMessage("SSH disconnected");
}

void Editor::showSSHTransferDialog() {
    if (current_ssh_config_.host.empty()) {
        setStatusMessage("SSH: No active SSH connection. Please connect first.");
        return;
    }

    ssh_transfer_dialog_.show(
        [this](const std::vector<pnana::ui::SSHTransferItem>& items) {
            handleSSHFileTransfer(items);
        },
        [this]() {
            handleSSHTransferCancel();
        });
}

namespace {

// 取路径最后一段作为文件名（不含目录）
static std::string basename(const std::string& path) {
    if (path.empty())
        return "";
    std::string p = path;
    while (!p.empty() && (p.back() == '/' || p.back() == '\\'))
        p.pop_back();
    size_t pos = p.find_last_of("/\\");
    if (pos == std::string::npos)
        return p;
    return p.substr(pos + 1);
}

// 若目标路径为目录（以 / 结尾、为 "."/".."、或像目录的路径），则返回 目录 + 源文件原名
static std::string resolveDestinationPath(const std::string& dest_path,
                                          const std::string& source_path) {
    if (dest_path.empty())
        return dest_path;
    if (dest_path.back() == '/' || dest_path.back() == '\\')
        return dest_path + basename(source_path);
    std::string d = dest_path;
    while (!d.empty() && (d.back() == '/' || d.back() == '\\'))
        d.pop_back();
    if (d.empty() || d == "." || d == "..")
        return (d.empty() ? "." : dest_path) + (dest_path.back() == '/' ? "" : "/") +
               basename(source_path);
    // 路径像目录（含 / 且最后一段无扩展名，如 /home/frames、/tmp）时，按目录处理，避免写入目录导致
    // Process exited with status 1
    size_t last_slash = d.find_last_of("/\\");
    std::string last_component = (last_slash == std::string::npos) ? d : d.substr(last_slash + 1);
    if (last_slash != std::string::npos && last_component.find('.') == std::string::npos)
        return dest_path + (dest_path.back() == '/' ? "" : "/") + basename(source_path);
    return dest_path;
}

} // namespace

void Editor::handleSSHFileTransfer(const std::vector<pnana::ui::SSHTransferItem>& items) {
    if (current_ssh_config_.host.empty()) {
        setStatusMessage("SSH: No active SSH connection");
        return;
    }

    setStatusMessage("SSH: Starting file transfer...");

    if (!items.empty()) {
        const auto& item = items[0];
        std::string local_path = item.local_path;
        std::string remote_path = item.remote_path;

        // 若用户输入的是文件夹路径（以 / 结尾或像目录），则以原名保存到该目录
        if (item.direction == "upload") {
            remote_path = resolveDestinationPath(remote_path, local_path);
        } else {
            local_path = resolveDestinationPath(local_path, remote_path);
        }

        features::ssh::Client ssh_client;

        if (item.direction == "upload") {
            auto result = ssh_client.uploadFile(current_ssh_config_, local_path, remote_path);
            if (result.success) {
                setStatusMessage("SSH: File uploaded successfully: " + local_path);
            } else {
                setStatusMessage("SSH: Upload failed: " + result.error);
            }
        } else if (item.direction == "download") {
            auto result = ssh_client.downloadFile(current_ssh_config_, remote_path, local_path);
            if (result.success) {
                setStatusMessage("SSH: File downloaded successfully: " + local_path);
            } else {
                setStatusMessage("SSH: Download failed: " + result.error);
            }
        }
    }
}

void Editor::handleSSHTransferCancel() {
    setStatusMessage("SSH: File transfer cancelled");
}

bool Editor::saveSSHFile(Document* doc, const pnana::ui::SSHConfig& config,
                         const std::string& filepath) {
    if (!doc) {
        return false;
    }

    // 获取文档内容
    std::string content = doc->getContent();

    // 确定远程路径
    std::string remote_path = config.remote_path;
    if (!filepath.empty()) {
        // 如果提供了新路径，解析它
        pnana::ui::SSHConfig new_config;
        if (parseSSHPath(filepath, new_config)) {
            remote_path = new_config.remote_path;
        }
    }

    setStatusMessage("SSH: Saving to " + config.host + ":" + remote_path + "...");

    // 创建SSH配置用于写回
    pnana::ui::SSHConfig write_config = config;
    write_config.remote_path = remote_path;

    // 创建SSH客户端并写入文件
    features::ssh::Client ssh_client;
    features::ssh::Result result = ssh_client.writeFile(write_config, content);

    if (!result.success) {
        setStatusMessage("SSH Error: " + result.error);
        return false;
    }

    // 更新文档路径（如果是新路径）
    if (!filepath.empty()) {
        // 更新文档路径和SSH配置映射
        doc->setFilePath(filepath);
        size_t doc_index = document_manager_.getCurrentIndex();
        document_ssh_configs_[doc_index] = write_config;
    }

    // 标记为已保存
    doc->setModified(false);

    size_t line_count = doc->lineCount();
    size_t byte_count = content.length();
    std::string msg = std::string(pnana::ui::icons::SAVED) + " Wrote " +
                      std::to_string(line_count) + " lines (" + std::to_string(byte_count) +
                      " bytes) to " + config.host + ":" + remote_path;
    setStatusMessage(msg);
    return true;
}

// 与 FzfPopup 中忽略目录一致
static const std::vector<std::string> kFzfIgnoreDirs = {
    ".git",   "node_modules", "__pycache__", ".svn",    ".hg",  "build", "dist",
    "target", ".cache",       ".idea",       ".vscode", "venv", ".venv", "vendor"};

static bool fzfPathShouldIgnore(const std::string& rel_path) {
    for (const auto& ignore : kFzfIgnoreDirs) {
        std::string with_slash = "/" + ignore + "/";
        if (rel_path.find(with_slash) != std::string::npos)
            return true;
        if (rel_path.size() > ignore.size() + 1 &&
            rel_path.compare(rel_path.size() - ignore.size() - 1, ignore.size() + 1,
                             "/" + ignore) == 0)
            return true;
        if (rel_path.size() >= ignore.size() + 1 &&
            rel_path.compare(0, ignore.size() + 1, ignore + "/") == 0)
            return true;
    }
    return false;
}

void Editor::onFzfRemoteLoad(const std::string& ssh_uri) {
    pnana::ui::SSHConfig config;
    if (!parseSSHPath(ssh_uri, config)) {
        std::string uri_copy = ssh_uri;
        screen_.Post([this, uri_copy]() {
            fzf_popup_.receiveFiles({}, {}, uri_copy);
            force_ui_update_ = true;
            screen_.PostEvent(ftxui::Event::Custom);
        });
        return;
    }
    std::string uri_copy = ssh_uri;
    std::thread([this, config, uri_copy]() {
        features::ssh::Client client;
        std::string cwd = config.remote_path.empty() ? "." : config.remote_path;
        features::ssh::Result result = client.runCommand(config, cwd, "find . -type f 2>/dev/null");
        std::vector<std::string> files;
        std::vector<std::string> display_paths;
        if (result.success && !result.content.empty()) {
            std::string base_uri = uri_copy;
            if (base_uri.size() > 1 && base_uri.back() == '/')
                base_uri.pop_back();
            std::istringstream iss(result.content);
            std::string line;
            while (std::getline(iss, line)) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                    line.pop_back();
                if (line.empty())
                    continue;
                if (line.size() >= 2 && line.compare(0, 2, "./") == 0)
                    line = line.substr(2);
                if (line.empty())
                    continue;
                if (fzfPathShouldIgnore(line))
                    continue;
                std::string full = base_uri + "/" + line;
                files.push_back(full);
                display_paths.push_back(line);
            }
        }
        screen_.Post([this, files = std::move(files), display_paths = std::move(display_paths),
                      root = uri_copy]() mutable {
            fzf_popup_.receiveFiles(std::move(files), std::move(display_paths), std::move(root));
            force_ui_update_ = true;
            screen_.PostEvent(ftxui::Event::Custom);
        });
    }).detach();
}

} // namespace core
} // namespace pnana
