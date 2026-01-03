#include "features/split_view.h"
#include <algorithm>
#include <climits>
#include <cmath>

namespace pnana {
namespace features {

SplitViewManager::SplitViewManager() : active_region_index_(0) {
    // 初始化时创建一个默认区域
    regions_.emplace_back(0, 0, 0, 0, 0);
    regions_[0].is_active = true;
}

void SplitViewManager::splitVertical(int screen_width, int screen_height) {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* active = getActiveRegion();
    if (!active) {
        return;
    }

    // 如果这是第一次分屏，需要先设置当前区域的尺寸
    if (active->width == 0 || active->height == 0) {
        active->x = 0;
        active->y = 0;
        active->width = screen_width;
        active->height = screen_height;
    }

    // 计算新区域的尺寸
    int new_width = active->width / 2;
    int remaining_width = active->width - new_width;

    // 调整当前区域
    active->width = new_width;

    // 创建新区域（在右侧）
    size_t new_doc_index = active->document_index; // 新区域显示相同的文档
    ViewRegion new_region(active->x + new_width, active->y, remaining_width, active->height,
                          new_doc_index);
    new_region.is_active = true;
    active->is_active = false;

    regions_.push_back(new_region);
    active_region_index_ = regions_.size() - 1;

    // 创建分屏线
    SplitLine split_line(true, active->x + new_width, active->y, active->y + active->height);
    split_lines_.push_back(split_line);
}

void SplitViewManager::splitHorizontal(int screen_width, int screen_height) {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* active = getActiveRegion();
    if (!active) {
        return;
    }

    // 如果这是第一次分屏，需要先设置当前区域的尺寸
    if (active->width == 0 || active->height == 0) {
        active->x = 0;
        active->y = 0;
        active->width = screen_width;
        active->height = screen_height;
    }

    // 计算新区域的尺寸
    int new_height = active->height / 2;
    int remaining_height = active->height - new_height;

    // 调整当前区域
    active->height = new_height;

    // 创建新区域（在下侧）
    size_t new_doc_index = active->document_index; // 新区域显示相同的文档
    ViewRegion new_region(active->x, active->y + new_height, active->width, remaining_height,
                          new_doc_index);
    new_region.is_active = true;
    active->is_active = false;

    regions_.push_back(new_region);
    active_region_index_ = regions_.size() - 1;

    // 创建分屏线
    SplitLine split_line(false, active->y + new_height, active->x, active->x + active->width);
    split_lines_.push_back(split_line);
}

void SplitViewManager::closeCurrentRegion() {
    if (regions_.size() <= 1) {
        return; // 不能关闭最后一个区域
    }
    closeRegion(active_region_index_);
}

void SplitViewManager::closeRegion(size_t region_index) {
    if (regions_.size() <= 1) {
        return; // 不能关闭最后一个区域
    }

    if (region_index >= regions_.size()) {
        return; // 无效的索引
    }

    // 移除指定区域
    regions_.erase(regions_.begin() + region_index);

    // 移除相关的分屏线
    // 简化处理：移除所有分屏线，稍后可以优化
    split_lines_.clear();

    // 调整激活区域索引
    if (active_region_index_ >= regions_.size()) {
        active_region_index_ = regions_.size() - 1;
    }
    if (active_region_index_ >= region_index && active_region_index_ > 0) {
        active_region_index_--;
    }

    // 激活第一个区域（如果还有区域）
    if (!regions_.empty()) {
        regions_[active_region_index_].is_active = true;
        // 更新其他区域的激活状态
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (i != active_region_index_) {
                regions_[i].is_active = false;
            }
        }
    }
}

void SplitViewManager::closeAllSplits() {
    reset();
}

void SplitViewManager::focusNextRegion() {
    if (regions_.empty()) {
        return;
    }

    regions_[active_region_index_].is_active = false;
    active_region_index_ = (active_region_index_ + 1) % regions_.size();
    regions_[active_region_index_].is_active = true;
}

void SplitViewManager::focusPreviousRegion() {
    if (regions_.empty()) {
        return;
    }

    regions_[active_region_index_].is_active = false;
    if (active_region_index_ == 0) {
        active_region_index_ = regions_.size() - 1;
    } else {
        active_region_index_--;
    }
    regions_[active_region_index_].is_active = true;
}

void SplitViewManager::focusLeftRegion() {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* current = getActiveRegion();
    if (!current) {
        return;
    }

    // 找到左侧最近的区域
    ViewRegion* leftmost = nullptr;
    int min_distance = INT_MAX;

    for (auto& region : regions_) {
        if (&region == current) {
            continue;
        }

        // 检查是否在左侧（y 范围重叠，x 更小）
        if (region.y < current->y + current->height && region.y + region.height > current->y &&
            region.x + region.width <= current->x) {
            int distance = current->x - (region.x + region.width);
            if (distance < min_distance) {
                min_distance = distance;
                leftmost = &region;
            }
        }
    }

    if (leftmost) {
        current->is_active = false;
        leftmost->is_active = true;
        // 找到 leftmost 在 regions_ 中的索引
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (&regions_[i] == leftmost) {
                active_region_index_ = i;
                break;
            }
        }
    }
}

