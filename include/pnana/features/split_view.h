#ifndef PNANA_FEATURES_SPLIT_VIEW_H
#define PNANA_FEATURES_SPLIT_VIEW_H

#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include <functional>
#include <memory>
#include <vector>

namespace pnana {
namespace features {

// 分屏方向
enum class SplitDirection {
    VERTICAL,  // 竖直分屏（左右）
    HORIZONTAL // 横向分屏（上下）
};

// 视图区域
struct ViewRegion {
    int x;                                // 起始列
    int y;                                // 起始行
    int width;                            // 宽度
    int height;                           // 高度
    bool is_active;                       // 是否激活
    size_t current_document_index;        // 该区域当前显示的文档索引
    std::vector<size_t> document_indices; // 该区域管理的文档索引列表

    // 默认构造函数，确保所有成员都被初始化
    ViewRegion()
        : x(0), y(0), width(0), height(0), is_active(false), current_document_index(0),
          document_indices() {}

    ViewRegion(int x_, int y_, int w, int h, size_t doc_idx)
        : x(x_), y(y_), width(w), height(h), is_active(false), current_document_index(doc_idx),
          document_indices({doc_idx}) {}
};

// 分屏线
struct SplitLine {
    bool is_vertical; // 是否为竖直分屏线
    int position;     // 位置（列或行）
    int start_pos;    // 起始位置（行或列）
    int end_pos;      // 结束位置（行或列）
    bool is_dragging; // 是否正在拖动

    SplitLine(bool vertical, int pos, int start, int end)
        : is_vertical(vertical), position(pos), start_pos(start), end_pos(end), is_dragging(false) {
    }
};

// 分屏视图管理器
class SplitViewManager {
  public:
    SplitViewManager();

    // 分屏操作
    void splitVertical(int screen_width, int screen_height);
    void splitHorizontal(int screen_width, int screen_height);
    void closeCurrentRegion();
    void closeRegion(size_t region_index); // 关闭指定区域
    void closeAllSplits();

    // 区域导航
    void focusNextRegion();
    void focusPreviousRegion();
    void focusLeftRegion();
    void focusRightRegion();
    void focusUpRegion();
    void focusDownRegion();

    // 获取当前激活的区域
    ViewRegion* getActiveRegion();
    const ViewRegion* getActiveRegion() const;

    // 获取所有区域
    const std::vector<ViewRegion>& getRegions() const {
        return regions_;
    }

    // 获取所有分屏线
    const std::vector<SplitLine>& getSplitLines() const {
        return split_lines_;
    }

    // 鼠标处理
    bool handleMouseEvent(ftxui::Event& event, int screen_width, int screen_height,
                          int x_offset = 0, int y_offset = 0);

    // 渲染
    ftxui::Element renderRegions(std::function<ftxui::Element(const ViewRegion&)> render_func,
                                 int screen_width, int screen_height);

    // 检查是否有分屏
    bool hasSplits() const {
        return regions_.size() > 1;
    }

    // 重置为单视图
    void reset();

    // 设置当前文档索引（为激活区域）
    void setCurrentDocumentIndex(size_t index);

    // 设置指定区域的文档索引
    void setDocumentIndexForRegion(size_t region_index, size_t document_index);

    // 获取指定区域的文档索引
    size_t getDocumentIndexForRegion(size_t region_index) const;

    // 获取指定区域的文档索引列表
    const std::vector<size_t>& getDocumentIndicesForRegion(size_t region_index) const;

    // 为指定区域添加文档索引
    void addDocumentIndexToRegion(size_t region_index, size_t document_index);

    // 从指定区域移除文档索引
    void removeDocumentIndexFromRegion(size_t region_index, size_t document_index);

    // 获取区域数量
    size_t getRegionCount() const {
        return regions_.size();
    }

    // 获取当前激活区域的索引
    size_t getActiveRegionIndex() const {
        return active_region_index_;
    }

    // 更新区域尺寸
    void updateRegionSizes(int screen_width, int screen_height);

    // 调整分屏线位置
    void adjustSplitLinePosition(size_t line_index, int delta, int screen_width, int screen_height);

  private:
    std::vector<ViewRegion> regions_;
    std::vector<SplitLine> split_lines_;
    size_t active_region_index_;

    // 辅助方法
    ViewRegion* findRegionAt(int x, int y);
    SplitLine* findSplitLineAt(int x, int y);
    void adjustSplitLine(SplitLine& line, int new_position, int screen_width, int screen_height);
    bool isPointOnSplitLine(int x, int y, const SplitLine& line) const;
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_SPLIT_VIEW_H
