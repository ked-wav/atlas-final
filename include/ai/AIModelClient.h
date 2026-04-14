#pragma once

#include <chrono>
#include <functional>
#include <string>

namespace atlas::ai {

struct AIResponseChunk {
    std::string text;
    bool done{false};
};

class AIModelClient {
public:
    enum class Backend {
        LlamaCpp,
        Ollama
    };

    using StreamCallback = std::function<void(const AIResponseChunk&)>;

    explicit AIModelClient(Backend backend = Backend::Ollama);

    std::string prompt(const std::string& input,
                       std::chrono::milliseconds timeout = std::chrono::milliseconds(2500)) const;

    bool promptStream(const std::string& input,
                      const StreamCallback& callback,
                      std::chrono::milliseconds timeout = std::chrono::milliseconds(2500)) const;

private:
    Backend backend_;
    std::string generateResponse(const std::string& input) const;
};

} // namespace atlas::ai
