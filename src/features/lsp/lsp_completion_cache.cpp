#include "features/lsp/lsp_completion_cache.h"
#include "utils/logger.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <sstream>

namespace pnana {
namespace features {

static bool matchesCamelCase(const std::string& pattern, const std::string& label) {
    if (pattern.empty() || label.empty()) {
        return false;
    }

    size_t pi = 0;
    size_t li = 0;
    size_t pattern_len = pattern.length();
    size_t label_len = label.length();

    while (pi < pattern_len && li < label_len) {
        if (pattern[pi] == label[li]) {
            pi++;
            li++;
        } else if (std::isupper(static_cast<unsigned char>(label[li]))) {
            li++;
        } else {
            return false;
        }
    }

    return pi == pattern_len;
}

static int camelCaseMatchScore(const std::string& pattern, const std::string& label) {
    if (pattern.empty() || label.empty()) {
        return 0;
    }

    size_t pi = 0;
    size_t li = 0;
    size_t label_len = label.length();
    int consecutive = 0;
    int max_consecutive = 0;
    size_t first_match_pos = std::string::npos;

    while (li < label_len) {
        if (pi < pattern.length() && pattern[pi] == label[li]) {
            if (first_match_pos == std::string::npos) {
                first_match_pos = li;
            }
            consecutive++;
            max_consecutive = std::max(max_consecutive, consecutive);
            pi++;
        } else if (std::isupper(static_cast<unsigned char>(label[li]))) {
            consecutive = 0;
        } else {
            consecutive = 0;
        }
        li++;
    }

    if (pi != pattern.length()) {
        return 0;
    }

    int score = 0;
    score += max_consecutive * 15;
    if (first_match_pos != std::string::npos) {
        score += static_cast<int>(100 - first_match_pos);
    }
    return score;
}

LspCompletionCache::LspCompletionCache() {}

void LspCompletionCache::setDebugLoggingEnabled(bool enabled) {
    debug_logging_enabled_ = enabled;
}

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

static bool isContextShiftLikely(const std::string& prefix) {
    if (prefix.empty())
        return false;
    char last = prefix.back();
    return std::isspace(static_cast<unsigned char>(last)) || last == '\n' || last == '\t' ||
           last == '(' || last == ')' || last == '{' || last == '}' || last == ';' || last == ',' ||
           last == ':';
}

std::vector<CompletionItem> LspCompletionCache::getFallbackItems(const CacheKey& key) {
    std::vector<CompletionItem> best;
    int best_score = -1;

    for (const auto& [cached_key, cached_value] : cache_) {
        if (cached_key.uri != key.uri || cached_value.items.empty())
            continue;

        int score = 0;
        if (cached_key.line == key.line)
            score += 100;
        score -= std::abs(cached_key.line - key.line) * 10;
        score -= std::abs(cached_key.character - key.character);

        if (!key.context_prefix.empty() && !cached_key.context_prefix.empty()) {
            if (key.context_prefix == cached_key.context_prefix) {
                score += 80;
            } else if (key.context_prefix.find(cached_key.context_prefix) == 0 ||
                       cached_key.context_prefix.find(key.context_prefix) == 0) {
                score += 40;
            } else {
                score -= 60;
            }
        }

        if (!key.trigger_character.empty() && !cached_key.trigger_character.empty()) {
            if (key.trigger_character == cached_key.trigger_character) {
                score += 50;
            } else {
                score -= 50;
            }
        }

        if (!key.semantic_context.empty() && !cached_key.semantic_context.empty()) {
            if (key.semantic_context == cached_key.semantic_context) {
                score += 20;
            } else {
                score -= 20;
            }
        }

        if (isContextShiftLikely(key.prefix) || isContextShiftLikely(key.context_prefix)) {
            score -= 100;
        }

        if (score > best_score) {
            best_score = score;
            best = cached_value.items;
        }
    }

    if (debug_logging_enabled_) {
        LOG_DEBUG("[LspCompletionCache] fallback scored uri=" + key.uri +
                  " line=" + std::to_string(key.line) + " char=" + std::to_string(key.character) +
                  " prefix='" + key.prefix + "' context='" + key.context_prefix + "' trigger='" +
                  key.trigger_character + "' result=" + std::to_string(best.size()) +
                  " score=" + std::to_string(best_score));
    }

    return best_score >= 0 ? best : std::vector<CompletionItem>{};
}

std::vector<CompletionItem> LspCompletionCache::filterByPrefix(const CacheKey& key,
                                                               const std::string& new_prefix,
                                                               const std::string& context_line,
                                                               int cursor_col) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    if (debug_logging_enabled_) {
        LOG_DEBUG("[LspCompletionCache] filter begin uri=" + key.uri +
                  " line=" + std::to_string(key.line) + " char=" + std::to_string(key.character) +
                  " prefix='" + new_prefix + "' cache_size=" + std::to_string(cache_.size()));
    }

    (void)new_prefix;

