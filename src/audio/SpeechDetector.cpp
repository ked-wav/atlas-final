// src/audio/SpeechDetector.cpp — energy-based voice-activity detector
// ---------------------------------------------------------------------------
// Uses RMS energy with a hangover timer.  For noisy environments replace
// with Silero-VAD (ONNX) for significantly better accuracy.
// ---------------------------------------------------------------------------

#include "audio/SpeechDetector.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace atlas::audio {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

SpeechDetector::SpeechDetector(int sampleRate, float energyThreshold,
                               int silenceTimeoutMs, int preSpeechMs)
    : sampleRate_(sampleRate),
      energyThreshold_(energyThreshold),
      silenceTimeoutMs_(silenceTimeoutMs),
      preSpeechMs_(preSpeechMs) {
    // Pre-allocate the circular pre-speech buffer.
    const std::size_t preSpeechSamples =
        static_cast<std::size_t>(sampleRate_) * preSpeechMs_ / 1000;
    preSpeechBuffer_.resize(preSpeechSamples, 0.0f);
}

// ---------------------------------------------------------------------------
// Callback registration
// ---------------------------------------------------------------------------

void SpeechDetector::setOnSpeechStart(SpeechStartCb cb) {
    onSpeechStart_ = std::move(cb);
}

void SpeechDetector::setOnSpeechEnd(SpeechEndCb cb) {
    onSpeechEnd_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// Core processing
// ---------------------------------------------------------------------------

void SpeechDetector::feed(const float* samples, std::size_t count) {
    if (!samples || count == 0) {
        return;
    }

    const float rms = computeRms(samples, count);
    const bool  frameHasSpeech = (rms >= energyThreshold_);

    if (!speaking_) {
        // ---- Silence state ----
        // Always update the circular pre-speech buffer.
        for (std::size_t i = 0; i < count; ++i) {
            if (!preSpeechBuffer_.empty()) {
                preSpeechBuffer_[preSpeechWritePos_] = samples[i];
                preSpeechWritePos_ =
                    (preSpeechWritePos_ + 1) % preSpeechBuffer_.size();
                if (preSpeechSamplesStored_ < preSpeechBuffer_.size()) {
                    ++preSpeechSamplesStored_;
                }
            }
        }

        if (frameHasSpeech) {
            speaking_ = true;
            silentSampleCount_ = 0;

            // Flush pre-speech buffer into utterance.
            utteranceBuffer_.clear();
            if (preSpeechSamplesStored_ > 0 && !preSpeechBuffer_.empty()) {
                // Read from the circular buffer in order.
                const std::size_t bufSize = preSpeechBuffer_.size();
                std::size_t readStart = 0;
                if (preSpeechSamplesStored_ >= bufSize) {
                    readStart = preSpeechWritePos_;  // oldest sample
                } else {
                    readStart = (preSpeechWritePos_ + bufSize -
                                 preSpeechSamplesStored_) % bufSize;
                }
                utteranceBuffer_.reserve(preSpeechSamplesStored_ + count);
                for (std::size_t i = 0; i < preSpeechSamplesStored_; ++i) {
                    utteranceBuffer_.push_back(
                        preSpeechBuffer_[(readStart + i) % bufSize]);
                }
            }

            // Append current frame.
            utteranceBuffer_.insert(utteranceBuffer_.end(),
                                    samples, samples + count);

            std::cout << "[SpeechDetector] Speech started\n";
            if (onSpeechStart_) {
                onSpeechStart_();
            }
        }
    } else {
        // ---- Speech state ----
        // Always append audio.
        utteranceBuffer_.insert(utteranceBuffer_.end(),
                                samples, samples + count);

        if (!frameHasSpeech) {
            silentSampleCount_ += static_cast<int>(count);
            const int silentMs = silentSampleCount_ * 1000 / sampleRate_;

            if (silentMs >= silenceTimeoutMs_) {
                // End of speech detected.
                speaking_ = false;
                silentSampleCount_ = 0;
                preSpeechSamplesStored_ = 0;
                preSpeechWritePos_ = 0;

                std::cout << "[SpeechDetector] Speech ended ("
                          << utteranceBuffer_.size() << " samples, "
                          << utteranceBuffer_.size() * 1000 / sampleRate_
                          << " ms)\n";

                if (onSpeechEnd_) {
                    onSpeechEnd_(std::move(utteranceBuffer_));
                }
                utteranceBuffer_.clear();
            }
        } else {
            // Reset silence counter — speech is continuing.
            silentSampleCount_ = 0;
        }
    }
}

void SpeechDetector::reset() {
    speaking_ = false;
    silentSampleCount_ = 0;
    utteranceBuffer_.clear();
    preSpeechSamplesStored_ = 0;
    preSpeechWritePos_ = 0;
    std::fill(preSpeechBuffer_.begin(), preSpeechBuffer_.end(), 0.0f);
}

// ---------------------------------------------------------------------------
// RMS energy computation
// ---------------------------------------------------------------------------

float SpeechDetector::computeRms(const float* data, std::size_t count) {
    if (count == 0) {
        return 0.0f;
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < count; ++i) {
        const double s = static_cast<double>(data[i]);
        sum += s * s;
    }
    return static_cast<float>(std::sqrt(sum / static_cast<double>(count)));
}

} // namespace atlas::audio
