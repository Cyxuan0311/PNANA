#ifndef PNANA_CORE_BUFFER_FACTORY_H
#define PNANA_CORE_BUFFER_FACTORY_H

#include "core/buffer_backend.h"
#include "core/gap_buffer.h"
#include "core/piece_table.h"
#include "core/rope.h"
#include "core/sqrt_decomposition.h"
#include <algorithm>
#include <filesystem>
#include <memory>
#include <string>

namespace pnana {
namespace core {

// Smart Buffer Factory
// Automatically selects the optimal buffer backend based on file size and type
class SmartBufferFactory {
  public:
    // File size thresholds (bytes)
    static constexpr size_t SMALL_FILE_THRESHOLD = 1024 * 1024;       // 1MB
    static constexpr size_t MEDIUM_FILE_THRESHOLD = 10 * 1024 * 1024; // 10MB
    static constexpr size_t LARGE_FILE_THRESHOLD = 50 * 1024 * 1024;  // 50MB

    // Select backend by file size
    static BufferBackendType selectBackendBySize(size_t file_size) {
        if (file_size < SMALL_FILE_THRESHOLD) {
            // Small files: Use GapBuffer, simple implementation, fast single-character operations
            return BufferBackendType::GAP_BUFFER;
        } else if (file_size < MEDIUM_FILE_THRESHOLD) {
            // Medium files: Use SqrtDecomposition, balances query and update efficiency
            return BufferBackendType::SQRT_DECOMPOSITION;
        } else if (file_size < LARGE_FILE_THRESHOLD) {
            // Large files: Use Rope, avoids large text copying
            return BufferBackendType::ROPE;
        } else {
            // Very large files: Use PieceTable, highest memory efficiency
            return BufferBackendType::PIECE_TABLE;
        }
    }

    // Select backend by file extension (optimized for specific file types)
    static BufferBackendType selectBackendByExtension(const std::string& filepath) {
        std::string ext = getFileExtension(filepath);

        // Convert to lowercase
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        // Log files: Usually append-write, use PieceTable
        if (ext == ".log" || ext == ".txt") {
            return BufferBackendType::PIECE_TABLE;
        }

        // Code files: Frequent editing, use GapBuffer or PieceTable
        if (ext == ".cpp" || ext == ".h" || ext == ".hpp" || ext == ".c" || ext == ".cc" ||
            ext == ".java" || ext == ".py" || ext == ".js" || ext == ".ts" || ext == ".go" ||
            ext == ".rs" || ext == ".rb" || ext == ".php" || ext == ".cs") {
            return BufferBackendType::PIECE_TABLE;
        }

        // Configuration files: Usually small, use GapBuffer
        if (ext == ".json" || ext == ".xml" || ext == ".yaml" || ext == ".yml" || ext == ".toml" ||
            ext == ".ini" || ext == ".conf" || ext == ".cfg") {
            return BufferBackendType::GAP_BUFFER;
        }

        // Markup languages: Use PieceTable
        if (ext == ".md" || ext == ".rst" || ext == ".adoc") {
            return BufferBackendType::PIECE_TABLE;
        }

        // Other files: Default to PieceTable (generally optimal)
        return BufferBackendType::PIECE_TABLE;
    }

    // Select optimal backend based on both file size and type
    static BufferBackendType selectBackend(const std::string& filepath, size_t file_size = 0) {
        // If file doesn't exist or size is 0, select by extension
        if (file_size == 0) {
            return selectBackendByExtension(filepath);
        }

        // Large files: prioritize size-based selection
        if (file_size >= MEDIUM_FILE_THRESHOLD) {
            return selectBackendBySize(file_size);
        }

        // Small files: select by type
        return selectBackendByExtension(filepath);
    }

    // Create smart buffer instance
    static std::unique_ptr<BufferBackend> create(const std::string& filepath,
                                                 size_t file_size = 0) {
        BufferBackendType type = selectBackend(filepath, file_size);

        switch (type) {
            case BufferBackendType::GAP_BUFFER:
                return std::make_unique<GapBuffer>();

            case BufferBackendType::SQRT_DECOMPOSITION:
                return std::make_unique<SqrtDecomposition>();

            case BufferBackendType::ROPE:
                return std::make_unique<Rope>();

            case BufferBackendType::PIECE_TABLE:
                return std::make_unique<PieceTable>();

            default:
                return std::make_unique<PieceTable>();
        }
    }

    // Create instance by backend type
    static std::unique_ptr<BufferBackend> create(BufferBackendType type) {
        switch (type) {
            case BufferBackendType::GAP_BUFFER:
                return std::make_unique<GapBuffer>();

            case BufferBackendType::SQRT_DECOMPOSITION:
                return std::make_unique<SqrtDecomposition>();

            case BufferBackendType::ROPE:
                return std::make_unique<Rope>();

            case BufferBackendType::PIECE_TABLE:
                return std::make_unique<PieceTable>();

            default:
                return std::make_unique<PieceTable>();
        }
    }

    // Get backend name
    static const char* getBackendName(BufferBackendType type) {
        switch (type) {
            case BufferBackendType::GAP_BUFFER:
                return "GapBuffer";
            case BufferBackendType::SQRT_DECOMPOSITION:
                return "SqrtDecomposition";
            case BufferBackendType::ROPE:
                return "Rope";
            case BufferBackendType::PIECE_TABLE:
                return "PieceTable";
            default:
                return "Unknown";
        }
    }

    // Get backend description
    static std::string getBackendDescription(BufferBackendType type) {
        switch (type) {
            case BufferBackendType::GAP_BUFFER:
                return "Gap Buffer - Ideal for small text (<1MB), frequent local editing, simple "
                       "implementation, fast single-char operations";
            case BufferBackendType::SQRT_DECOMPOSITION:
                return "Sqrt Decomposition - Ideal for medium text (1-10MB), balances query and "
                       "update efficiency";
            case BufferBackendType::ROPE:
                return "Rope - Ideal for large text (>10MB), avoids large text copying, used by VS "
                       "Code/Sublime";
            case BufferBackendType::PIECE_TABLE:
                return "Piece Table - Gold standard for text editors, red-black tree based, used "
                       "by Notepad++/Vim";
            default:
                return "Unknown backend";
        }
    }

  private:
    static std::string getFileExtension(const std::string& filepath) {
        size_t pos = filepath.find_last_of('.');
        if (pos != std::string::npos && pos > 0) {
            return filepath.substr(pos + 1);
        }
        return "";
    }
};

} // namespace core
} // namespace pnana

#endif // PNANA_CORE_BUFFER_FACTORY_H
