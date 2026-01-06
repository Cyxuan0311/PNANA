#include "features/lsp/lsp_completion_cache.h"
#include "utils/logger.h"
#include <algorithm>
#include <chrono>

namespace pnana {
namespace features {

LspCompletionCache::LspCompletionCache() {}

std::optional<std::vector<CompletionItem>> LspCompletionCache::get(const CacheKey& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    auto it = cache_.find(key);
    if (it == cache_.end()) {
        return std::nullopt;
    }

    // 检查是否过期
    auto now = std::chrono::steady_clock::now();
    auto age = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.timestamp);
    if (age > CACHE_TTL) {
        // erase from cache and LRU structures
        if (lru_index_.count(key)) {
            lru_list_.erase(lru_index_[key]);
            lru_index_.erase(key);
        }
        cache_.erase(it);
        return std::nullopt;
    }
    // Update LRU
    touchLRU(key);
    return it->second.items;
}

void LspCompletionCache::set(const CacheKey& key, const std::vector<CompletionItem>& items,
                             bool is_complete) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 如果已存在，更新并移动到 LRU front
    CacheValue value;
    value.items = items;
    value.is_complete = is_complete;
    value.timestamp = std::chrono::steady_clock::now();

    if (cache_.count(key)) {
        cache_[key] = value;
        touchLRU(key);
        return;
    }

    // 如果缓存已满，清理最旧的项
    if (cache_.size() >= MAX_CACHE_SIZE) {
        evictOldest();
    }

    // 插入新项，并加入 LRU
    cache_[key] = value;
    lru_list_.push_front(key);
    lru_index_[key] = lru_list_.begin();
}

