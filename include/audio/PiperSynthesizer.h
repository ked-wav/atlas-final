#pragma once

// ---------------------------------------------------------------------------
// PiperSynthesizer — text-to-speech via Piper TTS
// ---------------------------------------------------------------------------
// Wraps the Piper TTS engine for offline speech synthesis.  When Piper is not
// linked the class still compiles — synthesize() returns an empty buffer and
// speakText() writes to stdout instead.
//
// Usage:
//     PiperSynthesizer tts;
//     tts.loadVoice("/path/to/voice.onnx", "/path/to/voice.onnx.json");
//     tts.speakText("Hello, world!");
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace atlas::audio {

class PiperSynthesizer {
public:
    PiperSynthesizer();
    ~PiperSynthesizer();

    // Non-copyable.
    PiperSynthesizer(const PiperSynthesizer&) = delete;
    PiperSynthesizer& operator=(const PiperSynthesizer&) = delete;

    /// Load a Piper ONNX voice model and its JSON config.
    /// Returns true on success.
    bool loadVoice(const std::string& modelPath,
                   const std::string& configPath = "");

    /// True if a voice model is loaded.
    bool isLoaded() const;

    /// Synthesize text to 16-bit PCM audio at the voice's native sample rate.
    /// Returns an empty vector if no voice is loaded or Piper is unavailable.
    std::vector<int16_t> synthesize(const std::string& text) const;

    /// Synthesize and immediately play through the default audio output.
    /// Falls back to stdout logging when audio playback is unavailable.
    void speakText(const std::string& text) const;

    /// Output sample rate of the loaded voice (0 if none loaded).
    int sampleRate() const { return sampleRate_; }

private:
    std::string modelPath_;
    int         sampleRate_{0};
    bool        loaded_{false};
};

} // namespace atlas::audio
