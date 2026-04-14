#pragma once

#include <string>

namespace atlas::audio {

enum class NotificationPhrase {
    Listening,
    Processing,
    SendingEmail,
    EventAdded
};

class VoiceNotifier {
public:
    void setEnabled(bool enabled);
    bool isEnabled() const;

    void speakPhrase(NotificationPhrase phrase);
    void speakText(const std::string& text);

    const std::string& lastUtterance() const;

private:
    bool enabled_{true};
    std::string lastUtterance_;
};

} // namespace atlas::audio
