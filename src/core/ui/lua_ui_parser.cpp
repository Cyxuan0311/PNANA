#ifdef BUILD_LUA_SUPPORT

#include "core/ui/lua_ui_parser.h"
#include "utils/logger.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace pnana {
namespace core {
namespace ui {

namespace {

WidgetType parseWidgetType(const char* type_str, bool& out_focusable) {
    out_focusable = false;
    if (!type_str) {
        return WidgetType::TEXT;
    }

    if (strcmp(type_str, "text") == 0) {
        return WidgetType::TEXT;
    }
    if (strcmp(type_str, "paragraph") == 0) {
        return WidgetType::PARAGRAPH;
    }
    if (strcmp(type_str, "separator") == 0) {
        return WidgetType::SEPARATOR;
    }
    if (strcmp(type_str, "canvas") == 0) {
        return WidgetType::CANVAS;
    }
    if (strcmp(type_str, "spinner") == 0) {
        return WidgetType::SPINNER;
    }
    if (strcmp(type_str, "image") == 0) {
        return WidgetType::IMAGE;
    }
    if (strcmp(type_str, "animation") == 0) {
        return WidgetType::ANIMATION;
    }
    if (strcmp(type_str, "bullet") == 0) {
        return WidgetType::BULLET;
    }
    if (strcmp(type_str, "link") == 0) {
        return WidgetType::LINK;
    }

    // 交互组件
    if (strcmp(type_str, "input") == 0) {
        out_focusable = true;
        return WidgetType::INPUT;
    }
    if (strcmp(type_str, "textarea") == 0) {
        out_focusable = true;
        return WidgetType::TEXTAREA;
    }
    if (strcmp(type_str, "button") == 0) {
        out_focusable = true;
        return WidgetType::BUTTON;
    }
    if (strcmp(type_str, "checkbox") == 0) {
        out_focusable = true;
        return WidgetType::CHECKBOX;
    }
    if (strcmp(type_str, "radiobox") == 0) {
        out_focusable = true;
        return WidgetType::RADIOBOX;
    }
    if (strcmp(type_str, "toggle") == 0) {
        out_focusable = true;
        return WidgetType::TOGGLE;
    }
    if (strcmp(type_str, "slider") == 0) {
        out_focusable = true;
        return WidgetType::SLIDER;
    }
    if (strcmp(type_str, "dropdown") == 0) {
        out_focusable = true;
        return WidgetType::DROPDOWN;
    }
    if (strcmp(type_str, "menu") == 0) {
        out_focusable = true;
        return WidgetType::MENU;
    }
    if (strcmp(type_str, "color_picker") == 0) {
        out_focusable = true;
        return WidgetType::COLOR_PICKER;
    }
    if (strcmp(type_str, "file_picker") == 0) {
        out_focusable = true;
        return WidgetType::FILE_PICKER;
    }
    if (strcmp(type_str, "gauge") == 0) {
        return WidgetType::GAUGE;
    }
    if (strcmp(type_str, "list") == 0) {
        out_focusable = true;
        return WidgetType::LIST;
    }

    // 容器组件
    if (strcmp(type_str, "window") == 0) {
        return WidgetType::WINDOW;
    }
    if (strcmp(type_str, "container") == 0) {
        return WidgetType::CONTAINER;
    }
    if (strcmp(type_str, "group") == 0) {
        return WidgetType::GROUP;
    }
    if (strcmp(type_str, "hbox") == 0) {
        return WidgetType::HBOX;
    }
    if (strcmp(type_str, "vbox") == 0) {
        return WidgetType::VBOX;
    }
    if (strcmp(type_str, "dbox") == 0) {
        return WidgetType::DBOX;
    }
    if (strcmp(type_str, "split") == 0) {
        return WidgetType::SPLIT;
    }
    if (strcmp(type_str, "resizable_split") == 0) {
        return WidgetType::RESIZABLE_SPLIT;
    }
    if (strcmp(type_str, "tabs") == 0) {
        return WidgetType::TABS;
    }
    if (strcmp(type_str, "grid") == 0) {
        return WidgetType::GRID;
    }
    if (strcmp(type_str, "frame") == 0) {
        return WidgetType::FRAME;
    }
    if (strcmp(type_str, "yframe") == 0) {
        return WidgetType::YFRAME;
    }
    if (strcmp(type_str, "xframe") == 0) {
        return WidgetType::XFRAME;
    }
    if (strcmp(type_str, "vscroll") == 0) {
        return WidgetType::VSCROLL;
    }
    if (strcmp(type_str, "hscroll") == 0) {
        return WidgetType::HSCROLL;
    }

    // 弹窗/模态组件
    if (strcmp(type_str, "modal") == 0) {
        return WidgetType::MODAL;
    }
    if (strcmp(type_str, "popup") == 0) {
        return WidgetType::POPUP;
    }
    if (strcmp(type_str, "notification") == 0) {
        return WidgetType::NOTIFICATION;
    }

    return WidgetType::TEXT;
}

} // namespace

void LuaUIParser::parseLayoutOptionsFromLua(lua_State* L, int index, WidgetSpec& spec) {
    if (!lua_istable(L, index)) {
        return;
    }

    // 解析 direction
    lua_getfield(L, index, "direction");
    if (lua_isstring(L, -1)) {
        const char* dir = lua_tostring(L, -1);
        if (strcmp(dir, "horizontal") == 0 || strcmp(dir, "h") == 0) {
            spec.layout_direction = LayoutDirection::HORIZONTAL;
        } else {
            spec.layout_direction = LayoutDirection::VERTICAL;
        }
    }
    lua_pop(L, 1);

    // 解析 align
    lua_getfield(L, index, "align");
    if (lua_isstring(L, -1)) {
        const char* align = lua_tostring(L, -1);
        if (strcmp(align, "start") == 0) {
            spec.alignment = Alignment::START;
        } else if (strcmp(align, "center") == 0) {
            spec.alignment = Alignment::CENTER;
        } else if (strcmp(align, "end") == 0) {
            spec.alignment = Alignment::END;
        } else if (strcmp(align, "stretch") == 0) {
            spec.alignment = Alignment::STRETCH;
        }
    }
    lua_pop(L, 1);

    // 解析 flex
    lua_getfield(L, index, "flex");
    if (lua_isnumber(L, -1)) {
        spec.flex = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    // 解析 min_width / min_height
    lua_getfield(L, index, "min_width");
    if (lua_isnumber(L, -1)) {
        spec.min_width = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "min_height");
    if (lua_isnumber(L, -1)) {
        spec.min_height = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    // 解析 padding / spacing
    lua_getfield(L, index, "padding");
    if (lua_isnumber(L, -1)) {
        spec.padding = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "spacing");
    if (lua_isnumber(L, -1)) {
        spec.spacing = static_cast<int>(lua_tonumber(L, -1));
    }
    lua_pop(L, 1);

    // 解析 border
    lua_getfield(L, index, "border");
    if (lua_isstring(L, -1)) {
        spec.border_style = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析 decorators（装饰器数组）
    lua_getfield(L, index, "decorators");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                lua_getfield(L, -1, "name");
                lua_getfield(L, -1, "value");
                if (lua_isstring(L, -2) && lua_type(L, -1) != LUA_TNIL) {
                    const char* dec_name = lua_tostring(L, -2);

                    // 解析颜色装饰器
                    if (strcmp(dec_name, "color") == 0) {
                        if (lua_isstring(L, -1)) {
                            spec.props["decorator.color"] = lua_tostring(L, -1);
                        }
                    } else if (strcmp(dec_name, "bgcolor") == 0) {
                        if (lua_isstring(L, -1)) {
                            spec.props["decorator.bgcolor"] = lua_tostring(L, -1);
                        }
                    }
                    // 解析渐变背景
                    else if (strcmp(dec_name, "gradient") == 0) {
                        if (lua_istable(L, -1)) {
                            lua_getfield(L, -1, "start");
                            lua_getfield(L, -1, "end");
                            lua_getfield(L, -1, "direction");
                            if (lua_isstring(L, -3) && lua_isstring(L, -2)) {
                                spec.props["decorator.gradient.start"] = lua_tostring(L, -3);
                                spec.props["decorator.gradient.end"] = lua_tostring(L, -2);
                                if (lua_isstring(L, -1)) {
                                    spec.props["decorator.gradient.direction"] =
                                        lua_tostring(L, -1);
                                }
                            }
                            lua_pop(L, 3);
                        }
                    }
                    // 解析边框装饰器
                    else if (strcmp(dec_name, "border") == 0) {
                        if (lua_isstring(L, -1)) {
                            spec.border_style = lua_tostring(L, -1);
                        }
                    } else if (strcmp(dec_name, "border_color") == 0) {
                        if (lua_isstring(L, -1)) {
                            spec.props["decorator.border_color"] = lua_tostring(L, -1);
                        }
                    }
                    // 解析布尔样式装饰器
                    else if (strcmp(dec_name, "bold") == 0 || strcmp(dec_name, "italic") == 0 ||
                             strcmp(dec_name, "underlined") == 0 || strcmp(dec_name, "dim") == 0 ||
                             strcmp(dec_name, "inverted") == 0 ||
                             strcmp(dec_name, "strikethrough") == 0 ||
                             strcmp(dec_name, "blink") == 0) {
                        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                            spec.props[std::string("decorator.") + dec_name] = "true";
                        }
                    }
                    // 解析对齐
                    else if (strcmp(dec_name, "align") == 0) {
                        if (lua_isstring(L, -1)) {
                            const char* align_val = lua_tostring(L, -1);
                            if (strcmp(align_val, "left") == 0) {
                                spec.alignment = Alignment::START;
                            } else if (strcmp(align_val, "center") == 0) {
                                spec.alignment = Alignment::CENTER;
                            } else if (strcmp(align_val, "right") == 0) {
                                spec.alignment = Alignment::END;
                            } else if (strcmp(align_val, "stretch") == 0) {
                                spec.alignment = Alignment::STRETCH;
                            }
                        }
                    }
                    // 解析尺寸
                    else if (strcmp(dec_name, "size") == 0) {
                        if (lua_istable(L, -1)) {
                            lua_getfield(L, -1, "width");
                            if (lua_isnumber(L, -1)) {
                                spec.min_width = static_cast<int>(lua_tonumber(L, -1));
                            }
                            lua_pop(L, 1);

                            lua_getfield(L, -1, "height");
                            if (lua_isnumber(L, -1)) {
                                spec.min_height = static_cast<int>(lua_tonumber(L, -1));
                            }
                            lua_pop(L, 1);
                        }
                    }
                    // 解析 margin
                    else if (strcmp(dec_name, "margin") == 0) {
                        if (lua_isnumber(L, -1)) {
                            spec.props["decorator.margin"] =
                                std::to_string(static_cast<int>(lua_tonumber(L, -1)));
                        }
                    }
                    // 解析 padding
                    else if (strcmp(dec_name, "padding") == 0) {
                        if (lua_isnumber(L, -1)) {
                            spec.padding = static_cast<int>(lua_tonumber(L, -1));
                        }
                    }
                    // 解析 flex
                    else if (strcmp(dec_name, "flex") == 0) {
                        if (lua_isnumber(L, -1)) {
                            spec.flex = static_cast<int>(lua_tonumber(L, -1));
                        }
                    }
                    // 解析阴影效果
                    else if (strcmp(dec_name, "shadow") == 0) {
                        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                            spec.props["decorator.shadow"] = "true";
                        }
                    }
                    // 解析圆角
                    else if (strcmp(dec_name, "rounded") == 0) {
                        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                            spec.border_style = "rounded";
                        }
                    }
                    // 解析透明度
                    else if (strcmp(dec_name, "opacity") == 0) {
                        if (lua_isnumber(L, -1)) {
                            double opacity = lua_tonumber(L, -1);
                            if (opacity >= 0.0 && opacity <= 1.0) {
                                spec.props["decorator.opacity"] = std::to_string(opacity);
                            }
                        }
                    }
                }
                lua_pop(L, 2);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 解析 events（事件处理）
    lua_getfield(L, index, "events");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_isstring(L, -2) && lua_isfunction(L, -1)) {
                const char* event_name = lua_tostring(L, -2);
                if (event_name) {
                    spec.props[std::string("event.") + event_name] = "lua_function";
                }
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 解析 canvas 特定属性
    lua_getfield(L, index, "width");
    if (lua_isnumber(L, -1)) {
        spec.props["canvas.width"] = std::to_string(static_cast<int>(lua_tonumber(L, -1)));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "height");
    if (lua_isnumber(L, -1)) {
        spec.props["canvas.height"] = std::to_string(static_cast<int>(lua_tonumber(L, -1)));
    }
    lua_pop(L, 1);

    // 解析 slider 特定属性
    lua_getfield(L, index, "min");
    if (lua_isnumber(L, -1)) {
        spec.props["slider.min"] = std::to_string(static_cast<int>(lua_tonumber(L, -1)));
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "max");
    if (lua_isnumber(L, -1)) {
        spec.props["slider.max"] = std::to_string(static_cast<int>(lua_tonumber(L, -1)));
    }
    lua_pop(L, 1);

    // 解析窗口特定属性（用于 vim.ui.* API）
    lua_getfield(L, index, "window_title");
    if (lua_isstring(L, -1)) {
        spec.window_title = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析标题装饰器
    lua_getfield(L, index, "window_title_decorators");
    if (lua_istable(L, -1)) {
        lua_getfield(L, -1, "bold");
        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
            spec.window_title_decorators.bold = true;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "inverted");
        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
            spec.window_title_decorators.inverted = true;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "dim");
        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
            spec.window_title_decorators.dim = true;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "underlined");
        if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
            spec.window_title_decorators.underlined = true;
        }
        lua_pop(L, 1);

