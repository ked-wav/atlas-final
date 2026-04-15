#include "voice/piper_tts.h"

#include <iostream>

namespace atlas::voice {

PiperTTS::PiperTTS() {
    // Piper model loading would happen here (e.g. loading .onnx voice model).
    ready_ = true;
    std::cout << "[PiperTTS] Engine initialised.\n";
}

PiperTTS::~PiperTTS() {
    std::cout << "[PiperTTS] Engine shut down.\n";
}

void PiperTTS::speak(const std::string& text) {
    lastText_ = text;
    if (!ready_) {
        std::cerr << "[PiperTTS] Engine not ready — cannot speak.\n";
        return;
    }
    // Piper inference + audio playback would happen here.
    std::cout << "[PiperTTS] Speaking: " << text << '\n';
}

bool PiperTTS::isReady() const {
    return ready_;
}

} // namespace atlas::voice
