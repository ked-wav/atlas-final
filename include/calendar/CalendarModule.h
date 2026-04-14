#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace atlas::calendar {

struct CalendarEvent {
    std::string title;
    std::string location;
    std::chrono::system_clock::time_point startsAt;
    std::chrono::system_clock::time_point endsAt;
};

class CalendarModule {
public:
    CalendarModule();

    std::vector<CalendarEvent> getNextSevenDaysEvents() const;
    bool addEvent(const CalendarEvent& event);

private:
    std::vector<CalendarEvent> fallbackEvents_;

    void seedFallbackData();
};

} // namespace atlas::calendar