void SplitViewManager::focusRightRegion() {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* current = getActiveRegion();
    if (!current) {
        return;
    }

    // 找到右侧最近的区域
    ViewRegion* rightmost = nullptr;
    int min_distance = INT_MAX;

    for (auto& region : regions_) {
        if (&region == current) {
            continue;
        }

        // 检查是否在右侧（y 范围重叠，x 更大）
        if (region.y < current->y + current->height && region.y + region.height > current->y &&
            region.x >= current->x + current->width) {
            int distance = region.x - (current->x + current->width);
            if (distance < min_distance) {
                min_distance = distance;
                rightmost = &region;
            }
        }
    }

    if (rightmost) {
        current->is_active = false;
        rightmost->is_active = true;
        // 找到 rightmost 在 regions_ 中的索引
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (&regions_[i] == rightmost) {
                active_region_index_ = i;
                break;
            }
        }
    }
}

void SplitViewManager::focusUpRegion() {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* current = getActiveRegion();
    if (!current) {
        return;
    }

    // 找到上方最近的区域
    ViewRegion* uppermost = nullptr;
    int min_distance = INT_MAX;

    for (auto& region : regions_) {
        if (&region == current) {
            continue;
        }

        // 检查是否在上方（x 范围重叠，y 更小）
        if (region.x < current->x + current->width && region.x + region.width > current->x &&
            region.y + region.height <= current->y) {
            int distance = current->y - (region.y + region.height);
            if (distance < min_distance) {
                min_distance = distance;
                uppermost = &region;
            }
        }
    }

    if (uppermost) {
        current->is_active = false;
        uppermost->is_active = true;
        // 找到 uppermost 在 regions_ 中的索引
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (&regions_[i] == uppermost) {
                active_region_index_ = i;
                break;
            }
        }
    }
}

void SplitViewManager::focusDownRegion() {
    if (regions_.empty()) {
        return;
    }

    ViewRegion* current = getActiveRegion();
    if (!current) {
        return;
    }

    // 找到下方最近的区域
    ViewRegion* downmost = nullptr;
    int min_distance = INT_MAX;

    for (auto& region : regions_) {
        if (&region == current) {
            continue;
        }

        // 检查是否在下方（x 范围重叠，y 更大）
        if (region.x < current->x + current->width && region.x + region.width > current->x &&
            region.y >= current->y + current->height) {
            int distance = region.y - (current->y + current->height);
            if (distance < min_distance) {
                min_distance = distance;
                downmost = &region;
            }
        }
    }

    if (downmost) {
        current->is_active = false;
        downmost->is_active = true;
        // 找到 downmost 在 regions_ 中的索引
        for (size_t i = 0; i < regions_.size(); ++i) {
            if (&regions_[i] == downmost) {
                active_region_index_ = i;
                break;
            }
        }
    }
}

ViewRegion* SplitViewManager::getActiveRegion() {
    if (active_region_index_ < regions_.size()) {
        return &regions_[active_region_index_];
    }
    return nullptr;
}

const ViewRegion* SplitViewManager::getActiveRegion() const {
    if (active_region_index_ < regions_.size()) {
        return &regions_[active_region_index_];
    }
    return nullptr;
}

