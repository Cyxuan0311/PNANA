#include "features/lsp/lsp_types.h"
#include "features/lsp/snippets/snippets_registry.h"
#include <vector>

namespace pnana {
namespace features {
namespace snippets {

std::vector<Snippet> getSqlSnippets() {
    return {
        // SELECT Statements
        {"select", "SELECT ${1:*} FROM ${2:table}", "SELECT statement"},
        {"selectw", "SELECT ${1:*} FROM ${2:table} WHERE ${3:condition}", "SELECT with WHERE"},
        {"selectj", "SELECT ${1:*} FROM ${2:table1} JOIN ${3:table2} ON ${4:condition}",
         "SELECT with JOIN"},
        {"selectl", "SELECT ${1:*} FROM ${2:table} LIMIT ${3:10}", "SELECT with LIMIT"},
        {"selecto", "SELECT ${1:*} FROM ${2:table} ORDER BY ${3:column}", "SELECT with ORDER BY"},
        {"selectg", "SELECT ${1:column}, COUNT(*) FROM ${2:table} GROUP BY ${1:column}",
         "SELECT with GROUP BY"},
        {"selectd", "SELECT DISTINCT ${1:column} FROM ${2:table}", "SELECT DISTINCT"},

        // INSERT Statements
        {"insert",
         "INSERT INTO ${1:table} (${2:column1}, ${3:column2}) VALUES (${4:value1}, ${5:value2})",
         "INSERT statement"},
        {"inserts", "INSERT INTO ${1:table} (${2:columns}) SELECT ${3:*} FROM ${4:source}",
         "INSERT SELECT"},

        // UPDATE Statements
        {"update", "UPDATE ${1:table} SET ${2:column} = ${3:value} WHERE ${4:condition}",
         "UPDATE statement"},
        {"updatem",
         "UPDATE ${1:table} SET ${2:column1} = ${3:value1}, ${4:column2} = ${5:value2} WHERE "
         "${6:condition}",
         "UPDATE multiple columns"},

        // DELETE Statements
        {"delete", "DELETE FROM ${1:table} WHERE ${2:condition}", "DELETE statement"},
        {"truncate", "TRUNCATE TABLE ${1:table}", "TRUNCATE table"},

        // CREATE Statements
        {"createtable",
         "CREATE TABLE ${1:table} (\n    ${2:id} INT PRIMARY KEY,\n    ${3:name} VARCHAR(255)\n)",
         "CREATE TABLE"},
        {"createindex", "CREATE INDEX ${1:index_name} ON ${2:table} (${3:column})", "CREATE INDEX"},
        {"createview", "CREATE VIEW ${1:view_name} AS\nSELECT ${2:*} FROM ${3:table}",
         "CREATE VIEW"},

        // ALTER Statements
        {"alter", "ALTER TABLE ${1:table} ADD COLUMN ${2:column} ${3:type}",
         "ALTER TABLE ADD COLUMN"},
        {"alterd", "ALTER TABLE ${1:table} DROP COLUMN ${2:column}", "ALTER TABLE DROP COLUMN"},
        {"alterm", "ALTER TABLE ${1:table} MODIFY COLUMN ${2:column} ${3:type}",
         "ALTER TABLE MODIFY COLUMN"},

        // DROP Statements
        {"drop", "DROP TABLE ${1:table}", "DROP TABLE"},
        {"dropi", "DROP INDEX ${1:index_name}", "DROP INDEX"},
        {"dropv", "DROP VIEW ${1:view_name}", "DROP VIEW"},

        // JOINs
        {"inner", "INNER JOIN ${1:table} ON ${2:condition}", "INNER JOIN"},
        {"left", "LEFT JOIN ${1:table} ON ${2:condition}", "LEFT JOIN"},
        {"right", "RIGHT JOIN ${1:table} ON ${2:condition}", "RIGHT JOIN"},
        {"full", "FULL OUTER JOIN ${1:table} ON ${2:condition}", "FULL OUTER JOIN"},
        {"cross", "CROSS JOIN ${1:table}", "CROSS JOIN"},

        // WHERE Clauses
        {"where", "WHERE ${1:condition}", "WHERE clause"},
        {"whereand", "WHERE ${1:condition1} AND ${2:condition2}", "WHERE with AND"},
        {"whereor", "WHERE ${1:condition1} OR ${2:condition2}", "WHERE with OR"},
        {"wherein", "WHERE ${1:column} IN (${2:value1}, ${3:value2})", "WHERE IN"},
        {"wherelike", "WHERE ${1:column} LIKE '${2:pattern}'", "WHERE LIKE"},
        {"wherebet", "WHERE ${1:column} BETWEEN ${2:value1} AND ${3:value2}", "WHERE BETWEEN"},
        {"whereis", "WHERE ${1:column} IS NULL", "WHERE IS NULL"},
        {"whereisn", "WHERE ${1:column} IS NOT NULL", "WHERE IS NOT NULL"},

        // Aggregate Functions
        {"count", "COUNT(${1:*})", "COUNT function"},
        {"sum", "SUM(${1:column})", "SUM function"},
        {"avg", "AVG(${1:column})", "AVG function"},
        {"max", "MAX(${1:column})", "MAX function"},
        {"min", "MIN(${1:column})", "MIN function"},

        // Window Functions
        {"row_number", "ROW_NUMBER() OVER (ORDER BY ${1:column})", "ROW_NUMBER window function"},
        {"rank", "RANK() OVER (PARTITION BY ${1:column} ORDER BY ${2:column})",
         "RANK window function"},
        {"dense_rank", "DENSE_RANK() OVER (ORDER BY ${1:column})", "DENSE_RANK window function"},

        // Subqueries
        {"subquery",
         "SELECT ${1:*} FROM ${2:table} WHERE ${3:column} IN (SELECT ${4:column} FROM ${5:table2})",
         "Subquery"},
        {"exists", "WHERE EXISTS (SELECT 1 FROM ${1:table} WHERE ${2:condition})",
         "EXISTS subquery"},
        {"notexists", "WHERE NOT EXISTS (SELECT 1 FROM ${1:table} WHERE ${2:condition})",
         "NOT EXISTS subquery"},

        // Common Patterns
        {"union", "SELECT ${1:*} FROM ${2:table1}\nUNION\nSELECT ${1:*} FROM ${3:table2}", "UNION"},
        {"unionall", "SELECT ${1:*} FROM ${2:table1}\nUNION ALL\nSELECT ${1:*} FROM ${3:table2}",
         "UNION ALL"},
        {"case", "CASE\n    WHEN ${1:condition} THEN ${2:value}\n    ELSE ${3:default}\nEND",
         "CASE expression"},
        {"coalesce", "COALESCE(${1:column}, ${2:default})", "COALESCE function"},
        {"nullif", "NULLIF(${1:column}, ${2:value})", "NULLIF function"},

        // Transactions
        {"begin", "BEGIN TRANSACTION;", "BEGIN transaction"},
        {"commit", "COMMIT;", "COMMIT"},
        {"rollback", "ROLLBACK;", "ROLLBACK"},

        // Comments
        {"cmt", "-- ${1:comment}", "Single line comment"},
        {"cmtb", "/*\n * ${1:comment}\n */", "Block comment"},
    };
}

} // namespace snippets
} // namespace features
} // namespace pnana
