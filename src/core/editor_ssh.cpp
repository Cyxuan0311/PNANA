#include "core/editor.h"
#include "features/ssh/ssh_client.h"
#include <sstream>

namespace pnana {
namespace core {

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

    // 设置文件路径
    // 注意：Document 类可能需要修改以支持 SSH 文件
    // 暂时使用文件路径来标识

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

    setStatusMessage("SSH: Connected and loaded " + config.remote_path);
}

} // namespace core
} // namespace pnana
