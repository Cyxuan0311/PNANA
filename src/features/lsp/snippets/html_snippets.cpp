#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getHtmlSnippets() {
    return {
        // Document Structure
        {"html", "<!DOCTYPE html>\n<html lang=\"${1:en}\">\n<head>\n    <meta charset=\"UTF-8\">\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n    <title>${2:Document}</title>\n</head>\n<body>\n    ${3:// content}\n</body>\n</html>",
         "HTML5 document"},
        {"head", "<head>\n    <meta charset=\"UTF-8\">\n    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n    <title>${1:Title}</title>\n</head>",
         "Head section"},
        {"meta", "<meta ${1:name}=\"${2:content}\">",
         "Meta tag"},
        {"link", "<link rel=\"${1:stylesheet}\" href=\"${2:style.css}\">",
         "Link tag"},
        {"script", "<script src=\"${1:script.js}\"></script>",
         "Script tag"},
        {"scripti", "<script>\n    ${1:// code}\n</script>",
         "Inline script"},

        // Text Elements
        {"h1", "<h1>${1:Heading}</h1>",
         "Heading 1"},
        {"h2", "<h2>${1:Heading}</h2>",
         "Heading 2"},
        {"h3", "<h3>${1:Heading}</h3>",
         "Heading 3"},
        {"h4", "<h4>${1:Heading}</h4>",
         "Heading 4"},
        {"h5", "<h5>${1:Heading}</h5>",
         "Heading 5"},
        {"h6", "<h6>${1:Heading}</h6>",
         "Heading 6"},
        {"p", "<p>${1:Paragraph}</p>",
         "Paragraph"},
        {"span", "<span>${1:text}</span>",
         "Span"},
        {"div", "<div>${1:// content}</div>",
         "Div"},
        {"br", "<br>",
         "Line break"},
        {"hr", "<hr>",
         "Horizontal rule"},

        // Lists
        {"ul", "<ul>\n    <li>${1:item}</li>\n</ul>",
         "Unordered list"},
        {"ol", "<ol>\n    <li>${1:item}</li>\n</ol>",
         "Ordered list"},
        {"li", "<li>${1:item}</li>",
         "List item"},
        {"dl", "<dl>\n    <dt>${1:term}</dt>\n    <dd>${2:definition}</dd>\n</dl>",
         "Definition list"},

        // Links and Media
        {"a", "<a href=\"${1:#}\">${2:link}</a>",
         "Anchor link"},
        {"img", "<img src=\"${1:image.jpg}\" alt=\"${2:description}\">",
         "Image"},
        {"video", "<video src=\"${1:video.mp4}\" controls></video>",
         "Video"},
        {"audio", "<audio src=\"${1:audio.mp3}\" controls></audio>",
         "Audio"},

        // Forms
        {"form", "<form action=\"${1:#}\" method=\"${2:post}\">\n    ${3:// inputs}\n</form>",
         "Form"},
        {"input", "<input type=\"${1:text}\" name=\"${2:name}\" id=\"${2:name}\">",
         "Input"},
        {"inputt", "<input type=\"${1:text}\" name=\"${2:name}\" placeholder=\"${3:placeholder}\">",
         "Input with placeholder"},
        {"textarea", "<textarea name=\"${1:name}\" id=\"${1:name}\" rows=\"${2:4}\" cols=\"${3:50}\"></textarea>",
         "Textarea"},
        {"select", "<select name=\"${1:name}\" id=\"${1:name}\">\n    <option value=\"${2:value}\">${3:text}</option>\n</select>",
         "Select dropdown"},
        {"option", "<option value=\"${1:value}\">${2:text}</option>",
         "Option"},
        {"button", "<button type=\"${1:button}\">${2:Button}</button>",
         "Button"},
        {"label", "<label for=\"${1:id}\">${2:Label}</label>",
         "Label"},

        // Tables
        {"table", "<table>\n    <thead>\n        <tr>\n            <th>${1:Header}</th>\n        </tr>\n    </thead>\n    <tbody>\n        <tr>\n            <td>${2:Data}</td>\n        </tr>\n    </tbody>\n</table>",
         "Table"},
        {"tr", "<tr>\n    <td>${1:cell}</td>\n</tr>",
         "Table row"},
        {"td", "<td>${1:cell}</td>",
         "Table cell"},
        {"th", "<th>${1:header}</th>",
         "Table header"},

        // Semantic HTML5
        {"header", "<header>\n    ${1:// header content}\n</header>",
         "Header"},
        {"nav", "<nav>\n    ${1:// navigation}\n</nav>",
         "Navigation"},
        {"main", "<main>\n    ${1:// main content}\n</main>",
         "Main content"},
        {"article", "<article>\n    ${1:// article content}\n</article>",
         "Article"},
        {"section", "<section>\n    ${1:// section content}\n</section>",
         "Section"},
        {"aside", "<aside>\n    ${1:// sidebar content}\n</aside>",
         "Aside"},
        {"footer", "<footer>\n    ${1:// footer content}\n</footer>",
         "Footer"},

        // Text Formatting
        {"strong", "<strong>${1:text}</strong>",
         "Strong (bold)"},
        {"em", "<em>${1:text}</em>",
         "Emphasis (italic)"},
        {"code", "<code>${1:code}</code>",
         "Inline code"},
        {"pre", "<pre><code>${1:code}</code></pre>",
         "Preformatted code"},
        {"blockquote", "<blockquote>\n    ${1:quote}\n</blockquote>",
         "Blockquote"},

        // Comments
        {"cmt", "<!-- ${1:comment} -->",
         "HTML comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana

