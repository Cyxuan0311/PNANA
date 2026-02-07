#include "ui/package_detail_dialog.h"
#include "ui/icons.h"
#include <algorithm>
#include <chrono>
#include <exception>
#include <thread>

using namespace ftxui;

namespace pnana {
namespace ui {

// Custom border decorator with theme color
static inline Decorator borderWithColor(Color border_color) {
    return [=](Element child) -> Element {
        return child | border | ftxui::color(border_color);
    };
}

PackageDetailDialog::PackageDetailDialog(Theme& theme)
    : theme_(theme), visible_(false), operation_in_progress_(false), operation_success_(false) {}

void PackageDetailDialog::show(
    const features::package_manager::Package& package,
    std::shared_ptr<features::package_manager::PackageManagerBase> manager) {
    package_ = package;
    manager_ = manager;
    visible_ = true;
    std::lock_guard<std::mutex> lock(operation_mutex_);
    operation_status_.clear();
    operation_in_progress_ = false;
    operation_success_ = false;
}

void PackageDetailDialog::hide() {
    visible_ = false;
}

bool PackageDetailDialog::handleInput(Event event) {
    if (!visible_) {
        return false;
    }

    if (event == Event::Escape) {
        hide();
        return true;
    }

    // 如果操作正在进行中，忽略其他输入
    {
        std::lock_guard<std::mutex> lock(operation_mutex_);
        if (operation_in_progress_) {
            return true;
        }
    }

    // 处理操作按键
    if (!manager_) {
        return true;
    }

    if (event == Event::Character('u')) {
        // 更新单个包
        {
            std::lock_guard<std::mutex> lock(operation_mutex_);
            operation_in_progress_ = true;
            operation_status_ = "Updating package: " + package_.name + "...";
        }

        std::thread([this]() {
            bool success = manager_->updatePackage(package_.name);

            if (!success) {
                // 如果启动失败，立即更新状态
                {
                    std::lock_guard<std::mutex> lock(operation_mutex_);
                    operation_in_progress_ = false;
                    operation_success_ = false;
                    operation_status_ = "Failed to start update: " + package_.name;
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
                return;
            }

            // 命令已启动，等待命令完成
            const int wait_seconds = 5;
            const int check_interval_ms = 500;
            int waited_milliseconds = 0;

            while (waited_milliseconds < wait_seconds * 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                waited_milliseconds += check_interval_ms;

                // 检查是否有错误（如果命令失败，错误信息会被设置）
                if (manager_->hasError()) {
                    std::string error = manager_->getError();
                    if (error.find("Failed") != std::string::npos ||
                        error.find("not found") != std::string::npos) {
                        break;
                    }
                }
            }

            // 检查最终结果
            bool has_error = manager_->hasError();
            std::string error_msg = has_error ? manager_->getError() : "";
            bool final_success = !has_error || error_msg.empty();

            try {
                std::lock_guard<std::mutex> lock(operation_mutex_);
                operation_in_progress_ = false;
                operation_success_ = final_success;
                if (final_success) {
                    operation_status_ = "Update completed for: " + package_.name;
                } else {
                    operation_status_ = "Failed to update: " + package_.name;
                    if (has_error) {
                        operation_status_ += " - " + error_msg;
                    }
                }
            } catch (const std::exception& e) {
                // 忽略异常
            }

            // 清除缓存，强制刷新
            try {
                manager_->clearCache();
            } catch (const std::exception& e) {
                // 忽略异常
            }
        }).detach();
        return true;
    } else if (event == Event::Character('U')) {
        // 链式更新所有依赖
        {
            std::lock_guard<std::mutex> lock(operation_mutex_);
            operation_in_progress_ = true;
            operation_status_ = "Updating package and all dependencies: " + package_.name + "...";
        }

        std::thread([this]() {
            bool success = manager_->updateAllDependencies(package_.name);

            if (!success) {
                {
                    std::lock_guard<std::mutex> lock(operation_mutex_);
                    operation_in_progress_ = false;
                    operation_success_ = false;
                    operation_status_ =
                        "Failed to start update: " + package_.name + " and dependencies";
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
                return;
            }

            const int wait_seconds = 10;
            const int check_interval_ms = 500;
            int waited_milliseconds = 0;

            while (waited_milliseconds < wait_seconds * 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                waited_milliseconds += check_interval_ms;

                if (manager_->hasError()) {
                    std::string error = manager_->getError();
                    if (error.find("Failed") != std::string::npos ||
                        error.find("not found") != std::string::npos) {
                        break;
                    }
                }
            }

            bool final_success = !manager_->hasError() || manager_->getError().empty();

            {
                std::lock_guard<std::mutex> lock(operation_mutex_);
                operation_in_progress_ = false;
                operation_success_ = final_success;
                if (final_success) {
                    operation_status_ =
                        "Update completed for: " + package_.name + " and dependencies";
                } else {
                    operation_status_ = "Failed to update: " + package_.name + " and dependencies";
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
            }
            manager_->clearCache();
        }).detach();
        return true;
    } else if (event == Event::Delete) {
        // 删除包
        {
            std::lock_guard<std::mutex> lock(operation_mutex_);
            operation_in_progress_ = true;
            operation_status_ = "Removing package: " + package_.name + "...";
        }

        std::thread([this]() {
            bool success = manager_->removePackage(package_.name);

            if (!success) {
                {
                    std::lock_guard<std::mutex> lock(operation_mutex_);
                    operation_in_progress_ = false;
                    operation_success_ = false;
                    operation_status_ = "Failed to start removal: " + package_.name;
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
                return;
            }

            const int wait_seconds = 5;
            const int check_interval_ms = 500;
            int waited_milliseconds = 0;

            while (waited_milliseconds < wait_seconds * 1000) {
                std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
                waited_milliseconds += check_interval_ms;

                if (manager_->hasError()) {
                    std::string error = manager_->getError();
                    if (error.find("Failed") != std::string::npos ||
                        error.find("not found") != std::string::npos) {
                        break;
                    }
                }
            }

            bool final_success = !manager_->hasError() || manager_->getError().empty();

            {
                std::lock_guard<std::mutex> lock(operation_mutex_);
                operation_in_progress_ = false;
                operation_success_ = final_success;
                if (final_success) {
                    operation_status_ = "Removal completed for: " + package_.name;
                } else {
                    operation_status_ = "Failed to remove: " + package_.name;
                    if (manager_->hasError()) {
                        operation_status_ += " - " + manager_->getError();
                    }
                }
            }
            manager_->clearCache();
        }).detach();
        return true;
    }

    // 其他按键都被忽略
    return true;
}

Element PackageDetailDialog::render() const {
    if (!visible_) {
        return text("");
    }

    auto& colors = theme_.getColors();
    Elements content;

    // 标题
    Elements header_elements;
    header_elements.push_back(text("  ") | color(colors.menubar_fg));
    header_elements.push_back(text(icons::PACKAGE) | color(colors.success));
    header_elements.push_back(text(" Package Details ") | color(colors.menubar_fg) | bold);
    content.push_back(hbox(std::move(header_elements)) | bgcolor(colors.menubar_bg));
    content.push_back(separator());

    // 包信息
    content.push_back(renderPackageInfo());

    // 操作状态
    // 注意：renderOperationStatus() 内部会获取 mutex，所以这里先检查状态，然后释放 mutex 再调用
    bool has_status = false;
    {
        std::lock_guard<std::mutex> lock(operation_mutex_);
        has_status = !operation_status_.empty();
    }
    if (has_status) {
        content.push_back(separator());
        content.push_back(renderOperationStatus());
    }

    content.push_back(separator());

    // 提示信息
    Elements hint_elements;
    hint_elements.push_back(text("  ") | color(colors.comment));
    hint_elements.push_back(text("Press ") | color(colors.comment) | dim);
    hint_elements.push_back(text("u") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Update  ") | color(colors.comment) | dim);
    hint_elements.push_back(text("U") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Update All  ") | color(colors.comment) | dim);
    hint_elements.push_back(text("Delete") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Remove  ") | color(colors.comment) | dim);
    hint_elements.push_back(text("Esc") | color(colors.helpbar_key) | bold);
    hint_elements.push_back(text(": Return") | color(colors.comment) | dim);
    content.push_back(hbox(std::move(hint_elements)));

    return window(text(" Package Details ") | color(colors.success) | bold,
                  vbox(std::move(content))) |
           size(WIDTH, GREATER_THAN, 80) | size(HEIGHT, GREATER_THAN, 15) |
           bgcolor(colors.background) | borderWithColor(colors.dialog_border) | center;
}

Element PackageDetailDialog::renderPackageInfo() const {
    auto& colors = theme_.getColors();
    Elements content;

    Elements info_elements;

    // 包名
    info_elements.push_back(text("  Name: ") | color(colors.keyword) | bold);
    info_elements.push_back(text(package_.name) | color(colors.foreground));
    info_elements.push_back(text(""));
    content.push_back(hbox(std::move(info_elements)));

    info_elements.clear();
    info_elements.push_back(text("  Version: ") | color(colors.keyword) | bold);
    info_elements.push_back(text(package_.version) | color(colors.foreground));
    info_elements.push_back(text(""));
    content.push_back(hbox(std::move(info_elements)));

    // 位置/来源
    if (!package_.location.empty()) {
        info_elements.clear();
        info_elements.push_back(text("  Location: ") | color(colors.keyword) | bold);
        info_elements.push_back(text(package_.location) | color(colors.comment));
        info_elements.push_back(text(""));
        content.push_back(hbox(std::move(info_elements)));
    }

    // 状态
    if (!package_.status.empty()) {
        info_elements.clear();
        info_elements.push_back(text("  Status: ") | color(colors.keyword) | bold);
        info_elements.push_back(text(package_.status) | color(colors.comment));
        info_elements.push_back(text(""));
        content.push_back(hbox(std::move(info_elements)));
    }

    // 描述
    if (!package_.description.empty()) {
        content.push_back(separator());
        info_elements.clear();
        info_elements.push_back(text("  Description: ") | color(colors.keyword) | bold);
        info_elements.push_back(text(""));
        content.push_back(hbox(std::move(info_elements)));

        // 描述可能很长，需要换行显示
        std::string desc = package_.description;
        const size_t max_width = 70;
        if (desc.length() > max_width) {
            // 简单的文本换行
            std::string wrapped_desc;
            size_t pos = 0;
            while (pos < desc.length()) {
                size_t end_pos = std::min(pos + max_width, desc.length());
                if (end_pos < desc.length()) {
                    // 尝试在空格处断开
                    size_t space_pos = desc.find_last_of(" \t", end_pos);
                    if (space_pos != std::string::npos && space_pos > pos) {
                        end_pos = space_pos;
                    }
                }
                wrapped_desc += desc.substr(pos, end_pos - pos);
                if (end_pos < desc.length()) {
                    wrapped_desc += "\n  ";
                }
                pos = (end_pos < desc.length() && desc[end_pos] == ' ') ? end_pos + 1 : end_pos;
            }
            content.push_back(text("  " + wrapped_desc) | color(colors.foreground));
        } else {
            content.push_back(text("  " + desc) | color(colors.foreground));
        }
    }

    return vbox(std::move(content));
}

Element PackageDetailDialog::renderOperationStatus() const {
    auto& colors = theme_.getColors();
    Elements status_elements;

    std::lock_guard<std::mutex> lock(operation_mutex_);

    // 根据操作状态选择颜色和图标
    Color status_color;
    std::string status_icon;

    if (operation_in_progress_) {
        // 进行中：黄色警告
        status_color = colors.warning;
        status_icon = icons::REFRESH;
    } else if (operation_success_) {
        // 成功完成：绿色成功
        status_color = colors.success;
        status_icon = icons::CHECK_CIRCLE;
    } else {
        // 失败：红色错误
        status_color = colors.error;
        status_icon = icons::ERROR;
    }

    std::string status_text = operation_status_;

    status_elements.push_back(text("  ") | color(colors.comment));
    status_elements.push_back(text(status_icon) | color(status_color));
    status_elements.push_back(text(" ") | color(colors.comment));
    status_elements.push_back(text(status_text) | color(status_color) |
                              (operation_in_progress_ ? bold : dim));

    return hbox(std::move(status_elements));
}

} // namespace ui
} // namespace pnana
