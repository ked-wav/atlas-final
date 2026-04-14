#pragma once

// ---------------------------------------------------------------------------
// WakeWordClient — TCP socket client for the Python wake-word service
// ---------------------------------------------------------------------------
// Connects to the Python openWakeWord service at 127.0.0.1:5055 and listens
// for "WAKE\n" messages.  Runs on its own thread, auto-reconnects on
// connection loss, and handles partial TCP reads with an internal line buffer.
//
// This is the *fallback* integration path.  For the preferred pure-C++ ONNX
// approach see the migration notes in wakeword/wakeword_service.py and the
// comments at the end of WakeWordClient.cpp.
// ---------------------------------------------------------------------------

#include <atomic>
#include <functional>
#include <string>
#include <thread>

namespace atlas::wake {

class WakeWordClient {
public:
    /// Construct a client that will connect to \p host:\p port.
    explicit WakeWordClient(const std::string& host = "127.0.0.1", int port = 5055);

    ~WakeWordClient();

    // Non-copyable, non-movable (owns a thread).
    WakeWordClient(const WakeWordClient&) = delete;
    WakeWordClient& operator=(const WakeWordClient&) = delete;

    /// Register the callback invoked when "WAKE" is received.
    void setCallback(std::function<void()> callback);

    /// Start the background listener thread.
    void start();

    /// Request the background thread to stop and join it.
    void stop();

    /// Returns true while the listener thread is supposed to be running.
    bool isRunning() const;

private:
    std::string host_;
    int port_;

    std::atomic<bool> running_{false};
    std::thread listenerThread_;
    std::function<void()> wakeCallback_;

    /// Main loop executed on the listener thread.
    void listenerLoop();

    /// Attempt to connect a TCP socket.  Returns the fd or -1.
    int tryConnect() const;
};

} // namespace atlas::wake
