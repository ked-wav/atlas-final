#include "calendar/CalendarModule.h"

namespace atlas::calendar {

CalendarModule::CalendarModule() {
    seedFallbackData();
}

void CalendarModule::seedFallbackData() {
    using namespace std::chrono;

    // SQLite fallback schema (portable baseline):
    // CREATE TABLE events(id INTEGER PRIMARY KEY, title TEXT, location TEXT, starts_at TEXT, ends_at TEXT);
    fallbackEvents_.push_back(
        {"Team Standup", "Remote", system_clock::now() + hours(24), system_clock::now() + hours(25)});
}

std::vector<CalendarEvent> CalendarModule::getNextSevenDaysEvents() const {
    // CalDAV fetch via libcurl + ICS parsing via libical should occur here.
    // CalDAV providers vary in timezone handling and recurrence expansion behavior.
    const auto now = std::chrono::system_clock::now();
    const auto horizon = now + std::chrono::hours(24 * 7);

    std::vector<CalendarEvent> events;
    for (const auto& event : fallbackEvents_) {
        if (event.startsAt >= now && event.startsAt <= horizon) {
            events.push_back(event);
        }
    }
    return events;
}

bool CalendarModule::addEvent(const CalendarEvent& event) {
    fallbackEvents_.push_back(event);
    return true;
}

} // namespace atlas::calendar