        lua_getfield(L, -1, "color");
        if (lua_isstring(L, -1)) {
            spec.window_title_decorators.color = lua_tostring(L, -1);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "window_prompt");
    if (lua_isstring(L, -1)) {
        spec.window_prompt = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_input_line");
    if (lua_isstring(L, -1)) {
        spec.component_input_line = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_left_title");
    if (lua_isstring(L, -1)) {
        spec.component_left_title = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_right_title");
    if (lua_isstring(L, -1)) {
        spec.component_right_title = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_left_lines");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                spec.component_left_lines.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_right_lines");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                spec.component_right_lines.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "component_help_lines");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                spec.component_help_lines.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 解析 component_left_line_colors（每行颜色配置）
    lua_getfield(L, index, "component_left_line_colors");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                std::map<std::string, std::string> color_config;

                // 解析 fg（前景色）
                lua_getfield(L, -1, "fg");
                if (lua_isstring(L, -1)) {
                    color_config["fg"] = lua_tostring(L, -1);
                }
                lua_pop(L, 1);

                // 解析 bg（背景色）
                lua_getfield(L, -1, "bg");
                if (lua_isstring(L, -1)) {
                    color_config["bg"] = lua_tostring(L, -1);
                }
                lua_pop(L, 1);

                // 解析 bold
                lua_getfield(L, -1, "bold");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["bold"] = "true";
                }
                lua_pop(L, 1);

                // 解析 italic
                lua_getfield(L, -1, "italic");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["italic"] = "true";
                }
                lua_pop(L, 1);

