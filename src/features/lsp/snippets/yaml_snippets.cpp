#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getYamlSnippets() {
    return {
        // Basic Structure
        {"key", "${1:key}: ${2:value}", "Key-value pair"},
        {"keyq", "\"${1:key}\": ${2:value}", "Quoted key"},
        {"list", "${1:key}:\n  - ${2:item1}\n  - ${3:item2}", "List"},
        {"listi", "- ${1:item}", "List item"},
        {"nested", "${1:parent}:\n  ${2:child}: ${3:value}", "Nested object"},
        {"multiline", "${1:key}: |\n  ${2:multiline text}", "Multiline string (literal)"},
        {"multilinef", "${1:key}: >\n  ${2:multiline text}", "Multiline string (folded)"},

        // Common Patterns
        {"doc", "---\n${1:# document}", "Document separator"},
        {"comment", "# ${1:comment}", "Comment"},
        {"anchor", "&${1:anchor} ${2:value}", "Anchor"},
        {"alias", "*${1:anchor}", "Alias"},
        {"merge", "<<: *${1:anchor}", "Merge key"},
        {"null", "${1:key}: null", "Null value"},
        {"bool", "${1:key}: ${2:true}", "Boolean"},
        {"int", "${1:key}: ${2:123}", "Integer"},
        {"float", "${1:key}: ${2:123.45}", "Float"},
        {"string", "${1:key}: \"${2:string}\"", "String"},
        {"date", "${1:key}: ${2:2023-01-01}", "Date"},

        // Docker Compose
        {"dockercompose",
         "version: '${1:3.8}'\nservices:\n  ${2:service}:\n    image: ${3:image}\n    ports:\n     "
         " - \"${4:8080}:80\"",
         "Docker Compose"},
        {"service", "${1:service}:\n  image: ${2:image}\n  ports:\n    - \"${3:8080}:80\"",
         "Docker service"},

        // GitHub Actions
        {"workflow",
         "name: ${1:Workflow}\non:\n  push:\n    branches: [ ${2:main} ]\njobs:\n  ${3:build}:\n   "
         " runs-on: ${4:ubuntu-latest}\n    steps:\n      - uses: actions/checkout@v2",
         "GitHub Actions workflow"},
        {"job",
         "${1:job}:\n  runs-on: ${2:ubuntu-latest}\n  steps:\n    - uses: ${3:actions/checkout@v2}",
         "GitHub Actions job"},
        {"step", "- name: ${1:Step name}\n  uses: ${2:action}", "GitHub Actions step"},

        // Kubernetes
        {"deployment",
         "apiVersion: apps/v1\nkind: Deployment\nmetadata:\n  name: ${1:deployment}\nspec:\n  "
         "replicas: ${2:1}\n  selector:\n    matchLabels:\n      app: ${1:deployment}\n  "
         "template:\n    metadata:\n      labels:\n        app: ${1:deployment}\n    spec:\n      "
         "containers:\n      - name: ${1:deployment}\n        image: ${3:image}",
         "Kubernetes Deployment"},
        {"servicek",
         "apiVersion: v1\nkind: Service\nmetadata:\n  name: ${1:service}\nspec:\n  selector:\n    "
         "app: ${2:app}\n  ports:\n  - port: ${3:80}\n    targetPort: ${4:8080}",
         "Kubernetes Service"},
        {"configmap",
         "apiVersion: v1\nkind: ConfigMap\nmetadata:\n  name: ${1:configmap}\ndata:\n  ${2:key}: "
         "${3:value}",
         "Kubernetes ConfigMap"},
        {"secret",
         "apiVersion: v1\nkind: Secret\nmetadata:\n  name: ${1:secret}\ntype: Opaque\ndata:\n  "
         "${2:key}: ${3:base64value}",
         "Kubernetes Secret"},

        // CI/CD
        {"gitlab", "${1:job}:\n  script:\n    - ${2:command}\n  only:\n    - ${3:main}",
         "GitLab CI job"},
        {"travis", "language: ${1:node_js}\nnode_js:\n  - ${2:18}\nscript:\n  - ${3:npm test}",
         "Travis CI config"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
