#pragma once

#include <string>
#include <vector>

namespace atlas::ai {
class AIModelClient;
}

namespace atlas::audio {
class VoiceNotifier;
}

namespace atlas::voice {

struct SpeechSegment {
    std::vector<float> samples;
    int sampleRate{16000};
};

class VoicePipeline {
public:
    VoicePipeline(ai::AIModelClient& aiClient, audio::VoiceNotifier& notifier);

    std::string runSingleTurn();

private:
    ai::AIModelClient& aiClient_;
    audio::VoiceNotifier& notifier_;

    std::vector<float> recordMicrophoneAudio() const;
    std::vector<SpeechSegment> detectSpeechSegments(const std::vector<float>& audioBuffer) const;
    std::string transcribe(const SpeechSegment& segment) const;
    void playResponse(const std::string& response) const;
};

} // namespace atlas::voice
