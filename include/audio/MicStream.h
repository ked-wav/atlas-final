#pragma once

// ---------------------------------------------------------------------------
// MicStream — low-latency microphone capture with circular buffering
// ---------------------------------------------------------------------------
// Captures 16 kHz mono float32 audio from the default input device and feeds
// fixed-size frames to a callback.  Uses a lock-free circular buffer to
// decouple the real-time audio thread from the processing thread.
//
// Performance notes:
//   - The frame size controls the latency/accuracy trade-off.
//     512 samples = 32 ms, 1024 = 64 ms, 1280 = 80 ms.
//   - The circular buffer absorbs short processing stalls without dropping
//     audio.  If processing consistently takes longer than frame time,
//     increase the buffer or reduce the number of active models.
// ---------------------------------------------------------------------------

#include <atomic>
#include <cstddef>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace atlas::audio {

class MicStream {
public:
    /// \p frameSize  Number of float samples per frame delivered to the callback.
    /// \p sampleRate Capture sample rate in Hz (default 16000).
    explicit MicStream(std::size_t frameSize = 1280, int sampleRate = 16000);

    ~MicStream();

    // Non-copyable.
    MicStream(const MicStream&) = delete;
    MicStream& operator=(const MicStream&) = delete;

    /// Register the callback that receives each audio frame.
    /// The callback is invoked on the capture thread.
    void setFrameCallback(std::function<void(const std::vector<float>&)> callback);

    /// Start capturing audio.  Returns false if already running or on error.
    bool start();

    /// Stop capturing.
    void stop();

    /// Query whether the stream is active.
    bool isRunning() const;

    /// Get the configured frame size.
    std::size_t frameSize() const noexcept;

    /// Get the configured sample rate.
    int sampleRate() const noexcept;

private:
    std::size_t frameSize_;
    int sampleRate_;

    std::atomic<bool> running_{false};
    std::thread captureThread_;

    std::mutex callbackMutex_;
    std::function<void(const std::vector<float>&)> frameCallback_;

    // Circular buffer
    std::vector<float> ringBuffer_;
    std::atomic<std::size_t> writePos_{0};
    std::size_t readPos_{0};

    /// Background thread entry point — captures audio and dispatches frames.
    void captureLoop();

    /// Synthesize a silent audio frame (used when no real audio device is available).
    void generateSilence(std::vector<float>& buffer);
};

} // namespace atlas::audio
