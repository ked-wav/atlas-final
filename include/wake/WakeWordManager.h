#pragma once

// ---------------------------------------------------------------------------
// WakeWordManager — high-level controller for wake-word detection
// ---------------------------------------------------------------------------
// Owns a WakeWordEngine, ModelManager, and MicStream.  Replaces the old TCP
// client approach with fully in-process ONNX inference.  Application code
// interacts only with this class.
//
// Usage:
//     atlas::wake::WakeWordManager wakeManager;
//     wakeManager.setCallback([](){ std::cout << "Wake!\n"; });
//     wakeManager.start();
//     // … run main loop …
//     wakeManager.stop();
//
// The manager also exposes the underlying engine and model manager so that
// higher-level code (e.g. drag-and-drop UI) can add models at runtime.
// ---------------------------------------------------------------------------

#include "wake/ModelManager.h"
#include "wake/WakeWordEngine.h"
#include "audio/MicStream.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace atlas::wake {

class WakeWordManager {
public:
    /// Construct the manager.
    /// \p modelsDir  Directory to scan for .onnx models on startup.
    explicit WakeWordManager(const std::string& modelsDir = "./models/wakewords");

    ~WakeWordManager();

    // Non-copyable, non-movable.
    WakeWordManager(const WakeWordManager&) = delete;
    WakeWordManager& operator=(const WakeWordManager&) = delete;

    /// Set the callback invoked when any wake word is detected.
    void setCallback(std::function<void()> callback);

    /// Start listening: load models, open mic stream, begin inference.
    void start();

    /// Stop listening and release resources.
    void stop();

    /// Minimum time (ms) between successive wake-word activations.
    void setDebouncePeriodMs(int ms);

    /// Query whether the manager is currently active.
    bool isRunning() const;

    /// Access the underlying engine (e.g. for UI integration).
    WakeWordEngine& engine();

    /// Access the model manager (e.g. for drag-and-drop file handling).
    ModelManager& modelManager();

private:
    WakeWordEngine engine_;
    ModelManager modelManager_;
    audio::MicStream micStream_;

    std::atomic<bool> running_{false};

    std::function<void()> userCallback_;
    std::mutex callbackMutex_;
};

} // namespace atlas::wake
