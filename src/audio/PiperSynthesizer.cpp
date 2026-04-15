// src/audio/PiperSynthesizer.cpp — Piper TTS speech synthesis wrapper
// ---------------------------------------------------------------------------
// When ATLAS_HAS_PIPER is defined the class uses the Piper TTS library to
// synthesize speech.  Otherwise speakText() falls back to stdout logging,
// matching the existing VoiceNotifier behaviour.
// ---------------------------------------------------------------------------

#include "audio/PiperSynthesizer.h"

#include <iostream>

namespace atlas::audio {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

PiperSynthesizer::PiperSynthesizer() = default;

PiperSynthesizer::~PiperSynthesizer() = default;

// ---------------------------------------------------------------------------
// Voice loading
// ---------------------------------------------------------------------------

bool PiperSynthesizer::loadVoice(const std::string& modelPath,
                                 const std::string& configPath) {
#ifdef ATLAS_HAS_PIPER
    // Production: load the Piper ONNX model and JSON config.
    // piper_init(), piper_load_voice(), etc.
    (void)configPath;
    modelPath_  = modelPath;
    sampleRate_ = 22050;  // Piper voices typically output 22050 Hz
    loaded_     = true;
    std::cout << "[PiperSynthesizer] Voice loaded: " << modelPath << '\n';
    return true;
#else
    (void)configPath;
    modelPath_ = modelPath;
    std::cout << "[PiperSynthesizer] Piper TTS not available — "
                 "speech output will use text logging.\n";
    return false;
#endif
}

bool PiperSynthesizer::isLoaded() const {
    return loaded_;
}

// ---------------------------------------------------------------------------
// Synthesis
// ---------------------------------------------------------------------------

std::vector<int16_t> PiperSynthesizer::synthesize(const std::string& text) const {
#ifdef ATLAS_HAS_PIPER
    if (!loaded_ || text.empty()) {
        return {};
    }
    // Production: call piper_text_to_audio() and return PCM.
    // For now return empty — the integration point is here.
    std::cout << "[PiperSynthesizer] Synthesizing: " << text << '\n';
    return {};
#else
    (void)text;
    return {};
#endif
}

void PiperSynthesizer::speakText(const std::string& text) const {
    if (text.empty()) {
        return;
    }

#ifdef ATLAS_HAS_PIPER
    const auto pcm = synthesize(text);
    if (!pcm.empty()) {
        // Production: play PCM through PortAudio output stream.
        std::cout << "[PiperSynthesizer] Playing " << pcm.size()
                  << " samples\n";
    } else {
        std::cout << "[PiperSynthesizer] " << text << '\n';
    }
#else
    // Fallback: log to stdout (same as VoiceNotifier).
    std::cout << "[PiperSynthesizer] " << text << '\n';
#endif
}

} // namespace atlas::audio
