#pragma once

// ---------------------------------------------------------------------------
// MicRecorder — PortAudio-based real-time microphone capture
// ---------------------------------------------------------------------------
// Captures 16 kHz mono float32 audio from the default input device using
// PortAudio and delivers fixed-size frames to a registered callback.
//
// When PortAudio is not available at build time the class still compiles —
// start() returns false and no audio is captured.  This allows the rest of
// the pipeline to be exercised with synthetic input.
//
// Usage:
//     atlas::audio::MicRecorder mic;
//     mic.setCallback([](const float* data, std::size_t n) { … });
//     mic.start();   // opens default mic
//     …
//     mic.stop();
// ---------------------------------------------------------------------------

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <vector>

namespace atlas::audio {

class MicRecorder {
public:
    /// \p frameSize  Number of float samples per frame delivered to the
    ///               callback (default 1024 = 64 ms at 16 kHz).
    /// \p sampleRate Capture sample rate in Hz.
    explicit MicRecorder(std::size_t frameSize = 1024, int sampleRate = 16000);

    ~MicRecorder();

    // Non-copyable.
    MicRecorder(const MicRecorder&) = delete;
    MicRecorder& operator=(const MicRecorder&) = delete;

    /// Register the callback that receives each audio frame.
    /// \p callback  Receives a pointer to \c frameSize float samples and their
    ///              count.  Called from the PortAudio callback thread — keep it
    ///              fast (no allocations, no blocking).
    using AudioCallback = std::function<void(const float* samples,
                                             std::size_t  count)>;
    void setCallback(AudioCallback callback);

    /// Open the default input device and start streaming.
    /// Returns false if PortAudio is unavailable or the device cannot be opened.
    bool start();

    /// Stop streaming and close the device.
    void stop();

    /// Query whether the stream is active.
    bool isRunning() const;

    /// Configured frame size (samples per callback invocation).
    std::size_t frameSize() const noexcept { return frameSize_; }

    /// Configured sample rate in Hz.
    int sampleRate() const noexcept { return sampleRate_; }

private:
    std::size_t frameSize_;
    int         sampleRate_;

    std::atomic<bool> running_{false};

    std::mutex    callbackMutex_;
    AudioCallback audioCallback_;

    /// Opaque PortAudio stream handle (PaStream*).  Stored as void* to avoid
    /// leaking the PortAudio header into every translation unit.
    void* paStream_{nullptr};

    /// PortAudio callback (static).
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const void* timeInfo,
                          unsigned long statusFlags,
                          void* userData);
};

} // namespace atlas::audio