bool SplitViewManager::handleMouseEvent(ftxui::Event& event, int screen_width, int screen_height,
                                        int x_offset, int y_offset) {
    if (!event.is_mouse()) {
        return false;
    }

    auto& mouse = event.mouse();
    // 调整鼠标坐标，减去UI元素的偏移
    int x = mouse.x - x_offset;
    int y = mouse.y - y_offset;

    // 检查是否点击在分屏线上
    for (auto& line : split_lines_) {
        if (isPointOnSplitLine(x, y, line)) {
            if (mouse.button == ftxui::Mouse::Button::Left &&
                mouse.motion == ftxui::Mouse::Motion::Pressed) {
                line.is_dragging = true;
                return true;
            }
        }
    }

    // 处理拖动
    for (auto& line : split_lines_) {
        if (line.is_dragging) {
            if (mouse.motion == ftxui::Mouse::Motion::Released) {
                line.is_dragging = false;
                return true;
            } else if (mouse.motion == ftxui::Mouse::Motion::Moved) {
                int new_position = line.is_vertical ? x : y;
                adjustSplitLine(line, new_position, screen_width, screen_height);
                return true;
            }
        }
    }

    return false;
}

ftxui::Element SplitViewManager::renderRegions(
    std::function<ftxui::Element(const ViewRegion&)> render_func, int screen_width,
    int screen_height) {
    using namespace ftxui;
    Elements elements;

    // 更新区域尺寸
    updateRegionSizes(screen_width, screen_height);

    // 渲染每个区域
    for (const auto& region : regions_) {
        Element region_elem = render_func(region);
        region_elem =
            region_elem | size(WIDTH, EQUAL, region.width) | size(HEIGHT, EQUAL, region.height);
        elements.push_back(region_elem);
    }

    // 渲染分屏线
    for (const auto& line : split_lines_) {
        if (line.is_vertical) {
            // 竖直分屏线
            Elements line_elements;
            for (int i = line.start_pos; i < line.end_pos; ++i) {
                line_elements.push_back(text("│") | color(Color::GrayDark));
            }
            Element line_elem = vbox(line_elements) | size(WIDTH, EQUAL, 1);
            elements.push_back(line_elem);
        } else {
            // 横向分屏线
            Elements line_elements;
            for (int i = line.start_pos; i < line.end_pos; ++i) {
                line_elements.push_back(text("─"));
            }
            Element line_elem =
                hbox(line_elements) | color(Color::GrayDark) | size(HEIGHT, EQUAL, 1);
            elements.push_back(line_elem);
        }
    }

    return vbox(elements);
}

void SplitViewManager::reset() {
    regions_.clear();
    split_lines_.clear();
    regions_.emplace_back(0, 0, 0, 0, 0);
    regions_[0].is_active = true;
    active_region_index_ = 0;
}

void SplitViewManager::setCurrentDocumentIndex(size_t index) {
    ViewRegion* active = getActiveRegion();
    if (active) {
        active->document_index = index;
    }
}

void SplitViewManager::updateRegionSizes(int screen_width, int screen_height) {
    if (regions_.empty()) {
        return;
    }

    if (regions_.size() == 1) {
        // 单视图，占满整个屏幕
        regions_[0].x = 0;
        regions_[0].y = 0;
        regions_[0].width = screen_width;
        regions_[0].height = screen_height;
        return;
    }

    // 多视图：需要根据分屏线重新计算区域大小
    // 简化实现：这里可以根据分屏线的位置重新分配区域
    // 为了简化，我们假设区域已经正确设置
}

ViewRegion* SplitViewManager::findRegionAt(int x, int y) {
    for (auto& region : regions_) {
        if (x >= region.x && x < region.x + region.width && y >= region.y &&
            y < region.y + region.height) {
            return &region;
        }
    }
    return nullptr;
}

SplitLine* SplitViewManager::findSplitLineAt(int x, int y) {
    for (auto& line : split_lines_) {
        if (isPointOnSplitLine(x, y, line)) {
            return &line;
        }
    }
    return nullptr;
}

