// src/audio/WhisperTranscriber.cpp — whisper.cpp speech-to-text wrapper
// ---------------------------------------------------------------------------
// When ATLAS_HAS_WHISPER is defined the class uses whisper.cpp to
// transcribe 16 kHz mono float32 PCM audio to text.  Otherwise
// transcribe() returns a descriptive placeholder.
// ---------------------------------------------------------------------------

#include "audio/WhisperTranscriber.h"

#include <iostream>

#ifdef ATLAS_HAS_WHISPER
#include "whisper.h"
#endif

namespace atlas::audio {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WhisperTranscriber::WhisperTranscriber() = default;

WhisperTranscriber::~WhisperTranscriber() {
#ifdef ATLAS_HAS_WHISPER
    if (ctx_) {
        whisper_free(static_cast<struct whisper_context*>(ctx_));
        ctx_ = nullptr;
    }
#endif
}

// ---------------------------------------------------------------------------
// Model loading
// ---------------------------------------------------------------------------

bool WhisperTranscriber::loadModel(const std::string& modelPath) {
#ifdef ATLAS_HAS_WHISPER
    if (ctx_) {
        whisper_free(static_cast<struct whisper_context*>(ctx_));
        ctx_ = nullptr;
    }

    struct whisper_context_params cparams = whisper_context_default_params();
    ctx_ = whisper_init_from_file_with_params(modelPath.c_str(), cparams);
    if (!ctx_) {
        std::cerr << "[WhisperTranscriber] Failed to load model: "
                  << modelPath << '\n';
        return false;
    }

    modelPath_ = modelPath;
    std::cout << "[WhisperTranscriber] Model loaded: " << modelPath << '\n';
    return true;
#else
    (void)modelPath;
    std::cout << "[WhisperTranscriber] whisper.cpp not available — "
                 "transcription will use placeholder text.\n";
    modelPath_ = modelPath;
    return false;
#endif
}

bool WhisperTranscriber::isLoaded() const {
#ifdef ATLAS_HAS_WHISPER
    return ctx_ != nullptr;
#else
    return false;
#endif
}

// ---------------------------------------------------------------------------
// Transcription
// ---------------------------------------------------------------------------

std::string WhisperTranscriber::transcribe(const std::vector<float>& pcm16k) const {
    return transcribe(pcm16k.data(), pcm16k.size());
}

std::string WhisperTranscriber::transcribe(const float* samples,
                                           std::size_t count) const {
#ifdef ATLAS_HAS_WHISPER
    if (!ctx_ || !samples || count == 0) {
        return {};
    }

    struct whisper_full_params params =
        whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    // Note: params.language points to language_.c_str(). The whisper_full()
    // call copies the string internally, so this is safe as long as language_
    // is not modified concurrently (the method is const).
    params.language  = language_.c_str();
    params.n_threads = threads_;
    params.print_progress   = false;
    params.print_timestamps = false;
    params.print_special    = false;
    params.no_context       = true;
    params.single_segment   = true;

    auto* wctx = static_cast<struct whisper_context*>(ctx_);
    const int ret = whisper_full(wctx, params, samples,
                                 static_cast<int>(count));
    if (ret != 0) {
        std::cerr << "[WhisperTranscriber] whisper_full failed: " << ret << '\n';
        return {};
    }

    const int nSegments = whisper_full_n_segments(wctx);
    std::string result;
    for (int i = 0; i < nSegments; ++i) {
        const char* text = whisper_full_get_segment_text(wctx, i);
        if (text) {
            if (!result.empty()) {
                result += ' ';
            }
            result += text;
        }
    }

    // Trim leading/trailing whitespace.
    const auto start = result.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = result.find_last_not_of(" \t\n\r");
    return result.substr(start, end - start + 1);

#else
    (void)samples;
    (void)count;
    // Placeholder when whisper.cpp is not linked.
    return "Summarize my latest emails and next calendar event.";
#endif
}

} // namespace atlas::audio
