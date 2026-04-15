// src/audio/MicStream.cpp — microphone capture with circular buffer
// ---------------------------------------------------------------------------
// Performance notes:
//   - The circular buffer is sized to hold several seconds of audio so that
//     short processing stalls do not cause data loss.
//   - In production, replace the synthetic silence generator with a real
//     audio backend (PortAudio, ALSA, CoreAudio, WASAPI) that fills the
//     ring buffer from the hardware callback.
//   - Keep the frame callback fast — ideally just enqueue the frame for
//     processing on another thread or run a lightweight inference pass.
// ---------------------------------------------------------------------------

#include "audio/MicStream.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace atlas::audio {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MicStream::MicStream(std::size_t frameSize, int sampleRate)
    : frameSize_(frameSize),
      sampleRate_(sampleRate),
      // Ring buffer holds ~4 seconds of audio.
      ringBuffer_(static_cast<std::size_t>(sampleRate) * 4, 0.0f) {}

MicStream::~MicStream() {
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void MicStream::setFrameCallback(
    std::function<void(const std::vector<float>&)> callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    frameCallback_ = std::move(callback);
}

bool MicStream::start() {
    if (running_.load()) {
        return false;
    }
    running_.store(true);
    captureThread_ = std::thread(&MicStream::captureLoop, this);
    std::cout << "[MicStream] Capture started ("
              << sampleRate_ << " Hz, frame=" << frameSize_ << " samples)\n";
    return true;
}

void MicStream::stop() {
    running_.store(false);
    if (captureThread_.joinable()) {
        captureThread_.join();
    }
}

bool MicStream::isRunning() const {
    return running_.load();
}

std::size_t MicStream::frameSize() const noexcept {
    return frameSize_;
}

int MicStream::sampleRate() const noexcept {
    return sampleRate_;
}

// ---------------------------------------------------------------------------
// Capture loop
// ---------------------------------------------------------------------------

void MicStream::captureLoop() {
    // Frame duration in microseconds — used for real-time pacing.
    const auto frameDuration = std::chrono::microseconds(
        static_cast<long long>(frameSize_) * 1000000LL / sampleRate_);

    std::vector<float> frame(frameSize_, 0.0f);

    while (running_.load()) {
        const auto frameStart = std::chrono::steady_clock::now();

        // ---- Fill the ring buffer with audio data ----
        // In production, a real audio backend (PortAudio / ALSA / CoreAudio)
        // would write into ringBuffer_ from its hardware callback.  Here we
        // synthesize silence so the pipeline can be exercised without a
        // physical microphone.
        generateSilence(frame);

        // Write into ring buffer.
        const std::size_t bufSize = ringBuffer_.size();
        std::size_t wp = writePos_.load();
        for (std::size_t i = 0; i < frame.size(); ++i) {
            ringBuffer_[(wp + i) % bufSize] = frame[i];
        }
        writePos_.store((wp + frame.size()) % bufSize);

        // ---- Read a frame from the ring buffer and dispatch ----
        const std::size_t rp = readPos_;
        for (std::size_t i = 0; i < frame.size(); ++i) {
            frame[i] = ringBuffer_[(rp + i) % bufSize];
        }
        readPos_ = (rp + frame.size()) % bufSize;

        // Deliver to callback.
        {
            std::lock_guard<std::mutex> lock(callbackMutex_);
            if (frameCallback_) {
                frameCallback_(frame);
            }
        }

        // Pace the loop to match real-time audio rate.
        const auto elapsed = std::chrono::steady_clock::now() - frameStart;
        if (elapsed < frameDuration) {
            std::this_thread::sleep_for(frameDuration - elapsed);
        }
    }

    std::cout << "[MicStream] Capture stopped\n";
}

// ---------------------------------------------------------------------------
// Silence generator (placeholder for real audio capture)
// ---------------------------------------------------------------------------

void MicStream::generateSilence(std::vector<float>& buffer) {
    // Fill with silence (zeros).  Replace this method body with a read from
    // the audio hardware ring buffer in production.
    std::fill(buffer.begin(), buffer.end(), 0.0f);
}

} // namespace atlas::audio
