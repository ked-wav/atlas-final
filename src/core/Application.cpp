#include "core/Application.h"

#include <chrono>
#include <ctime>
#include <iostream>
#include <string>

namespace atlas::core {

Application::Application()
    : aiClient_(ai::AIModelClient::Backend::Ollama),
      emailModule_(aiClient_),
      voicePipeline_(aiClient_, notifier_),
      orchestrator_(voicePipeline_, emailModule_, calendarModule_) {}

bool Application::handleCommand(const std::string& command) {
    if (command == "talk") {
        const auto response = orchestrator_.handleVoiceTurn();
        std::cout << "AI: " << response << '\n';
        return true;
    }

    if (command == "email") {
        const auto emails = orchestrator_.listEmails();
        for (const auto& email : emails) {
            std::cout << "Email from " << email.sender << " | Subject:" << email.subject
                      << " | Date:" << email.date << "\nSummary: " << email.summary << "\n";
        }
        notifier_.speakPhrase(audio::NotificationPhrase::SendingEmail);
        return true;
    }

    if (command == "calendar") {
        const auto events = orchestrator_.listCalendarEvents();
        for (const auto& event : events) {
            const std::time_t startsAt = std::chrono::system_clock::to_time_t(event.startsAt);
            std::cout << "Event: " << event.title << " at " << event.location << " on "
                      << std::ctime(&startsAt);
        }
        orchestrator_.addVoiceCalendarEvent();
        notifier_.speakPhrase(audio::NotificationPhrase::EventAdded);
        return true;
    }

    if (command == "quit") {
        return false;
    }

    std::cout << "Unknown command. Use: talk | email | calendar | quit\n";
    return true;
}

int Application::run() {
    std::cout << "Atlas Assistant started. Commands: talk, email, calendar, quit\n";
    std::string command;
    bool keepRunning = true;

    while (keepRunning && std::getline(std::cin, command)) {
        keepRunning = handleCommand(command);
    }

    return 0;
}

int Application::runSmokeTest() {
    notifier_.setEnabled(false);

    const bool one = handleCommand("talk");
    const bool two = handleCommand("email");
    const bool three = handleCommand("calendar");
    const bool four = handleCommand("quit");

    return (one && two && three && !four) ? 0 : 1;
}

} // namespace atlas::core
