#include "features/ui_refresh_scheduler.h"

namespace pnana {
namespace features {

UIRefreshScheduler::UIRefreshScheduler() = default;

UIRefreshScheduler::~UIRefreshScheduler() {
    stop();
}

void UIRefreshScheduler::start(ConditionFn should_refresh, TriggerFn trigger_refresh,
                               std::chrono::milliseconds interval) {
    stop();

    running_.store(true, std::memory_order_relaxed);
    worker_ = std::thread([this, should_refresh = std::move(should_refresh),
                           trigger_refresh = std::move(trigger_refresh), interval]() {
        while (running_.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(interval);

            bool need_refresh = false;
            try {
                need_refresh = should_refresh();
            } catch (...) {
                continue;
            }

            if (need_refresh) {
                trigger_refresh();
            }
        }
    });
}

void UIRefreshScheduler::stop() {
    running_.store(false, std::memory_order_relaxed);
    if (worker_.joinable()) {
        worker_.join();
    }
}

} // namespace features
} // namespace pnana