                // 解析 underlined
                lua_getfield(L, -1, "underlined");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["underlined"] = "true";
                }
                lua_pop(L, 1);

                // 解析 dim
                lua_getfield(L, -1, "dim");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["dim"] = "true";
                }
                lua_pop(L, 1);

                // 解析 inverted
                lua_getfield(L, -1, "inverted");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["inverted"] = "true";
                }
                lua_pop(L, 1);

                // 解析 strikethrough
                lua_getfield(L, -1, "strikethrough");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["strikethrough"] = "true";
                }
                lua_pop(L, 1);

                spec.component_left_line_colors.push_back(color_config);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 解析 component_right_line_colors（每行颜色配置）
    lua_getfield(L, index, "component_right_line_colors");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                std::map<std::string, std::string> color_config;

                // 解析 fg（前景色）
                lua_getfield(L, -1, "fg");
                if (lua_isstring(L, -1)) {
                    color_config["fg"] = lua_tostring(L, -1);
                }
                lua_pop(L, 1);

                // 解析 bg（背景色）
                lua_getfield(L, -1, "bg");
                if (lua_isstring(L, -1)) {
                    color_config["bg"] = lua_tostring(L, -1);
                }
                lua_pop(L, 1);

                // 解析 bold
                lua_getfield(L, -1, "bold");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["bold"] = "true";
                }
                lua_pop(L, 1);

                // 解析 italic
                lua_getfield(L, -1, "italic");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["italic"] = "true";
                }
                lua_pop(L, 1);

                // 解析 underlined
                lua_getfield(L, -1, "underlined");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["underlined"] = "true";
                }
                lua_pop(L, 1);

                // 解析 dim
                lua_getfield(L, -1, "dim");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["dim"] = "true";
                }
                lua_pop(L, 1);

                // 解析 inverted
                lua_getfield(L, -1, "inverted");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["inverted"] = "true";
                }
                lua_pop(L, 1);

                // 解析 strikethrough
                lua_getfield(L, -1, "strikethrough");
                if (lua_isboolean(L, -1) && lua_toboolean(L, -1)) {
                    color_config["strikethrough"] = "true";
                }
                lua_pop(L, 1);

                spec.component_right_line_colors.push_back(color_config);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
}

