#include "audio/VoiceNotifier.h"

#include <iostream>

namespace atlas::audio {

namespace {
std::string toText(NotificationPhrase phrase) {
    switch (phrase) {
        case NotificationPhrase::Listening:
            return "Listening";
        case NotificationPhrase::Processing:
            return "Processing";
        case NotificationPhrase::SendingEmail:
            return "Sending your email";
        case NotificationPhrase::EventAdded:
            return "Event added";
        default:
            return "Processing";
    }
}
} // namespace

void VoiceNotifier::setEnabled(bool enabled) {
    enabled_ = enabled;
}

bool VoiceNotifier::isEnabled() const {
    return enabled_;
}

void VoiceNotifier::speakPhrase(NotificationPhrase phrase) {
    speakText(toText(phrase));
}

void VoiceNotifier::speakText(const std::string& text) {
    lastUtterance_ = text;
    if (enabled_) {
        // Piper TTS integration point: invoke Piper with text and play generated audio.
        std::cout << "[VoiceNotifier] " << text << '\n';
    }
}

const std::string& VoiceNotifier::lastUtterance() const {
    return lastUtterance_;
}

} // namespace atlas::audio
