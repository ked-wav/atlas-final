#include "wake/WakeWordClient.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <iostream>

// POSIX socket headers
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

WakeWordClient::WakeWordClient(const std::string& host, int port)
    : host_(host), port_(port) {}

WakeWordClient::~WakeWordClient() {
    stop();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void WakeWordClient::setCallback(std::function<void()> callback) {
    wakeCallback_ = std::move(callback);
}

void WakeWordClient::start() {
    if (running_.load()) {
        return;
    }
    running_.store(true);
    listenerThread_ = std::thread(&WakeWordClient::listenerLoop, this);
}

void WakeWordClient::stop() {
    running_.store(false);
    if (listenerThread_.joinable()) {
        listenerThread_.join();
    }
}

bool WakeWordClient::isRunning() const {
    return running_.load();
}

// ---------------------------------------------------------------------------
// Networking helpers
// ---------------------------------------------------------------------------

int WakeWordClient::tryConnect() const {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    struct sockaddr_in addr {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (::inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0) {
        ::close(fd);
        return -1;
    }

    // Use a non-blocking connect with a short timeout so that the listener
    // loop can check the running_ flag regularly.
    struct timeval tv {};
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        ::close(fd);
        return -1;
    }

    return fd;
}

// ---------------------------------------------------------------------------
// Listener loop (runs on its own thread)
// ---------------------------------------------------------------------------

void WakeWordClient::listenerLoop() {
    constexpr int kReconnectDelaySec = 2;
    constexpr std::size_t kBufSize = 256;

    while (running_.load()) {
        // ----- Connect (with auto-reconnect) -----
        int fd = tryConnect();
        if (fd < 0) {
            std::cerr << "[WakeWordClient] Connection to " << host_ << ":"
                      << port_ << " failed — retrying in "
                      << kReconnectDelaySec << "s\n";
            // Sleep in small increments so we can exit quickly when stopped.
            for (int i = 0; i < kReconnectDelaySec * 10 && running_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            continue;
        }

        std::cout << "[WakeWordClient] Connected to " << host_ << ":" << port_ << "\n";

        // ----- Read loop with partial-read / line buffering -----
        // TCP is a stream protocol — a single recv() may return a partial
        // line or multiple lines.  We accumulate data in `lineBuf` and
        // extract complete newline-terminated messages.
        std::string lineBuf;
        char raw[kBufSize];

        while (running_.load()) {
            ssize_t n = ::recv(fd, raw, kBufSize, 0);

            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // Receive timeout — just loop and check running_.
                    continue;
                }
                std::cerr << "[WakeWordClient] recv error: " << std::strerror(errno) << "\n";
                break;  // reconnect
            }

            if (n == 0) {
                // Server closed the connection.
                std::cerr << "[WakeWordClient] Server disconnected\n";
                break;  // reconnect
            }

            // Append received data to the line buffer.
            lineBuf.append(raw, static_cast<std::size_t>(n));

            // Process all complete lines.
            std::string::size_type pos;
            while ((pos = lineBuf.find('\n')) != std::string::npos) {
                std::string line = lineBuf.substr(0, pos);
                lineBuf.erase(0, pos + 1);

                // Strip trailing '\r' if present.
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }

                if (line == "WAKE") {
                    if (wakeCallback_) {
                        wakeCallback_();
                    }
                }
            }
        }

        ::close(fd);

        // Brief pause before reconnecting.
        if (running_.load()) {
            std::cerr << "[WakeWordClient] Will reconnect in "
                      << kReconnectDelaySec << "s …\n";
            for (int i = 0; i < kReconnectDelaySec * 10 && running_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

} // namespace atlas::wake

// ===========================================================================
// ONNX C++ Migration Notes
// ===========================================================================
//
// When migrating to the pure-C++ ONNX approach, this entire class becomes
// unnecessary.  Replace it with an OnnxWakeWordDetector that:
//
//   1. Loads the atlas.onnx model via Ort::Session.
//   2. Captures audio via PortAudio (already stubbed in VoicePipeline).
//   3. Runs inference on each 80 ms chunk in the audio callback.
//   4. Fires the wake callback when confidence > threshold.
//
// WakeWordManager's interface stays the same — only the internal
// implementation changes from "TCP client" to "ONNX detector".
// ===========================================================================
