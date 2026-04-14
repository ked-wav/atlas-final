#pragma once

#include "calendar/CalendarModule.h"
#include "email/EmailModule.h"
#include "voice/VoicePipeline.h"

#include <string>
#include <vector>

namespace atlas::core {

class ServiceOrchestrator {
public:
    ServiceOrchestrator(voice::VoicePipeline& voicePipeline,
                        email::EmailModule& emailModule,
                        calendar::CalendarModule& calendarModule);

    std::string handleVoiceTurn();
    std::vector<email::EmailMessage> listEmails();
    std::vector<calendar::CalendarEvent> listCalendarEvents();
    void addVoiceCalendarEvent();

private:
    voice::VoicePipeline& voicePipeline_;
    email::EmailModule& emailModule_;
    calendar::CalendarModule& calendarModule_;
};

} // namespace atlas::core
