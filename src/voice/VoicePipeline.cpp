#include "voice/VoicePipeline.h"

#include "ai/AIModelClient.h"
#include "audio/VoiceNotifier.h"
#include "audio/SpeechDetector.h"
#include "audio/WhisperTranscriber.h"

#include <cmath>

namespace atlas::voice {

VoicePipeline::VoicePipeline(ai::AIModelClient& aiClient, audio::VoiceNotifier& notifier)
    : aiClient_(aiClient), notifier_(notifier) {}

std::vector<float> VoicePipeline::recordMicrophoneAudio() const {
    // PortAudio integration should capture live microphone samples.
    // For real-time capture use VoiceInteractionManager which uses MicRecorder.
    std::vector<float> synthetic(16000);
    for (std::size_t i = 0; i < synthetic.size(); ++i) {
        synthetic[i] = 0.2f * std::sin(static_cast<float>(i) * 0.02f);
    }
    return synthetic;
}

std::vector<SpeechSegment> VoicePipeline::detectSpeechSegments(const std::vector<float>& audioBuffer) const {
    // For real VAD use the SpeechDetector class which provides energy-based
    // detection with pre-speech buffering and configurable thresholds.
    if (audioBuffer.empty()) {
        return {};
    }
    return {SpeechSegment{audioBuffer, 16000}};
}

std::string VoicePipeline::transcribe(const SpeechSegment& segment) const {
    // For real transcription use WhisperTranscriber with a loaded model.
    // This placeholder keeps the smoke test working without whisper.cpp.
    audio::WhisperTranscriber transcriber;
    return transcriber.transcribe(segment.samples);
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
