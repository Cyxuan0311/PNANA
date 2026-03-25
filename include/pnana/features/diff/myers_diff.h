#ifndef PNANA_FEATURES_DIFF_MYERS_DIFF_H
#define PNANA_FEATURES_DIFF_MYERS_DIFF_H

#include <string>
#include <vector>

namespace pnana {
namespace features {
namespace diff {

struct DiffRecord {
    enum class OpType { ADD, DELETE, NO_CHANGE };

    int line_num = 0;
    OpType op = OpType::NO_CHANGE;
    std::string content;
};

class MyersDiff {
  public:
    static std::vector<DiffRecord> compute(const std::vector<std::string>& old_lines,
                                           const std::vector<std::string>& new_lines);

    static std::vector<std::string> applyForward(const std::vector<std::string>& base,
                                                 const std::vector<DiffRecord>& diff);
};

} // namespace diff
} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_DIFF_MYERS_DIFF_H
