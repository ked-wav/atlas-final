#pragma once

// ---------------------------------------------------------------------------
// SpeechDetector — lightweight energy-based voice-activity detector (VAD)
// ---------------------------------------------------------------------------
// Analyses short audio frames and decides whether they contain speech.
// Uses a simple RMS-energy threshold with a hangover timer to avoid
// chopping words at brief pauses.
//
// For production environments with noisy backgrounds, plug in Silero-VAD
// (ONNX) instead.  This module provides a good-enough baseline that works
// without additional model files.
//
// Usage:
//     SpeechDetector vad;
//     vad.setOnSpeechStart([&]() { recorder.beginBuffering(); });
//     vad.setOnSpeechEnd([&](std::vector<float> audio) { transcribe(audio); });
//     mic.setCallback([&](const float* d, size_t n) { vad.feed(d, n); });
// ---------------------------------------------------------------------------

#include <cstddef>
#include <functional>
#include <vector>

namespace atlas::audio {

class SpeechDetector {
public:
    /// \p sampleRate      Expected sample rate (Hz).
    /// \p energyThreshold RMS threshold above which a frame is "speech".
    /// \p silenceTimeoutMs How long (ms) of sub-threshold audio before we
    ///                     declare end-of-speech.
    /// \p preSpeechMs      Amount of audio (ms) to keep *before* the trigger
    ///                     so the leading edge of the utterance is not lost.
    explicit SpeechDetector(int    sampleRate       = 16000,
                            float  energyThreshold  = 0.015f,
                            int    silenceTimeoutMs = 600,
                            int    preSpeechMs      = 300);

    // -- Callbacks ----------------------------------------------------------

    /// Fired once when the detector transitions from silence → speech.
    using SpeechStartCb = std::function<void()>;
    void setOnSpeechStart(SpeechStartCb cb);

    /// Fired once when the detector transitions from speech → silence.
    /// Receives the complete captured utterance (including pre-speech buffer).
    using SpeechEndCb = std::function<void(std::vector<float> utterance)>;
    void setOnSpeechEnd(SpeechEndCb cb);

    // -- Processing ---------------------------------------------------------

    /// Feed a chunk of audio samples.  May be any size.
    void feed(const float* samples, std::size_t count);

    /// Reset detector state (e.g. after transcription is complete).
    void reset();

    /// True while the detector considers the current audio "speech".
    bool isSpeaking() const { return speaking_; }

    // -- Configuration ------------------------------------------------------

    void  setEnergyThreshold(float t)  { energyThreshold_ = t; }
    float energyThreshold() const      { return energyThreshold_; }

    void  setSilenceTimeoutMs(int ms)  { silenceTimeoutMs_ = ms; }
    int   silenceTimeoutMs() const     { return silenceTimeoutMs_; }

private:
    int   sampleRate_;
    float energyThreshold_;
    int   silenceTimeoutMs_;
    int   preSpeechMs_;

    bool  speaking_{false};

    /// Running buffer of the current utterance (speech + silence hangover).
    std::vector<float> utteranceBuffer_;

    /// Circular pre-speech buffer — keeps the last \c preSpeechMs_ of audio
    /// so the beginning of the utterance is not clipped.
    std::vector<float> preSpeechBuffer_;
    std::size_t        preSpeechWritePos_{0};
    std::size_t        preSpeechSamplesStored_{0};

    /// Count of consecutive silent samples (below threshold).
    int silentSampleCount_{0};

    SpeechStartCb onSpeechStart_;
    SpeechEndCb   onSpeechEnd_;

    /// Compute RMS energy for a block of samples.
    static float computeRms(const float* data, std::size_t count);
};

} // namespace atlas::audio
