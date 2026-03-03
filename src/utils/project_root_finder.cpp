#include "utils/project_root_finder.h"
#include <filesystem>

namespace fs = std::filesystem;

namespace pnana {
namespace utils {

std::string findRustProjectRoot(const std::string& filepath) {
    try {
        fs::path p = fs::path(filepath).parent_path();
        while (!p.empty() && p != p.root_path()) {
            if (fs::exists(p / "Cargo.toml")) {
                return p.string();
            }
            p = p.parent_path();
        }
    } catch (...) {
    }
    return "";
}

} // namespace utils
} // namespace pnana
