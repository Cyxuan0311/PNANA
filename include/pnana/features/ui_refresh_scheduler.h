#ifndef PNANA_FEATURES_UI_REFRESH_SCHEDULER_H
#define PNANA_FEATURES_UI_REFRESH_SCHEDULER_H

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace pnana {
namespace features {

class UIRefreshScheduler {
  public:
    using ConditionFn = std::function<bool()>;
    using TriggerFn = std::function<void()>;

    UIRefreshScheduler();
    ~UIRefreshScheduler();

    void start(ConditionFn should_refresh, TriggerFn trigger_refresh,
               std::chrono::milliseconds interval = std::chrono::milliseconds(50));

    void stop();

  private:
    std::thread worker_;
    std::atomic<bool> running_{false};
};

} // namespace features
} // namespace pnana

#endif // PNANA_FEATURES_UI_REFRESH_SCHEDULER_H