WidgetSpec LuaUIParser::parseWidgetSpecFromLua(lua_State* L, int index) {
    WidgetSpec spec;

    if (!lua_istable(L, index)) {
        spec.type = WidgetType::TEXT;
        spec.label = "(invalid widget)";
        return spec;
    }

    // 递增 ID 计数器：保证一次解析过程中的 id 可用且不重复
    static std::uint64_t counter = 1;

    // 解析 type
    lua_getfield(L, index, "type");
    if (lua_isstring(L, -1)) {
        const char* type_str = lua_tostring(L, -1);
        bool focusable = false;
        spec.type = parseWidgetType(type_str, focusable);
        spec.focusable = focusable;
    }
    lua_pop(L, 1);

    // 解析 id
    lua_getfield(L, index, "id");
    if (lua_isstring(L, -1)) {
        spec.id = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析 label/text
    lua_getfield(L, index, "text");
    if (lua_isstring(L, -1)) {
        spec.label = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, index, "label");
    if (lua_isstring(L, -1)) {
        spec.label = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析 value
    lua_getfield(L, index, "value");
    if (lua_isstring(L, -1)) {
        spec.value = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    // 解析 items
    lua_getfield(L, index, "items");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            if (lua_isstring(L, -1)) {
                spec.items.emplace_back(lua_tostring(L, -1));
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // 布局选项（direction/align/flex/min_width/...）
    parseLayoutOptionsFromLua(L, index, spec);

    // 解析 on.*（仅记录 key；真正注册 callback 在 open_layout_window 中完成）
    lua_getfield(L, index, "on");
    if (lua_istable(L, -1)) {
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_isstring(L, -2) && lua_isfunction(L, -1)) {
                const char* event_name = lua_tostring(L, -2);
                if (event_name) {
                    spec.props[std::string("on.") + event_name] = "lua_function";
                }
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    // children
    lua_getfield(L, index, "children");
    if (lua_istable(L, -1)) {
        int len = static_cast<int>(luaL_len(L, -1));
        for (int i = 1; i <= len; ++i) {
            lua_rawgeti(L, -1, i);
            spec.children.push_back(parseWidgetSpecFromLua(L, -1));
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);

    if (spec.id.empty()) {
        spec.id = "widget_" + std::to_string(counter++);
    }

    return spec;
}

} // namespace ui
} // namespace core
} // namespace pnana

#endif // BUILD_LUA_SUPPORT
