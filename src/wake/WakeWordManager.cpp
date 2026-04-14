#include "wake/WakeWordManager.h"
#include "wake/WakeWordClient.h"

#include <iostream>

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WakeWordManager::WakeWordManager(const std::string& host, int port)
    : client_(std::make_unique<WakeWordClient>(host, port)),
      lastTrigger_(std::chrono::steady_clock::now() - std::chrono::seconds(10)) {
    // Wire the raw client callback through our debounce logic.
    client_->setCallback([this]() { onRawWake(); });
}

WakeWordManager::~WakeWordManager() {
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void WakeWordManager::setCallback(std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    userCallback_ = std::move(callback);
}

void WakeWordManager::start() {
    client_->start();
    std::cout << "[WakeWordManager] Wake word detection started\n";
}

void WakeWordManager::stop() {
    client_->stop();
    std::cout << "[WakeWordManager] Wake word detection stopped\n";
}

void WakeWordManager::setDebouncePeriodMs(int ms) {
    debouncePeriodMs_.store(ms);
}

bool WakeWordManager::isRunning() const {
    return client_->isRunning();
}

// ---------------------------------------------------------------------------
// Debounce logic
// ---------------------------------------------------------------------------

void WakeWordManager::onRawWake() {
    const auto now = std::chrono::steady_clock::now();

    {
        std::lock_guard<std::mutex> lock(triggerMutex_);
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastTrigger_);

        if (elapsed.count() < debouncePeriodMs_.load()) {
            // Too soon after the last activation — suppress.
            std::cout << "[WakeWordManager] Wake suppressed (debounce: "
                      << elapsed.count() << " ms < " << debouncePeriodMs_.load()
                      << " ms)\n";
            return;
        }

        lastTrigger_ = now;
    }

    std::cout << "[WakeWordManager] Wake word detected!\n";

    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        cb = userCallback_;
    }

    if (cb) {
        cb();
    }
}

} // namespace atlas::wake
