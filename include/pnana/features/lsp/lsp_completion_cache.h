#ifndef PNANA_FEATURES_LSP_LSP_COMPLETION_CACHE_H
#define PNANA_FEATURES_LSP_LSP_COMPLETION_CACHE_H

#include "features/lsp/lsp_client.h"
#include <chrono>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace pnana {
namespace features {

/**
 * 补全缓存
 * 缓存补全结果，支持前缀过滤
 */
class LspCompletionCache {
  public:
    struct CacheKey {
        std::string uri;
        int line;
        int character;
        std::string prefix; // 当前输入的前缀

        bool operator<(const CacheKey& other) const {
            if (uri != other.uri)
                return uri < other.uri;
            if (line != other.line)
                return line < other.line;
            if (character != other.character)
                return character < other.character;
            return prefix < other.prefix;
        }
    };

    struct CacheValue {
        std::vector<CompletionItem> items;
        std::chrono::steady_clock::time_point timestamp;
        bool is_complete; // 是否完整（服务器返回的所有结果）

        CacheValue() : is_complete(false) {
            timestamp = std::chrono::steady_clock::now();
        }
    };

    LspCompletionCache();

    // 获取缓存
    std::optional<std::vector<CompletionItem>> get(const CacheKey& key);

    // 设置缓存
    void set(const CacheKey& key, const std::vector<CompletionItem>& items,
             bool is_complete = true);

    // 清除缓存（文档变更时）
    void invalidate(const std::string& uri);

    // 根据前缀过滤缓存结果
    std::vector<CompletionItem> filterByPrefix(const CacheKey& key, const std::string& new_prefix);

    // 清除所有缓存
    void clear();

    // 获取缓存统计信息
    size_t size() const;

  private:
    std::map<CacheKey, CacheValue> cache_;
    // LRU list (most-recent at front)
    std::list<CacheKey> lru_list_;
    std::map<CacheKey, std::list<CacheKey>::iterator> lru_index_;
    mutable std::mutex cache_mutex_;

    static constexpr size_t MAX_CACHE_SIZE = 500; // larger default
    static constexpr auto CACHE_TTL = std::chrono::minutes(5);

    // 清理过期缓存
    void cleanupExpired();

    // 清理最旧的缓存项（当缓存满时）
    void evictOldest();

    // 更新 LRU（move key to front）
    void touchLRU(const CacheKey& key);
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_LSP_LSP_COMPLETION_CACHE_H