void LspCompletionCache::invalidate(const std::string& uri) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // 删除所有匹配该 URI 的缓存项
    auto it = cache_.begin();
    while (it != cache_.end()) {
        if (it->first.uri == uri) {
            // remove from LRU structures
            if (lru_index_.count(it->first)) {
                lru_list_.erase(lru_index_[it->first]);
                lru_index_.erase(it->first);
            }
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

std::vector<CompletionItem> LspCompletionCache::filterByPrefix(const CacheKey& key,
                                                               const std::string& new_prefix) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    (void)new_prefix;

    // 查找相同位置但不同前缀的缓存项
    // 优化：允许在同一行的不同位置之间共享缓存（因为补全结果通常在同一行内是相似的）
    // 如果找到相同 URI、行（允许 character 不同）但不同前缀的缓存，尝试过滤
    int checked_count = 0;
    int found_candidates = 0;
    for (const auto& [cached_key, cached_value] : cache_) {
        checked_count++;
        // 允许在同一行的不同位置之间共享缓存
        // 这样可以提高缓存命中率，因为同一行的补全结果通常是相似的
        if (cached_key.uri == key.uri && cached_key.line == key.line &&
            !cached_value.items.empty()) {
            found_candidates++;
            (void)cached_value;

            // 如果新前缀是旧前缀的扩展，可以过滤
            // 或者旧前缀是空（所有结果），也可以过滤
            bool can_filter = cached_key.prefix.empty() ||
                              new_prefix.find(cached_key.prefix) == 0 ||
                              cached_key.prefix.find(new_prefix) == 0;

            (void)can_filter;
            (void)cached_key;
            (void)new_prefix;

            if (can_filter) {
                // 使用智能评分系统过滤和排序
                struct ScoredItem {
                    CompletionItem item;
                    int score;

                    bool operator<(const ScoredItem& other) const {
                        if (score != other.score) {
                            return score > other.score;
                        }
                        return item.label < other.item.label;
                    }
                };

                // 辅助函数：规范化字符串（去除特殊字符，用于匹配）
                auto normalizeForMatching = [](const std::string& str) -> std::string {
                    std::string normalized;
                    normalized.reserve(str.length());
                    for (char c : str) {
                        // 保留字母、数字、下划线、点、冒号、减号
                        // 去除 #、<、>、[、]、(、)、&、*、@ 等特殊字符
                        if (std::isalnum(c) || c == '_' || c == '.' || c == ':' || c == '-') {
                            normalized += c;
                        }
                    }
                    return normalized;
                };

                std::vector<ScoredItem> scored_items;
                std::string lower_prefix = new_prefix;
                std::transform(lower_prefix.begin(), lower_prefix.end(), lower_prefix.begin(),
                               ::tolower);
                std::string normalized_prefix = normalizeForMatching(new_prefix);
                std::string lower_normalized_prefix = normalizeForMatching(lower_prefix);

                // 获取类型优先级
                auto getTypePriority = [](const std::string& kind) -> int {
                    if (kind == "2" || kind == "3" || kind == "4")
                        return 10;
                    if (kind == "5" || kind == "6")
                        return 8;
                    if (kind == "7" || kind == "8" || kind == "22")
                        return 6;
                    if (kind == "21")
                        return 5;
                    if (kind == "14")
                        return 4;
                    return 3;
                };

                for (const auto& item : cached_value.items) {
                    std::string label = item.label;
                    std::string lower_label = label;
                    std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(),
                                   ::tolower);
                    std::string normalized_label = normalizeForMatching(label);
                    std::string lower_normalized_label = normalizeForMatching(lower_label);

                    int score = 0;

                    if (new_prefix.empty()) {
                        score = getTypePriority(item.kind) * 100;
                    } else {
                        // 精确匹配（大小写敏感）
                        if (label == new_prefix) {
                            score = 10000 + getTypePriority(item.kind) * 100;
                        }
                        // 精确匹配（大小写不敏感）
                        else if (lower_label == lower_prefix) {
                            score = 9000 + getTypePriority(item.kind) * 100;
                        }
                        // 规范化精确匹配（去除特殊字符后）
                        else if (normalized_label == normalized_prefix) {
                            score = 8500 + getTypePriority(item.kind) * 100;
                        }
                        // 规范化精确匹配（大小写不敏感）
                        else if (lower_normalized_label == lower_normalized_prefix) {
                            score = 8200 + getTypePriority(item.kind) * 100;
                        }
                        // 前缀匹配（大小写敏感）
                        else if (label.length() >= new_prefix.length() &&
                                 label.substr(0, new_prefix.length()) == new_prefix) {
                            score = 8000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                        }
                        // 前缀匹配（大小写不敏感）
                        else if (lower_label.length() >= lower_prefix.length() &&
                                 lower_label.substr(0, lower_prefix.length()) == lower_prefix) {
                            score = 7000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                        }
                        // 规范化前缀匹配（去除特殊字符后）
                        else if (!normalized_prefix.empty() &&
                                 normalized_label.length() >= normalized_prefix.length() &&
                                 normalized_label.substr(0, normalized_prefix.length()) ==
                                     normalized_prefix) {
                            score = 6500 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                        }
                        // 规范化前缀匹配（大小写不敏感）
                        else if (!lower_normalized_prefix.empty() &&
                                 lower_normalized_label.length() >=
                                     lower_normalized_prefix.length() &&
                                 lower_normalized_label.substr(0,
                                                               lower_normalized_prefix.length()) ==
                                     lower_normalized_prefix) {
                            score = 6000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(label.length()));
                        }
                        // 包含匹配（大小写敏感）
                        else if (label.find(new_prefix) != std::string::npos) {
                            size_t pos = label.find(new_prefix);
                            score = 5000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                        }
                        // 包含匹配（大小写不敏感）
                        else if (lower_label.find(lower_prefix) != std::string::npos) {
                            size_t pos = lower_label.find(lower_prefix);
                            score = 3000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                        }
                        // 规范化包含匹配（去除特殊字符后）
                        else if (!normalized_prefix.empty() &&
                                 normalized_label.find(normalized_prefix) != std::string::npos) {
                            size_t pos = normalized_label.find(normalized_prefix);
                            score = 2500 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                        }
                        // 规范化包含匹配（大小写不敏感）
                        else if (!lower_normalized_prefix.empty() &&
                                 lower_normalized_label.find(lower_normalized_prefix) !=
                                     std::string::npos) {
                            size_t pos = lower_normalized_label.find(lower_normalized_prefix);
                            score = 2000 + getTypePriority(item.kind) * 100;
                            score += (100 - static_cast<int>(pos));
                        }
                        // 模糊匹配（字符序列匹配，如 "usi" 匹配 "using"）- 最低优先级 (1000分)
                        else if (new_prefix.length() >= 3) { // 只对长度 >= 3 的前缀进行模糊匹配
                            // 检查前缀的字符是否按顺序出现在 label 中
                            size_t prefix_idx = 0;
                            for (size_t i = 0;
                                 i < lower_label.length() && prefix_idx < lower_prefix.length();
                                 i++) {
                                if (lower_label[i] == lower_prefix[prefix_idx]) {
                                    prefix_idx++;
                                }
                            }
                            if (prefix_idx == lower_prefix.length()) {
                                // 计算匹配质量：连续字符越多，分数越高
                                int consecutive = 0;
                                int max_consecutive = 0;
                                prefix_idx = 0;
                                for (size_t i = 0;
                                     i < lower_label.length() && prefix_idx < lower_prefix.length();
                                     i++) {
                                    if (lower_label[i] == lower_prefix[prefix_idx]) {
                                        consecutive++;
                                        max_consecutive = std::max(max_consecutive, consecutive);
                                        prefix_idx++;
                                    } else {
                                        consecutive = 0;
                                    }
                                }
                                score = 1000 + getTypePriority(item.kind) * 100;
                                // 连续字符越多，分数越高
                                score += max_consecutive * 10;
                                // 匹配位置越靠前越好
                                size_t first_match = lower_label.find(lower_prefix[0]);
                                if (first_match != std::string::npos) {
                                    score += (100 - static_cast<int>(first_match));
                                }
                            } else {
                                continue; // 跳过不匹配的项
                            }
                        }
                        // 不匹配 - 跳过
                        else {
                            continue;
                        }
                    }

                    scored_items.push_back({item, score});
                }

                // 按分数排序
                std::sort(scored_items.begin(), scored_items.end());

                // 提取排序后的 items
                std::vector<CompletionItem> filtered;
                filtered.reserve(scored_items.size());
                for (const auto& scored : scored_items) {
                    filtered.push_back(scored.item);
                }

                // 限制数量，提高响应速度
                if (filtered.size() > 30) {
                    filtered.resize(30);
                }

                (void)filtered;
                return filtered;
            }
        }
    }

    (void)checked_count;
    (void)found_candidates;
    return {};
}

