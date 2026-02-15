#include "core/editor.h"
#include "features/ssh/ssh_client.h"
#include <sstream>
#include <regex>

namespace pnana {
namespace core {

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
    ssh_dialog_.show(
        [this](const pnana::ui::SSHConfig& config) {
            handleSSHConnect(config);
        },
        [this]() {
            setStatusMessage("SSH connection cancelled");
        });
}

void Editor::handleSSHConnect(const pnana::ui::SSHConfig& config) {
    // 验证配置
    if (config.host.empty() || config.user.empty() || config.remote_path.empty()) {
        setStatusMessage("SSH: Missing required fields (host, user, or remote path)");
        return;
    }

    if (config.password.empty() && config.key_path.empty()) {
        setStatusMessage("SSH: Password or key path required");
        return;
    }

    setStatusMessage("SSH: Connecting to " + config.host + "...");

    // 创建 SSH 客户端并读取文件
    features::ssh::Client ssh_client;
    features::ssh::Result result = ssh_client.readFile(config);

    if (!result.success) {
        setStatusMessage("SSH Error: " + result.error);
        return;
    }

    // 创建临时本地文件用于编辑
    // 文件名格式: ssh://user@host:port/path
    std::ostringstream local_filename;
    local_filename << "ssh://" << config.user << "@" << config.host;
    if (config.port != 22) {
        local_filename << ":" << config.port;
    }
    local_filename << config.remote_path;

    // 创建新文档并加载内容
    size_t doc_index = document_manager_.createNewDocument();
    Document* doc = document_manager_.getDocument(doc_index);
    if (!doc) {
        setStatusMessage("SSH: Failed to create document");
        return;
    }

    // 设置文件路径（SSH路径格式: ssh://user@host:port/path）
    doc->setFilePath(local_filename.str());

    // 将内容按行分割并写入文档
    std::istringstream iss(result.content);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    // 如果内容为空，至少添加一个空行
    if (lines.empty()) {
        lines.push_back("");
    }

    // 直接设置行内容（通过 getLines() 获取可修改的引用）
    doc->getLines() = lines;

    // 标记为未保存的 SSH 文件
    doc->setModified(true);

    // 切换到新文档
    document_manager_.switchToDocument(document_manager_.getDocumentCount() - 1);
    cursor_row_ = 0;
    cursor_col_ = 0;
    view_offset_row_ = 0;
    view_offset_col_ = 0;

    // 设置语法高亮
    syntax_highlighter_.setFileType(getFileType());

    // 保存SSH连接信息（用于后续文件传输和保存）
    current_ssh_config_ = config;
    // 将SSH配置与文档关联
    document_ssh_configs_[doc_index] = config;

    setStatusMessage("SSH: Connected and loaded " + config.remote_path);
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

void Editor::handleSSHFileTransfer(const std::vector<pnana::ui::SSHTransferItem>& items) {
    if (current_ssh_config_.host.empty()) {
        setStatusMessage("SSH: No active SSH connection");
        return;
    }

    setStatusMessage("SSH: Starting file transfer...");

    // 这里应该启动异步文件传输
    // 暂时使用同步方式处理第一个项目作为示例
    if (!items.empty()) {
        const auto& item = items[0];
        features::ssh::Client ssh_client;

        if (item.direction == "upload") {
            auto result =
                ssh_client.uploadFile(current_ssh_config_, item.local_path, item.remote_path);
            if (result.success) {
                setStatusMessage("SSH: File uploaded successfully: " + item.local_path);
            } else {
                setStatusMessage("SSH: Upload failed: " + result.error);
            }
        } else if (item.direction == "download") {
            auto result =
                ssh_client.downloadFile(current_ssh_config_, item.remote_path, item.local_path);
            if (result.success) {
                setStatusMessage("SSH: File downloaded successfully: " + item.local_path);
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

} // namespace core
} // namespace pnana
