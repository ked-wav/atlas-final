#pragma once

// ---------------------------------------------------------------------------
// WhisperTranscriber — speech-to-text via whisper.cpp
// ---------------------------------------------------------------------------
// Wraps the whisper.cpp library for offline speech-to-text transcription.
// When whisper.cpp is not linked the class still compiles — transcribe()
// returns a placeholder string.
//
// Usage:
//     WhisperTranscriber asr;
//     asr.loadModel("/path/to/ggml-base.en.bin");
//     std::string text = asr.transcribe(pcmFloat16k);
// ---------------------------------------------------------------------------

#include <cstddef>
#include <string>
#include <vector>

namespace atlas::audio {

class WhisperTranscriber {
public:
    WhisperTranscriber();
    ~WhisperTranscriber();

    // Non-copyable.
    WhisperTranscriber(const WhisperTranscriber&) = delete;
    WhisperTranscriber& operator=(const WhisperTranscriber&) = delete;

    /// Load a GGML whisper model file.
    /// Returns true on success.
    bool loadModel(const std::string& modelPath);

    /// True if a model is loaded and ready.
    bool isLoaded() const;

    /// Transcribe 16 kHz mono float32 PCM audio.
    /// Returns the recognised text (UTF-8).
    std::string transcribe(const std::vector<float>& pcm16k) const;

    /// Transcribe from raw pointer + count.
    std::string transcribe(const float* samples, std::size_t count) const;

    /// Path to the currently loaded model (empty if none).
    const std::string& modelPath() const { return modelPath_; }

    // -- Tuning parameters --------------------------------------------------

    void setLanguage(const std::string& lang) { language_ = lang; }
    void setThreads(int n)                    { threads_  = n; }

private:
    std::string modelPath_;
    std::string language_{"en"};
    int         threads_{4};

    /// Opaque whisper context (struct whisper_context*).
    void* ctx_{nullptr};
};

} // namespace atlas::audio
