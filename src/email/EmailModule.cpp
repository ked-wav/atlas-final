#include "email/EmailModule.h"

#include "ai/AIModelClient.h"

#include <sstream>
#include <utility>

namespace atlas::email {

EmailModule::EmailModule(ai::AIModelClient& aiClient) : aiClient_(aiClient) {}

EmailMessage EmailModule::parseRawMessage(const std::string& raw) {
    EmailMessage message;
    std::istringstream lines(raw);
    std::string line;

    while (std::getline(lines, line)) {
        if (line.rfind("Subject:", 0) == 0) {
            message.subject = line.substr(8);
        } else if (line.rfind("From:", 0) == 0) {
            message.sender = line.substr(5);
        } else if (line.rfind("Date:", 0) == 0) {
            message.date = line.substr(5);
        } else if (!line.empty() && message.preview.empty()) {
            message.preview = line;
        }
    }

    return message;
}

std::vector<EmailMessage> EmailModule::fetchRecentEmails(std::size_t maxCount) const {
    // IMAP fetch using libcurl should be implemented here.
    std::vector<std::string> raw = {
        "Subject: Project Update\nFrom: team@example.com\nDate: 2026-04-14\nSprint review moved to 3 PM.",
        "Subject: Family Photos\nFrom: aunt@example.com\nDate: 2026-04-13\nSending you the latest pictures."
    };

    std::vector<EmailMessage> messages;
    for (const auto& item : raw) {
        if (messages.size() >= maxCount) {
            break;
        }
        auto parsed = parseRawMessage(item);
        parsed.summary = aiClient_.prompt("Summarize this email: " + parsed.preview);
        messages.push_back(std::move(parsed));
    }
    return messages;
}

bool EmailModule::sendEmail(const std::string& to, const std::string& subject, const std::string& body) const {
    (void)to;
    (void)subject;
    (void)body;
    // SMTP send using libcurl should be implemented here.
    return true;
}

} // namespace atlas::email
