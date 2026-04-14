#include "voice/VoicePipeline.h"

#include "ai/AIModelClient.h"
#include "audio/VoiceNotifier.h"

#include <cmath>

namespace atlas::voice {

VoicePipeline::VoicePipeline(ai::AIModelClient& aiClient, audio::VoiceNotifier& notifier)
    : aiClient_(aiClient), notifier_(notifier) {}

std::vector<float> VoicePipeline::recordMicrophoneAudio() const {
    // PortAudio integration should capture live microphone samples.
    // Mobile microphone permissions differ across Qt platforms and need runtime handling.
    std::vector<float> synthetic(16000);
    for (std::size_t i = 0; i < synthetic.size(); ++i) {
        synthetic[i] = 0.2f * std::sin(static_cast<float>(i) * 0.02f);
    }
    return synthetic;
}

std::vector<SpeechSegment> VoicePipeline::detectSpeechSegments(const std::vector<float>& audioBuffer) const {
    // Silero VAD threshold tuning is required per microphone/environment profile.
    if (audioBuffer.empty()) {
        return {};
    }
    return {SpeechSegment{audioBuffer, 16000}};
}

std::string VoicePipeline::transcribe(const SpeechSegment& segment) const {
    (void)segment;
    // whisper.cpp integration point. Quantized models reduce latency on edge devices.
    return "Summarize my latest emails and next calendar event.";
}

void VoicePipeline::playResponse(const std::string& response) const {
    // Piper TTS should synthesize response and stream to audio output device.
    notifier_.speakText(response);
}

std::string VoicePipeline::runSingleTurn() {
    notifier_.speakPhrase(audio::NotificationPhrase::Listening);
    const auto audio = recordMicrophoneAudio();
    const auto segments = detectSpeechSegments(audio);
    if (segments.empty()) {
        return "I didn't catch that.";
    }

    notifier_.speakPhrase(audio::NotificationPhrase::Processing);
    const auto transcript = transcribe(segments.front());
    const auto response = aiClient_.prompt(transcript);
    playResponse(response);
    return response;
}

} // namespace atlas::voice