    // 查找相同位置但不同前缀的缓存项
    // 弱化同一行复用：要求 character 也尽量接近，减少跨位置误复用
    int checked_count = 0;
    int found_candidates = 0;
    int accepted_candidates = 0;
    for (const auto& [cached_key, cached_value] : cache_) {
        checked_count++;
        // 允许在同一行复用，但优先要求列位置足够接近，减少不稳定复用
        if (cached_key.uri == key.uri && cached_key.line == key.line &&
            !cached_value.items.empty()) {
            found_candidates++;
            (void)cached_value;

            int char_distance = std::abs(cached_key.character - key.character);
            bool close_enough = char_distance <= 6;
            if (!close_enough) {
                if (debug_logging_enabled_) {
                    LOG_DEBUG("[LspCompletionCache] skip candidate line=" +
                              std::to_string(cached_key.line) +
                              " char_distance=" + std::to_string(char_distance));
                }
                continue;
            }

            // 如果新前缀是旧前缀的扩展，可以过滤
            // 或者旧前缀是空（所有结果），也可以过滤
            bool can_filter = cached_key.prefix.empty() ||
                              new_prefix.find(cached_key.prefix) == 0 ||
                              cached_key.prefix.find(new_prefix) == 0;

            (void)can_filter;
            (void)cached_key;
            (void)new_prefix;

            if (can_filter) {
                accepted_candidates++;
                if (debug_logging_enabled_) {
                    LOG_DEBUG("[LspCompletionCache] candidate accepted prefix='" +
                              cached_key.prefix + "' -> new_prefix='" + new_prefix +
                              "' items=" + std::to_string(cached_value.items.size()) +
                              " char_distance=" + std::to_string(char_distance));
                }
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
                    // 使用 filterText 过滤（LSP 规范），无则用 label
                    std::string label = !item.filterText.empty() ? item.filterText : item.label;
                    std::string lower_label = label;
                    std::transform(lower_label.begin(), lower_label.end(), lower_label.begin(),
                                   ::tolower);
                    std::string normalized_label = normalizeForMatching(label);
                    std::string lower_normalized_label = normalizeForMatching(lower_label);

                    int score = 0;

                    // filterText 显式匹配加分：LSP 服务器明确指定该符号应在此前缀下出现
                    if (!item.filterText.empty() && item.filterText == new_prefix) {
                        score += 500;
                    }

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
                        // CamelCase 匹配：前缀中的大写字母匹配 label 中的大写字母
                        // 例如 "ABC" 匹配 "AbstractBaseClass"
                        else if (matchesCamelCase(new_prefix, label)) {
                            int cc_score = camelCaseMatchScore(new_prefix, label);
                            score = 7500 + getTypePriority(item.kind) * 100 + cc_score;
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

                // 上下文相关性加分
                if (!context_line.empty()) {
                    std::string lower_line = context_line;
                    std::transform(lower_line.begin(), lower_line.end(), lower_line.begin(),
                                   ::tolower);
                    for (auto& scored : scored_items) {
                        int ctx_score = 0;
                        std::string lower_symbol = scored.item.label;
                        std::transform(lower_symbol.begin(), lower_symbol.end(),
                                       lower_symbol.begin(), ::tolower);
                        size_t pos = 0;
                        int occurrences = 0;
                        while ((pos = lower_line.find(lower_symbol, pos)) != std::string::npos) {
                            occurrences++;
                            pos += lower_symbol.length();
                        }
                        ctx_score += occurrences * 15;
                        if (cursor_col > 0) {
                            size_t before_cursor =
                                std::min(static_cast<size_t>(cursor_col), context_line.length());
                            std::string before = context_line.substr(0, before_cursor);
                            std::transform(before.begin(), before.end(), before.begin(), ::tolower);
                            if (before.find(lower_symbol) != std::string::npos) {
                                ctx_score += 30;
                            }
                        }
                        scored.score += ctx_score;
                    }
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

                if (filtered.size() < 3) {
                    if (debug_logging_enabled_) {
                        LOG_DEBUG("[LspCompletionCache] filtered too small uri=" + key.uri +
                                  " line=" + std::to_string(key.line) + " char=" +
                                  std::to_string(key.character) + " prefix='" + new_prefix +
                                  "' result=" + std::to_string(filtered.size()) + " -> fallback");
                    }
                    auto fallback = getFallbackItems(key);
                    if (!fallback.empty()) {
                        if (debug_logging_enabled_) {
                            LOG_DEBUG("[LspCompletionCache] fallback used uri=" + key.uri +
                                      " line=" + std::to_string(key.line) +
                                      " char=" + std::to_string(key.character) +
                                      " fallback_items=" + std::to_string(fallback.size()));
                        }
                        return fallback;
                    }
                }

                if (debug_logging_enabled_) {
                    LOG_DEBUG("[LspCompletionCache] filter success uri=" + key.uri +
                              " line=" + std::to_string(key.line) +
                              " char=" + std::to_string(key.character) + " prefix='" + new_prefix +
                              "' checked=" + std::to_string(checked_count) +
                              " candidates=" + std::to_string(found_candidates) +
                              " accepted=" + std::to_string(accepted_candidates) +
                              " result=" + std::to_string(filtered.size()));
                }
                return filtered;
            }
        }
    }

    if (debug_logging_enabled_) {
        LOG_DEBUG("[LspCompletionCache] filter empty uri=" + key.uri +
                  " line=" + std::to_string(key.line) + " char=" + std::to_string(key.character) +
                  " prefix='" + new_prefix + "' checked=" + std::to_string(checked_count) +
                  " candidates=" + std::to_string(found_candidates) +
                  " accepted=" + std::to_string(accepted_candidates) + " -> fallback");
    }
    auto fallback = getFallbackItems(key);
    if (!fallback.empty()) {
        if (debug_logging_enabled_) {
            LOG_DEBUG("[LspCompletionCache] fallback used on empty uri=" + key.uri + " line=" +
                      std::to_string(key.line) + " char=" + std::to_string(key.character) +
                      " fallback_items=" + std::to_string(fallback.size()));
        }
        return fallback;
    }
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
