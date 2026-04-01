#ifndef PNANA_CORE_UI_WIDGET_H
#define PNANA_CORE_UI_WIDGET_H

#include <map>
#include <string>
#include <vector>

namespace pnana {
namespace core {
namespace ui {

enum class WidgetType {
    // 基础显示组件
    TEXT,      // 普通文本
    PARAGRAPH, // 自动换行段落
    SEPARATOR, // 分隔线
    CANVAS,    // 2D 绘图画布
    SPINNER,   // 加载动画
    IMAGE,     // 终端图片
    ANIMATION, // 自定义动画
    BULLET,    // 列表小圆点
    LINK,      // 超链接样式文本

    // 基础交互组件
    INPUT,        // 文本输入框
    TEXTAREA,     // 多行文本输入域
    BUTTON,       // 普通按钮
    CHECKBOX,     // 复选框
    RADIOBOX,     // 单选框
    TOGGLE,       // 开关按钮
    SLIDER,       // 滑动条
    DROPDOWN,     // 下拉选择框
    MENU,         // 列表菜单
    LIST,         // 列表选择（向后兼容）
    COLOR_PICKER, // 颜色选择器
    FILE_PICKER,  // 文件选择器
    GAUGE,        // 进度条/仪表盘
    SCROLL,       // 滚动容器（向后兼容）

    // 容器组件（布局/嵌套）
    WINDOW,          // 带标题边框的窗口
    CONTAINER,       // 通用容器
    GROUP,           // 分组框（简洁边框）
    HBOX,            // 水平布局盒子
    VBOX,            // 垂直布局盒子
    DBOX,            // 深度层叠容器
    SPLIT,           // 分割布局
    RESIZABLE_SPLIT, // 可拖动分割面板
    TABS,            // 标签页容器
    GRID,            // 网格布局
    FRAME,           // 自动适配/填充父容器
    YFRAME,          // 纵向填充容器
    XFRAME,          // 横向填充容器
    VSCROLL,         // 带垂直滚动条的容器
    HSCROLL,         // 带水平滚动条的容器

    // 弹窗/模态组件
    MODAL,        // 模态弹窗
    POPUP,        // 悬浮弹出层
    NOTIFICATION, // 通知提示
};

// 布局方向
enum class LayoutDirection {
    HORIZONTAL,
    VERTICAL,
};

// 对齐方式
enum class Alignment {
    START,   // 左/上对齐
    CENTER,  // 居中
    END,     // 右/下对齐
    STRETCH, // 拉伸填充
};

enum class WidgetActionType {
    NONE,
    SUBMIT,
    CANCEL,
    CLOSE,
    CUSTOM,
};

struct WidgetAction {
    WidgetActionType type = WidgetActionType::NONE;
    std::string command;
};

struct WidgetSpec {
    WidgetType type = WidgetType::TEXT;
    std::string id;
    std::string label;
    std::string value;
    bool focusable = false;
    bool focused = false;
    std::vector<std::string> items;
    int selected_index = 0;
    int scroll_offset = 0;

    // 正式化 action dispatch
    WidgetAction action;
    bool on_submit = false;

    // 可扩展属性（声明式扩展）
    std::map<std::string, std::string> props;

    // 布局属性
    LayoutDirection layout_direction = LayoutDirection::VERTICAL; // 布局方向
    Alignment alignment = Alignment::START;                       // 对齐方式
    int flex = 0;                        // 弹性系数（0=固定大小）
    int min_width = 0;                   // 最小宽度
    int min_height = 0;                  // 最小高度
    int padding = 0;                     // 内边距
    int spacing = 0;                     // 子元素间距
    std::string border_style = "single"; // 边框样式：none, single, double, rounded

    // 标题装饰器配置
    struct TitleDecorators {
        bool bold = false;
        bool inverted = false;
        bool dim = false;
        bool underlined = false;
        std::string color; // 颜色：black, red, green, yellow, blue, magenta, cyan, white
    };
    TitleDecorators window_title_decorators;

    // 窗口特定属性（用于 vim.ui.* API）
    std::string window_title;                       // 窗口标题
    std::string window_prompt;                      // 提示文本
    std::string component_input_line;               // 输入行
    std::string component_left_title;               // 左面板标题
    std::string component_right_title;              // 右面板标题
    std::vector<std::string> component_left_lines;  // 左面板行（结果列表）
    std::vector<std::string> component_right_lines; // 右面板行（预览）
    std::vector<std::string> component_help_lines;  // 帮助文本行

    // 每行文本的颜色配置（新增）
    std::vector<std::map<std::string, std::string>> component_left_line_colors;
    std::vector<std::map<std::string, std::string>> component_right_line_colors;

    std::vector<WidgetSpec> children;
};

} // namespace ui
} // namespace core
} // namespace pnana

#endif // PNANA_CORE_UI_WIDGET_H
