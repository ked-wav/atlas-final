#include "core/ServiceOrchestrator.h"

#include <chrono>

namespace atlas::core {

ServiceOrchestrator::ServiceOrchestrator(voice::VoicePipeline& voicePipeline,
                                         email::EmailModule& emailModule,
                                         calendar::CalendarModule& calendarModule)
    : voicePipeline_(voicePipeline), emailModule_(emailModule), calendarModule_(calendarModule) {}

std::string ServiceOrchestrator::handleVoiceTurn() {
    return voicePipeline_.runSingleTurn();
}

std::vector<email::EmailMessage> ServiceOrchestrator::listEmails() {
    return emailModule_.fetchRecentEmails();
}

std::vector<calendar::CalendarEvent> ServiceOrchestrator::listCalendarEvents() {
    return calendarModule_.getNextSevenDaysEvents();
}

void ServiceOrchestrator::addVoiceCalendarEvent() {
    using namespace std::chrono;
    calendarModule_.addEvent({"Voice Added Event", "Home", system_clock::now() + hours(2), system_clock::now() + hours(3)});
}

} // namespace atlas::core
