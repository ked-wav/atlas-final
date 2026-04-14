#pragma once

#include "ai/AIModelClient.h"
#include "audio/VoiceNotifier.h"
#include "calendar/CalendarModule.h"
#include "core/ServiceOrchestrator.h"
#include "email/EmailModule.h"
#include "storage/CredentialStore.h"
#include "voice/VoicePipeline.h"
#include "wake/WakeWordClient.h"
#include "wake/WakeWordManager.h"

#include <string>

namespace atlas::core {

class Application {
public:
    Application();

    int run();
    int runSmokeTest();

private:
    ai::AIModelClient aiClient_;
    audio::VoiceNotifier notifier_;
    email::EmailModule emailModule_;
    calendar::CalendarModule calendarModule_;
    storage::CredentialStore credentialStore_;
    voice::VoicePipeline voicePipeline_;
    wake::WakeWordManager wakeManager_;
    ServiceOrchestrator orchestrator_;

    bool handleCommand(const std::string& command);
};

} // namespace atlas::core
