// src/audio/MicRecorder.cpp — PortAudio-based real-time microphone capture
// ---------------------------------------------------------------------------
// When ATLAS_HAS_PORTAUDIO is defined the class uses PortAudio to stream
// audio from the default input device.  Otherwise start() returns false and
// no audio is captured — the rest of the pipeline can still be exercised
// with synthetic input.
// ---------------------------------------------------------------------------

#include "audio/MicRecorder.h"

#include <iostream>

#ifdef ATLAS_HAS_PORTAUDIO
#include <portaudio.h>
#endif

namespace atlas::audio {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

MicRecorder::MicRecorder(std::size_t frameSize, int sampleRate)
    : frameSize_(frameSize), sampleRate_(sampleRate) {}

MicRecorder::~MicRecorder() {
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void MicRecorder::setCallback(AudioCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    audioCallback_ = std::move(callback);
}

bool MicRecorder::start() {
    if (running_.load()) {
        return false;
    }

#ifdef ATLAS_HAS_PORTAUDIO
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "[MicRecorder] Pa_Initialize failed: "
                  << Pa_GetErrorText(err) << '\n';
        return false;
    }

    PaStreamParameters inputParams{};
    inputParams.device = Pa_GetDefaultInputDevice();
    if (inputParams.device == paNoDevice) {
        std::cerr << "[MicRecorder] No default input device found\n";
        Pa_Terminate();
        return false;
    }
    inputParams.channelCount = 1;
    inputParams.sampleFormat = paFloat32;
    inputParams.suggestedLatency =
        Pa_GetDeviceInfo(inputParams.device)->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        &inputParams,
        nullptr,                             // no output
        static_cast<double>(sampleRate_),
        static_cast<unsigned long>(frameSize_),
        paClipOff,
        &MicRecorder::paCallback,
        this);

    if (err != paNoError) {
        std::cerr << "[MicRecorder] Pa_OpenStream failed: "
                  << Pa_GetErrorText(err) << '\n';
        Pa_Terminate();
        return false;
    }

    err = Pa_StartStream(reinterpret_cast<PaStream*>(paStream_));
    if (err != paNoError) {
        std::cerr << "[MicRecorder] Pa_StartStream failed: "
                  << Pa_GetErrorText(err) << '\n';
        Pa_CloseStream(reinterpret_cast<PaStream*>(paStream_));
        paStream_ = nullptr;
        Pa_Terminate();
        return false;
    }

    running_.store(true);
    std::cout << "[MicRecorder] Capture started (PortAudio, "
              << sampleRate_ << " Hz, frame=" << frameSize_ << ")\n";
    return true;

#else
    // No PortAudio — cannot capture real audio.
    std::cout << "[MicRecorder] PortAudio not available — "
                 "microphone capture disabled.\n";
    return false;
#endif
}

void MicRecorder::stop() {
    if (!running_.load()) {
        return;
    }
    running_.store(false);

#ifdef ATLAS_HAS_PORTAUDIO
    if (paStream_) {
        Pa_StopStream(reinterpret_cast<PaStream*>(paStream_));
        Pa_CloseStream(reinterpret_cast<PaStream*>(paStream_));
        paStream_ = nullptr;
    }
    Pa_Terminate();
    std::cout << "[MicRecorder] Capture stopped\n";
#endif
}

bool MicRecorder::isRunning() const {
    return running_.load();
}

// ---------------------------------------------------------------------------
// PortAudio callback (static)
// ---------------------------------------------------------------------------

int MicRecorder::paCallback(const void* input, void* /*output*/,
                            unsigned long frameCount,
                            const void* /*timeInfo*/,
                            unsigned long /*statusFlags*/,
                            void* userData) {
    auto* self = static_cast<MicRecorder*>(userData);
    if (!self || !input) {
        return 0;  // paContinue
    }

    const auto* samples = static_cast<const float*>(input);

    std::lock_guard<std::mutex> lock(self->callbackMutex_);
    if (self->audioCallback_) {
        self->audioCallback_(samples, static_cast<std::size_t>(frameCount));
    }

    return 0;  // paContinue
}

} // namespace atlas::audio
