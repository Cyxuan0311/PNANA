#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getCssSnippets() {
    return {
        // Selectors
        {"class", ".${1:class-name} {\n    ${2:// styles}\n}", "Class selector"},
        {"id", "#${1:id-name} {\n    ${2:// styles}\n}", "ID selector"},
        {"element", "${1:element} {\n    ${2:// styles}\n}", "Element selector"},
        {"descendant", "${1:parent} ${2:child} {\n    ${3:// styles}\n}", "Descendant selector"},
        {"child", "${1:parent} > ${2:child} {\n    ${3:// styles}\n}", "Child selector"},
        {"adjacent", "${1:element} + ${2:sibling} {\n    ${3:// styles}\n}", "Adjacent sibling"},
        {"general", "${1:element} ~ ${2:sibling} {\n    ${3:// styles}\n}", "General sibling"},
        {"attribute", "${1:element}[${2:attribute}] {\n    ${3:// styles}\n}",
         "Attribute selector"},
        {"hover", "${1:element}:hover {\n    ${2:// styles}\n}", "Hover pseudo-class"},
        {"focus", "${1:element}:focus {\n    ${2:// styles}\n}", "Focus pseudo-class"},
        {"before", "${1:element}::before {\n    content: \"${2:}\";\n    ${3:// styles}\n}",
         "Before pseudo-element"},
        {"after", "${1:element}::after {\n    content: \"${2:}\";\n    ${3:// styles}\n}",
         "After pseudo-element"},

        // Layout
        {"flex", "display: flex;", "Flexbox"},
        {"flexcol", "display: flex;\nflex-direction: column;", "Flex column"},
        {"flexrow", "display: flex;\nflex-direction: row;", "Flex row"},
        {"grid", "display: grid;\ngrid-template-columns: ${1:repeat(3, 1fr)};", "CSS Grid"},
        {"center", "display: flex;\njustify-content: center;\nalign-items: center;",
         "Center content"},
        {"position", "position: ${1:relative};\ntop: ${2:0};\nleft: ${3:0};", "Position"},
        {"absolute", "position: absolute;\ntop: ${1:0};\nleft: ${2:0};", "Absolute positioning"},
        {"relative", "position: relative;", "Relative positioning"},
        {"fixed", "position: fixed;\ntop: ${1:0};\nleft: ${2:0};", "Fixed positioning"},
        {"sticky", "position: sticky;\ntop: ${1:0};", "Sticky positioning"},

        // Spacing
        {"margin", "margin: ${1:0};", "Margin"},
        {"padding", "padding: ${1:0};", "Padding"},
        {"margin4", "margin: ${1:top} ${2:right} ${3:bottom} ${4:left};", "Margin (4 values)"},
        {"padding4", "padding: ${1:top} ${2:right} ${3:bottom} ${4:left};", "Padding (4 values)"},

        // Typography
        {"font",
         "font-family: ${1:Arial}, sans-serif;\nfont-size: ${2:16px};\nfont-weight: ${3:normal};",
         "Font properties"},
        {"fontsize", "font-size: ${1:16px};", "Font size"},
        {"fontweight", "font-weight: ${1:normal};", "Font weight"},
        {"lineheight", "line-height: ${1:1.5};", "Line height"},
        {"textalign", "text-align: ${1:left};", "Text align"},
        {"textdec", "text-decoration: ${1:none};", "Text decoration"},
        {"texttrans", "text-transform: ${1:uppercase};", "Text transform"},

        // Colors
        {"color", "color: ${1:#000};", "Text color"},
        {"bg", "background-color: ${1:#fff};", "Background color"},
        {"bgimg", "background-image: url('${1:image.jpg}');", "Background image"},
        {"bgsize", "background-size: ${1:cover};", "Background size"},
        {"bgpos", "background-position: ${1:center};", "Background position"},
        {"bgrepeat", "background-repeat: ${1:no-repeat};", "Background repeat"},

        // Borders
        {"border", "border: ${1:1px} solid ${2:#000};", "Border"},
        {"borderr", "border-radius: ${1:4px};", "Border radius"},
        {"borderw", "border-width: ${1:1px};", "Border width"},
        {"borders", "border-style: ${1:solid};", "Border style"},
        {"borderc", "border-color: ${1:#000};", "Border color"},

        // Sizing
        {"width", "width: ${1:100%};", "Width"},
        {"height", "height: ${1:100%};", "Height"},
        {"maxw", "max-width: ${1:100%};", "Max width"},
        {"maxh", "max-height: ${1:100%};", "Max height"},
        {"minw", "min-width: ${1:0};", "Min width"},
        {"minh", "min-height: ${1:0};", "Min height"},

        // Display
        {"block", "display: block;", "Block display"},
        {"inline", "display: inline;", "Inline display"},
        {"inlineb", "display: inline-block;", "Inline-block display"},
        {"none", "display: none;", "Display none"},
        {"hidden", "visibility: hidden;", "Visibility hidden"},

        // Overflow
        {"overflow", "overflow: ${1:hidden};", "Overflow"},
        {"overflowx", "overflow-x: ${1:hidden};", "Overflow X"},
        {"overflowy", "overflow-y: ${1:hidden};", "Overflow Y"},
        {"scroll", "overflow: scroll;", "Overflow scroll"},

        // Transforms and Animations
        {"transform", "transform: ${1:translateX(0)};", "Transform"},
        {"transition", "transition: ${1:all} ${2:0.3s} ${3:ease};", "Transition"},
        {"animation", "animation: ${1:name} ${2:1s} ${3:ease} ${4:infinite};", "Animation"},
        {"keyframes",
         "@keyframes ${1:name} {\n    from {\n        ${2:// styles}\n    }\n    to {\n        "
         "${3:// styles}\n    }\n}",
         "Keyframes"},

        // Media Queries
        {"media", "@media (${1:max-width: 768px}) {\n    ${2:// styles}\n}", "Media query"},
        {"mediamin", "@media (min-width: ${1:768px}) {\n    ${2:// styles}\n}",
         "Media query min-width"},
        {"mediamax", "@media (max-width: ${1:768px}) {\n    ${2:// styles}\n}",
         "Media query max-width"},

        // Variables
        {"var", "var(--${1:variable-name})", "CSS variable"},
        {"root", ":root {\n    --${1:variable-name}: ${2:value};\n}", "Root variables"},

        // Common Patterns
        {"reset", "* {\n    margin: 0;\n    padding: 0;\n    box-sizing: border-box;\n}",
         "CSS reset"},
        {"boxsizing", "box-sizing: border-box;", "Box sizing"},
        {"clearfix", "&::after {\n    content: \"\";\n    display: table;\n    clear: both;\n}",
         "Clearfix"},
        {"ellipsis", "overflow: hidden;\ntext-overflow: ellipsis;\nwhite-space: nowrap;",
         "Text ellipsis"},

        // Comments
        {"cmt", "/* ${1:comment} */", "CSS comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
