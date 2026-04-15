#pragma once

// ---------------------------------------------------------------------------
// WakeWordEngine — manages multiple ONNX wake word models and processes audio
// ---------------------------------------------------------------------------
// Maintains a dynamic list of OnnxWakeModel instances and runs inference on
// every audio frame supplied by the MicStream.  When any model's confidence
// exceeds the configurable threshold, the registered callback fires.
//
// Performance notes:
//   - If many models are loaded, consider running inference in parallel via a
//     small thread pool.  The current implementation runs models sequentially
//     which is sufficient for 1–4 models on modern CPUs.
//   - Batching frames across models is another option but requires models
//     with identical input shapes.
// ---------------------------------------------------------------------------

#include "wake/OnnxWakeModel.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace atlas::wake {

class WakeWordEngine {
public:
    WakeWordEngine();
    ~WakeWordEngine();

    // Non-copyable.
    WakeWordEngine(const WakeWordEngine&) = delete;
    WakeWordEngine& operator=(const WakeWordEngine&) = delete;

    // ---- Model management --------------------------------------------------

    /// Load an ONNX model from \p path and add it to the active set.
    /// Returns true on success.  Thread-safe.
    bool addModel(const std::string& path);

    /// Remove a previously loaded model identified by its file path.
    /// Returns true if the model was found and removed.  Thread-safe.
    bool removeModel(const std::string& path);

    /// Return the file paths of all currently loaded models.
    std::vector<std::string> listModels() const;

    /// Return the number of loaded models.
    std::size_t modelCount() const;

    // ---- Audio processing --------------------------------------------------

    /// Feed a single audio frame (mono, 16 kHz, float32) into the engine.
    /// All loaded models will run inference on this frame.  If any model
    /// exceeds the threshold and the cooldown has elapsed, the wake callback
    /// fires.
    void processAudioFrame(const std::vector<float>& frame);

    // ---- Configuration -----------------------------------------------------

    /// Set the confidence threshold in [0, 1].  Default: 0.5.
    void setThreshold(float threshold);

    /// Get the current threshold.
    float threshold() const;

    /// Set the cooldown period after a wake detection.  Default: 2000 ms.
    void setCooldownMs(int ms);

    /// Get the current cooldown in milliseconds.
    int cooldownMs() const;

    // ---- Callback ----------------------------------------------------------

    /// Register the callback invoked when a wake word is detected.
    void setWakeCallback(std::function<void(const std::string& modelName)> callback);

private:
    mutable std::mutex modelsMutex_;
    std::vector<OnnxWakeModel> models_;

    std::atomic<float> threshold_{0.5f};
    std::atomic<int> cooldownMs_{2000};

    std::mutex callbackMutex_;
    std::function<void(const std::string& modelName)> wakeCallback_;

    std::mutex triggerMutex_;
    std::chrono::steady_clock::time_point lastTrigger_;
};

} // namespace atlas::wake
