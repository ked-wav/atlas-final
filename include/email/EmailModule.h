#pragma once

#include <string>
#include <vector>

namespace atlas::ai {
class AIModelClient;
}

namespace atlas::email {

struct EmailMessage {
    std::string subject;
    std::string sender;
    std::string date;
    std::string preview;
    std::string summary;
};

class EmailModule {
public:
    explicit EmailModule(ai::AIModelClient& aiClient);

    std::vector<EmailMessage> fetchRecentEmails(std::size_t maxCount = 5) const;
    bool sendEmail(const std::string& to, const std::string& subject, const std::string& body) const;

private:
    ai::AIModelClient& aiClient_;

    static EmailMessage parseRawMessage(const std::string& raw);
};

} // namespace atlas::email