void SplitViewManager::adjustSplitLine(SplitLine& line, int new_position, int screen_width,
                                       int screen_height) {
    // 限制新位置在有效范围内
    int old_position = line.position;
    if (line.is_vertical) {
        new_position = std::max(10, std::min(new_position, screen_width - 10));
    } else {
        new_position = std::max(5, std::min(new_position, screen_height - 5));
    }

    if (new_position == old_position) {
        return; // 位置没有变化
    }

    int delta = new_position - old_position;
    line.position = new_position;

    // 更新相关区域的尺寸
    if (line.is_vertical) {
        // 竖直分屏线：调整左右区域的宽度
        ViewRegion* left_region = nullptr;
        ViewRegion* right_region = nullptr;

        // 找到与分屏线相邻的区域
        // 左侧区域：右边界在分屏线位置（允许1像素误差）
        // 右侧区域：左边界在分屏线位置（允许1像素误差）
        for (auto& region : regions_) {
            // 检查是否在分屏线左侧（区域的右边界接近分屏线）
            if (std::abs((region.x + region.width) - old_position) <= 2) {
                left_region = &region;
            }
            // 检查是否在分屏线右侧（区域的左边界接近分屏线）
            if (std::abs(region.x - (old_position + 1)) <= 2) {
                right_region = &region;
            }
        }

        if (left_region && right_region) {
            // 调整左侧区域宽度
            left_region->width += delta;
            // 调整右侧区域位置和宽度
            right_region->x += delta;
            right_region->width -= delta;

            // 确保区域尺寸有效（最小宽度10）
            if (left_region->width < 10) {
                int adjust = 10 - left_region->width;
                left_region->width = 10;
                right_region->x += adjust;
                right_region->width -= adjust;
                line.position = left_region->x + left_region->width;
            }
            if (right_region->width < 10) {
                int adjust = 10 - right_region->width;
                right_region->width = 10;
                left_region->width -= adjust;
                line.position = right_region->x - 1;
            }
        }
    } else {
        // 横向分屏线：调整上下区域的高度
        ViewRegion* top_region = nullptr;
        ViewRegion* bottom_region = nullptr;

        // 找到与分屏线相邻的区域
        for (auto& region : regions_) {
            // 检查是否在分屏线上方（区域的下边界接近分屏线）
            if (std::abs((region.y + region.height) - old_position) <= 2) {
                top_region = &region;
            }
            // 检查是否在分屏线下方（区域的上边界接近分屏线）
            if (std::abs(region.y - (old_position + 1)) <= 2) {
                bottom_region = &region;
            }
        }

        if (top_region && bottom_region) {
            // 调整上方区域高度
            top_region->height += delta;
            // 调整下方区域位置和高度
            bottom_region->y += delta;
            bottom_region->height -= delta;

            // 确保区域尺寸有效（最小高度5）
            if (top_region->height < 5) {
                int adjust = 5 - top_region->height;
                top_region->height = 5;
                bottom_region->y += adjust;
                bottom_region->height -= adjust;
                line.position = top_region->y + top_region->height;
            }
            if (bottom_region->height < 5) {
                int adjust = 5 - bottom_region->height;
                bottom_region->height = 5;
                top_region->height -= adjust;
                line.position = bottom_region->y - 1;
            }
        }
    }

    // 更新分屏线的范围
    if (line.is_vertical) {
        // 更新竖直分屏线的垂直范围
        int min_y = INT_MAX, max_y = 0;
        for (const auto& region : regions_) {
            // 找到与分屏线相交的区域
            if (region.x < line.position && region.x + region.width > line.position) {
                min_y = std::min(min_y, region.y);
                max_y = std::max(max_y, region.y + region.height);
            }
        }
        if (min_y < INT_MAX) {
            line.start_pos = min_y;
            line.end_pos = max_y;
        }
    } else {
        // 更新横向分屏线的水平范围
        int min_x = INT_MAX, max_x = 0;
        for (const auto& region : regions_) {
            // 找到与分屏线相交的区域
            if (region.y < line.position && region.y + region.height > line.position) {
                min_x = std::min(min_x, region.x);
                max_x = std::max(max_x, region.x + region.width);
            }
        }
        if (min_x < INT_MAX) {
            line.start_pos = min_x;
            line.end_pos = max_x;
        }
    }
}

bool SplitViewManager::isPointOnSplitLine(int x, int y, const SplitLine& line) const {
    if (line.is_vertical) {
        // 竖直分屏线：检查 x 是否在线上，y 是否在线范围内
        return std::abs(x - line.position) <= 1 && y >= line.start_pos && y < line.end_pos;
    } else {
        // 横向分屏线：检查 y 是否在线上，x 是否在线范围内
        return std::abs(y - line.position) <= 1 && x >= line.start_pos && x < line.end_pos;
    }
}

} // namespace features
} // namespace pnana
