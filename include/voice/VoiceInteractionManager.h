#pragma once

// ---------------------------------------------------------------------------
// VoiceInteractionManager — real-time voice interaction orchestrator
// ---------------------------------------------------------------------------
// Wires together:
//   MicRecorder  → SpeechDetector → WhisperTranscriber → AIModelClient → PiperSynthesizer
//
// Provides a high-level API for continuous voice interaction:
//   1. Captures audio from the microphone in real time.
//   2. Detects when the user starts/stops speaking (VAD).
//   3. Transcribes the utterance to text (whisper.cpp).
//   4. Sends the text to the LLM for a response.
//   5. Speaks the response aloud (Piper TTS).
//
// The manager supports two modes:
//   - Single-turn:  one utterance → one response, then stops.
//   - Continuous:   keeps listening for utterances until stopped.
//
// Usage:
//     VoiceInteractionManager vim(aiClient, notifier);
//     vim.loadModels("models/whisper-base.bin", "models/voice.onnx");
//     vim.startContinuous();  // or vim.runSingleTurn();
//     …
//     vim.stop();
// ---------------------------------------------------------------------------

#include "audio/MicRecorder.h"
#include "audio/SpeechDetector.h"
#include "audio/WhisperTranscriber.h"
#include "audio/PiperSynthesizer.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace atlas::ai {
class AIModelClient;
}

namespace atlas::audio {
class VoiceNotifier;
}

namespace atlas::voice {

class VoiceInteractionManager {
public:
    /// Interaction state reported via the state callback.
    enum class State {
        Idle,
        Listening,
        Detecting,
        Transcribing,
        Thinking,
        Speaking
    };

    VoiceInteractionManager(ai::AIModelClient& aiClient,
                            audio::VoiceNotifier& notifier);

    ~VoiceInteractionManager();

    // Non-copyable.
    VoiceInteractionManager(const VoiceInteractionManager&) = delete;
    VoiceInteractionManager& operator=(const VoiceInteractionManager&) = delete;

    // -- Model loading ------------------------------------------------------

    /// Load the whisper model for speech-to-text.
    bool loadWhisperModel(const std::string& path);

    /// Load the Piper TTS voice model.
    bool loadPiperVoice(const std::string& modelPath,
                        const std::string& configPath = "");

    // -- Interaction --------------------------------------------------------

    /// Run a single voice turn: listen → transcribe → LLM → speak.
    /// Blocks until the full cycle completes or timeout.
    /// Returns the LLM response text.
    std::string runSingleTurn(int timeoutMs = 15000);

    /// Start continuous voice interaction in a background thread.
    void startContinuous();

    /// Stop any active interaction.
    void stop();

    /// True if the manager is actively listening/processing.
    bool isActive() const;

    /// Current interaction state.
    State state() const;

    // -- Callbacks ----------------------------------------------------------

    /// Called whenever the interaction state changes.
    using StateCallback = std::function<void(State)>;
    void setStateCallback(StateCallback cb);

    /// Called with the transcript of each utterance.
    using TranscriptCallback = std::function<void(const std::string&)>;
    void setTranscriptCallback(TranscriptCallback cb);

    /// Called with the LLM response for each turn.
    using ResponseCallback = std::function<void(const std::string&)>;
    void setResponseCallback(ResponseCallback cb);

    // -- Configuration ------------------------------------------------------

    /// Set the VAD energy threshold (default 0.015).
    void setVadThreshold(float t);

    /// Set the silence timeout in ms (default 600).
    void setSilenceTimeoutMs(int ms);

private:
    ai::AIModelClient&   aiClient_;
    audio::VoiceNotifier& notifier_;

    audio::MicRecorder       mic_;
    audio::SpeechDetector    vad_;
    audio::WhisperTranscriber whisper_;
    audio::PiperSynthesizer  tts_;

    std::atomic<bool>  active_{false};
    std::atomic<State> state_{State::Idle};

    std::thread        interactionThread_;
    std::mutex         mutex_;
    std::condition_variable cv_;

    // Pending utterance delivered by the VAD callback.
    std::vector<float> pendingUtterance_;
    bool               utteranceReady_{false};

    StateCallback      stateCallback_;
    TranscriptCallback transcriptCallback_;
    ResponseCallback   responseCallback_;

    void setState(State s);

    /// Process one complete voice turn (called on the interaction thread).
    std::string processOneTurn(int timeoutMs);

    /// Continuous interaction loop.
    void continuousLoop();

    /// Handle a completed utterance from the VAD.
    void onUtteranceReady(std::vector<float> utterance);
};

} // namespace atlas::voice
