#pragma once

// ---------------------------------------------------------------------------
// WakeWordManager — high-level controller for wake-word detection
// ---------------------------------------------------------------------------
// Owns a WakeWordClient (TCP fallback) and adds debouncing / duplicate
// activation protection.  Application code interacts only with this class.
//
// Usage:
//     atlas::wake::WakeWordManager wakeManager;
//     wakeManager.setCallback([](){ std::cout << "Wake!\n"; });
//     wakeManager.start();
//     // … run main loop …
//     wakeManager.stop();
// ---------------------------------------------------------------------------

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace atlas::wake {

// Forward declaration — implementation detail.
class WakeWordClient;

class WakeWordManager {
public:
    /// Construct the manager.
    /// \p host / \p port specify the Python wake-word service address.
    explicit WakeWordManager(const std::string& host = "127.0.0.1", int port = 5055);

    ~WakeWordManager();

    // Non-copyable, non-movable.
    WakeWordManager(const WakeWordManager&) = delete;
    WakeWordManager& operator=(const WakeWordManager&) = delete;

    /// Set the callback invoked when the wake word is detected (debounced).
    void setCallback(std::function<void()> callback);

    /// Start listening for wake-word events.
    void start();

    /// Stop listening and release resources.
    void stop();

    /// Minimum time (ms) between successive wake-word activations.
    void setDebouncePeriodMs(int ms);

    /// Query whether the manager is currently active.
    bool isRunning() const;

private:
    std::unique_ptr<WakeWordClient> client_;

    std::function<void()> userCallback_;
    std::mutex callbackMutex_;

    /// Debounce tracking.
    std::atomic<int> debouncePeriodMs_{2000};  // 2 s default
    std::chrono::steady_clock::time_point lastTrigger_;
    std::mutex triggerMutex_;

    /// Called by the WakeWordClient on every raw "WAKE" message.
    void onRawWake();
};

} // namespace atlas::wake
