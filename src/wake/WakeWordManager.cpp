// src/wake/WakeWordManager.cpp — high-level wake word detection controller
// ---------------------------------------------------------------------------
// Replaces the old TCP-client approach.  Now owns a WakeWordEngine (ONNX
// inference), a ModelManager (directory scanning), and a MicStream (audio
// capture).  The microphone feeds audio frames into the engine, which fires
// the user callback when a wake word is detected.
// ---------------------------------------------------------------------------

#include "wake/WakeWordManager.h"

#include <iostream>

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WakeWordManager::WakeWordManager(const std::string& modelsDir)
    : modelManager_(engine_, modelsDir),
      micStream_(1280, 16000) {
    // Wire the engine callback to our user-facing callback.
    engine_.setWakeCallback([this](const std::string& modelName) {
        std::cout << "[WakeWordManager] Wake word detected by '"
                  << modelName << "'\n";

        std::function<void()> cb;
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            cb = userCallback_;
        }
        if (cb) {
            cb();
        }
    });

    // Wire the mic stream to the engine.
    micStream_.setFrameCallback([this](const std::vector<float>& frame) {
        engine_.processAudioFrame(frame);
    });
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
    if (running_.load()) {
        return;
    }

    // Load all models from the models directory.
    const int loaded = modelManager_.scanAndLoad();
    std::cout << "[WakeWordManager] " << loaded << " model(s) loaded from "
              << modelManager_.modelsDirectory() << '\n';

    // Start background polling for new models.
    modelManager_.startWatching(5);

    // Start microphone capture.
    micStream_.start();

    running_.store(true);
    std::cout << "[WakeWordManager] Wake word detection started (ONNX engine)\n";
}

void WakeWordManager::stop() {
    if (!running_.load()) {
        return;
    }

    modelManager_.stopWatching();
    micStream_.stop();
    running_.store(false);
    std::cout << "[WakeWordManager] Wake word detection stopped\n";
}

void WakeWordManager::setDebouncePeriodMs(int ms) {
    engine_.setCooldownMs(ms);
}

bool WakeWordManager::isRunning() const {
    return running_.load();
}

WakeWordEngine& WakeWordManager::engine() {
    return engine_;
}

ModelManager& WakeWordManager::modelManager() {
    return modelManager_;
}

} // namespace atlas::wake
