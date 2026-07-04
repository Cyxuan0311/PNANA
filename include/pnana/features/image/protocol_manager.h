#ifndef PNANA_FEATURES_PROTOCOL_MANAGER_H
#define PNANA_FEATURES_PROTOCOL_MANAGER_H

#include <cstdint>
#include <string>
#include <vector>

namespace pnana {
namespace features {

struct TerminalInfo {
    bool kitty = false;
    bool sixel = false;
    bool iterm2 = false;
    std::string name;
};

struct ProtocolImageData {
    std::string data;
    int term_rows = 0;
    int term_cols = 0;
    int render_w = 0;
    int render_h = 0;
    bool valid = false;
};

namespace detail {

inline std::string Base64Encode(const unsigned char* data, size_t len) {
    static const char enc[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                              "abcdefghijklmnopqrstuvwxyz"
                              "0123456789+/";
    std::string out;
    out.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int val = static_cast<unsigned int>(
            (static_cast<unsigned int>(data[i]) << 16) |
            (i + 1 < len ? (static_cast<unsigned int>(data[i + 1]) << 8) : 0) |
            (i + 2 < len ? static_cast<unsigned int>(data[i + 2]) : 0));
        out.push_back(enc[(val >> 18) & 0x3F]);
        out.push_back(enc[(val >> 12) & 0x3F]);
        out.push_back(i + 1 < len ? enc[(val >> 6) & 0x3F] : '=');
        out.push_back(i + 2 < len ? enc[val & 0x3F] : '=');
    }
    return out;
}

} // namespace detail

class ProtocolManager {
  public:
    static void init();
    static void detect();
    static bool isActive();
    static std::string protocolName();

    static bool encodeImage(const std::string& filepath, int pixel_w, int pixel_h,
                            ProtocolImageData& out);

    static void writeToTerminal(const ProtocolImageData& data, int term_row, int term_col);
    static void clearArea(int start_row, int num_rows);

    static bool isEnabled();
    static void setEnabled(bool on);
    static std::string getPreferred();
    static void setPreferred(const std::string& pref);

    static const TerminalInfo& getTerminalInfo();
    static std::vector<std::string> getAvailableProtocols();
    static std::string getActiveProtocolName();

    static int getActiveProtocol(); // 0=none, 1=kitty, 2=iterm2, 3=sixel
    static int getCellWidthPx();    // terminal cell width in pixels
    static int getCellHeightPx();   // terminal cell height in pixels
    static bool flushPending();
    static bool hasPending();
    static void setPending(const ProtocolImageData& data, int term_row, int term_col);
    static void updatePendingPosition(int term_row, int term_col);
    static void resendLast();

    // Cache one encoded image so we don't re-encode every frame
    static void cacheResult(const std::string& filepath, int pixel_w, int pixel_h,
                            const ProtocolImageData& data);
    static bool getCached(const std::string& filepath, int pixel_w, int pixel_h,
                          ProtocolImageData& out);

  private:
    static std::string detectTerminalByProcessTree();
    static std::string detectTerminalByEnv();
    static TerminalInfo nameToCapabilities(const std::string& name);
    static bool encodeKitty(const std::string& filepath, int pixel_w, int pixel_h,
                            ProtocolImageData& out);
    static bool encodeITerm2(const std::string& filepath, int pixel_w, int pixel_h,
                             ProtocolImageData& out);
    static bool encodeSixel(const std::string& filepath, int pixel_w, int pixel_h,
                            ProtocolImageData& out);

    static TerminalInfo s_info;
    static bool s_detected;
    static bool s_protocol_enabled;
    static std::string s_preferred;
    static int s_active_protocol; // 0=none, 1=kitty, 2=iterm2, 3=sixel

    static ProtocolImageData s_pending_data;
    static int s_pending_row;
    static int s_pending_col;
    static bool s_has_pending;

    // Cache: avoid re-encoding the same image every frame
    static std::string s_cache_key;
    static ProtocolImageData s_cache_data;

    // (removed: flush-once-per-setPending, no dirty check needed)
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_PROTOCOL_MANAGER_H
