#include "features/diff/myers_diff.h"
#include <algorithm>

namespace pnana {
namespace features {
namespace diff {

std::vector<DiffRecord> MyersDiff::compute(const std::vector<std::string>& old_lines,
                                           const std::vector<std::string>& new_lines) {
    const size_t n = old_lines.size();
    const size_t m = new_lines.size();

    // LCS DP（工程可维护实现；接口保持 MyersDiff）
    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (size_t i = 1; i <= n; ++i) {
        for (size_t j = 1; j <= m; ++j) {
            if (old_lines[i - 1] == new_lines[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    std::vector<DiffRecord> reversed;
    size_t i = n;
    size_t j = m;

    while (i > 0 || j > 0) {
        if (i > 0 && j > 0 && old_lines[i - 1] == new_lines[j - 1]) {
            reversed.push_back(DiffRecord{static_cast<int>(i - 1), DiffRecord::OpType::NO_CHANGE,
                                          old_lines[i - 1]});
            --i;
            --j;
        } else if (j > 0 && (i == 0 || dp[i][j - 1] >= dp[i - 1][j])) {
            reversed.push_back(
                DiffRecord{static_cast<int>(j - 1), DiffRecord::OpType::ADD, new_lines[j - 1]});
            --j;
        } else {
            reversed.push_back(
                DiffRecord{static_cast<int>(i - 1), DiffRecord::OpType::DELETE, old_lines[i - 1]});
            --i;
        }
    }

    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

std::vector<std::string> MyersDiff::applyForward(const std::vector<std::string>& base,
                                                 const std::vector<DiffRecord>& diff) {
    std::vector<std::string> output;
    output.reserve(base.size());

    size_t base_idx = 0;
    for (const auto& rec : diff) {
        switch (rec.op) {
            case DiffRecord::OpType::NO_CHANGE:
                if (base_idx < base.size()) {
                    output.push_back(base[base_idx]);
                    ++base_idx;
                }
                break;
            case DiffRecord::OpType::DELETE:
                if (base_idx < base.size()) {
                    ++base_idx;
                }
                break;
            case DiffRecord::OpType::ADD:
                output.push_back(rec.content);
                break;
        }
    }

    while (base_idx < base.size()) {
        output.push_back(base[base_idx]);
        ++base_idx;
    }

    return output;
}

} // namespace diff
} // namespace features
} // namespace pnana
