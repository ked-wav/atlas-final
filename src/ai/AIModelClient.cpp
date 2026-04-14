#include "ai/AIModelClient.h"

#include <future>
#include <sstream>
#include <thread>
#include <vector>

namespace atlas::ai {

AIModelClient::AIModelClient(Backend backend) : backend_(backend) {}

std::string AIModelClient::generateResponse(const std::string& input) const {
    const std::string backendName = backend_ == Backend::LlamaCpp ? "llama.cpp" : "Ollama";
    return "[" + backendName + "] I heard: \"" + input + "\". Here's a concise helpful response.";
}

std::string AIModelClient::prompt(const std::string& input, std::chrono::milliseconds timeout) const {
    auto future = std::async(std::launch::async, [this, input]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
        return generateResponse(input);
    });

    if (future.wait_for(timeout) != std::future_status::ready) {
        // Handling LLM delays: UI should show a "thinking" state while this fallback is returned.
        return "I'm still thinking. Let me get back to you in a moment.";
    }
    return future.get();
}

bool AIModelClient::promptStream(const std::string& input,
                                 const StreamCallback& callback,
                                 std::chrono::milliseconds timeout) const {
    const auto started = std::chrono::steady_clock::now();
    std::istringstream stream(generateResponse(input));
    std::string token;
    bool emitted = false;

    while (stream >> token) {
        if (std::chrono::steady_clock::now() - started > timeout) {
            return false;
        }
        callback({token + " ", false});
        emitted = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (!emitted) {
        return false;
    }

    callback({"", true});
    return true;
}

} // namespace atlas::ai