void LspCompletionCache::clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_.clear();
    lru_list_.clear();
    lru_index_.clear();
}

size_t LspCompletionCache::size() const {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    return cache_.size();
}

void LspCompletionCache::cleanupExpired() {
    auto now = std::chrono::steady_clock::now();
    auto it = cache_.begin();
    while (it != cache_.end()) {
        auto age = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.timestamp);
        if (age > CACHE_TTL) {
            if (lru_index_.count(it->first)) {
                lru_list_.erase(lru_index_[it->first]);
                lru_index_.erase(it->first);
            }
            it = cache_.erase(it);
        } else {
            ++it;
        }
    }
}

void LspCompletionCache::evictOldest() {
    if (cache_.empty()) {
        return;
    }

    // 找到最旧的项
    // 使用 LRU 列表的末尾作为最旧项
    if (!lru_list_.empty()) {
        CacheKey oldest_key = lru_list_.back();
        lru_list_.pop_back();
        lru_index_.erase(oldest_key);
        cache_.erase(oldest_key);
    } else {
        // fallback: remove first map item
        auto it = cache_.begin();
        cache_.erase(it);
    }
}

void LspCompletionCache::touchLRU(const CacheKey& key) {
    // Move key to front of LRU list
    if (lru_index_.count(key)) {
        auto it = lru_index_[key];
        lru_list_.erase(it);
    }
    lru_list_.push_front(key);
    lru_index_[key] = lru_list_.begin();
}

} // namespace features
} // namespace pnana
