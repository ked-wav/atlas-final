// src/wake/WakeWordEngine.cpp — multi-model wake word processing engine
// ---------------------------------------------------------------------------
// Performance notes:
//   - Models are evaluated sequentially.  For 1–4 models this is fine on any
//     modern CPU.  If you load 5+ models, consider a thread pool that runs
//     each model in parallel and aggregates results.
//   - Batching (running one large tensor through multiple models) is possible
//     only when all models share identical input shapes.  This is uncommon
//     for user-provided models so it is not implemented here.
//   - The cooldown prevents retriggering on the same utterance.  A 2-second
//     default matches conversational pace.
// ---------------------------------------------------------------------------

#include "wake/WakeWordEngine.h"

#include <algorithm>
#include <iostream>

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WakeWordEngine::WakeWordEngine()
    : lastTrigger_(std::chrono::steady_clock::now() - std::chrono::seconds(10)) {}

WakeWordEngine::~WakeWordEngine() = default;

// ---------------------------------------------------------------------------
// Model management
// ---------------------------------------------------------------------------

bool WakeWordEngine::addModel(const std::string& path) {
    OnnxWakeModel model;
    if (!model.loadModel(path)) {
        std::cerr << "[WakeWordEngine] Failed to add model: " << path << '\n';
        return false;
    }

    std::lock_guard<std::mutex> lock(modelsMutex_);

    // Prevent duplicate loading.
    for (const auto& m : models_) {
        if (m.modelPath() == path) {
            std::cerr << "[WakeWordEngine] Model already loaded: " << path << '\n';
            return false;
        }
    }

    std::cout << "[WakeWordEngine] Model '" << model.modelName()
              << "' added (" << models_.size() + 1 << " active)\n";
    models_.push_back(std::move(model));
    return true;
}

bool WakeWordEngine::removeModel(const std::string& path) {
    std::lock_guard<std::mutex> lock(modelsMutex_);

    auto it = std::remove_if(models_.begin(), models_.end(),
        [&path](const OnnxWakeModel& m) { return m.modelPath() == path; });

    if (it == models_.end()) {
        return false;
    }

    std::cout << "[WakeWordEngine] Model removed: " << path << '\n';
    models_.erase(it, models_.end());
    return true;
}

std::vector<std::string> WakeWordEngine::listModels() const {
    std::lock_guard<std::mutex> lock(modelsMutex_);
    std::vector<std::string> paths;
    paths.reserve(models_.size());
    for (const auto& m : models_) {
        paths.push_back(m.modelPath());
    }
    return paths;
}

std::size_t WakeWordEngine::modelCount() const {
    std::lock_guard<std::mutex> lock(modelsMutex_);
    return models_.size();
}

// ---------------------------------------------------------------------------
// Audio processing
// ---------------------------------------------------------------------------

void WakeWordEngine::processAudioFrame(const std::vector<float>& frame) {
    std::lock_guard<std::mutex> lock(modelsMutex_);

    for (auto& model : models_) {
        const float confidence = model.predict(frame);

        if (confidence >= threshold_.load()) {
            // Check cooldown.
            const auto now = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::mutex> tLock(triggerMutex_);
                const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastTrigger_);

                if (elapsed.count() < cooldownMs_.load()) {
                    // Suppressed by cooldown — skip this detection.
                    continue;
                }
                lastTrigger_ = now;
            }

            std::cout << "[WakeWordEngine] Wake word detected by '"
                      << model.modelName() << "' (confidence="
                      << confidence << ")\n";

            std::function<void(const std::string&)> cb;
            {
                std::lock_guard<std::mutex> cLock(callbackMutex_);
                cb = wakeCallback_;
            }

            if (cb) {
                cb(model.modelName());
            }

            // Only trigger once per frame even if multiple models fire.
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void WakeWordEngine::setThreshold(float threshold) {
    threshold_.store(std::max(0.0f, std::min(1.0f, threshold)));
}

float WakeWordEngine::threshold() const {
    return threshold_.load();
}

void WakeWordEngine::setCooldownMs(int ms) {
    cooldownMs_.store(std::max(0, ms));
}

int WakeWordEngine::cooldownMs() const {
    return cooldownMs_.load();
}

// ---------------------------------------------------------------------------
// Callback
// ---------------------------------------------------------------------------

void WakeWordEngine::setWakeCallback(
    std::function<void(const std::string& modelName)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    wakeCallback_ = std::move(callback);
}

} // namespace atlas::wake
