#pragma once

// ---------------------------------------------------------------------------
// PiperTTS — lightweight C++ wrapper around Piper text-to-speech
// ---------------------------------------------------------------------------
// Provides a simple interface for converting text to speech using the Piper
// TTS engine.  The speak() method is the primary entry point: it accepts a
// UTF-8 string and plays the synthesised audio through the default output
// device.
//
// The class is intentionally minimal so that it can be embedded in any UI
// window without pulling in large dependencies at the header level.
// ---------------------------------------------------------------------------

#ifndef ATLAS_VOICE_PIPERTTS_H
#define ATLAS_VOICE_PIPERTTS_H

#include <string>

namespace atlas::voice {

class PiperTTS {
public:
    /// Construct the TTS engine.  Loads the default voice model.
    PiperTTS();
    ~PiperTTS();

    /// Synthesise \p text and play the audio on the default output device.
    void speak(const std::string& text);

    /// Returns true when the voice model has been loaded successfully.
    bool isReady() const;

private:
    bool ready_{false};
    std::string lastText_;
};

} // namespace atlas::voice

#endif // ATLAS_VOICE_PIPERTTS_H

// ---------------------------------------------------------------------------
// Wiring instructions (for UI integration):
// ---------------------------------------------------------------------------
//  1.  #include "voice/piper_tts.h"           in your .cpp file.
//  2.  Add a member:   PiperTTS* m_tts;       to your window class.
//  3.  In the constructor, create the instance:
//          m_tts = new atlas::voice::PiperTTS();
//  4.  In the destructor, clean up:
//          delete m_tts;
//  5.  Call m_tts->speak(text) to vocalise any assistant response.
// ---------------------------------------------------------------------------
