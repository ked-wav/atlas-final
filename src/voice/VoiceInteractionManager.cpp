// src/voice/VoiceInteractionManager.cpp — real-time voice interaction
// ---------------------------------------------------------------------------
// Orchestrates the full voice loop:
//   MicRecorder → SpeechDetector → WhisperTranscriber → AIModelClient → TTS
//
// Supports single-turn and continuous modes.  All heavy processing runs on
// a dedicated interaction thread to keep the audio callback fast.
// ---------------------------------------------------------------------------

#include "voice/VoiceInteractionManager.h"

#include "ai/AIModelClient.h"
#include "audio/VoiceNotifier.h"

#include <iostream>

namespace atlas::voice {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

VoiceInteractionManager::VoiceInteractionManager(
    ai::AIModelClient& aiClient,
    audio::VoiceNotifier& notifier)
    : aiClient_(aiClient),
      notifier_(notifier),
      mic_(1024, 16000),
      vad_(16000, 0.015f, 600, 300) {
    // Wire the VAD → interaction thread pipeline.
    vad_.setOnSpeechStart([this]() {
        setState(State::Detecting);
    });

    vad_.setOnSpeechEnd([this](std::vector<float> utterance) {
        onUtteranceReady(std::move(utterance));
    });

    // Wire the microphone → VAD.
    mic_.setCallback([this](const float* samples, std::size_t count) {
        if (active_.load()) {
            vad_.feed(samples, count);
        }
    });
}

VoiceInteractionManager::~VoiceInteractionManager() {
    stop();
}

// ---------------------------------------------------------------------------
// Model loading
// ---------------------------------------------------------------------------

bool VoiceInteractionManager::loadWhisperModel(const std::string& path) {
    return whisper_.loadModel(path);
}

bool VoiceInteractionManager::loadPiperVoice(const std::string& modelPath,
                                              const std::string& configPath) {
    return tts_.loadVoice(modelPath, configPath);
}

// ---------------------------------------------------------------------------
// Single-turn interaction
// ---------------------------------------------------------------------------

std::string VoiceInteractionManager::runSingleTurn(int timeoutMs) {
    if (active_.load()) {
        return "Already processing a voice interaction.";
    }

    active_.store(true);
    vad_.reset();
    utteranceReady_ = false;
    setState(State::Listening);

    // Try to start the microphone.  If PortAudio is unavailable, fall back
    // to the existing VoicePipeline synthetic path.
    const bool micStarted = mic_.start();
    if (!micStarted) {
        // Fallback: use placeholder transcription.
        setState(State::Transcribing);
        const std::string transcript = whisper_.transcribe(nullptr, 0);

        setState(State::Thinking);
        notifier_.speakPhrase(audio::NotificationPhrase::Processing);
        const std::string response = aiClient_.prompt(transcript);

        setState(State::Speaking);
        tts_.speakText(response);
        notifier_.speakText(response);

        active_.store(false);
        setState(State::Idle);
        return response;
    }

    notifier_.speakPhrase(audio::NotificationPhrase::Listening);
    const std::string result = processOneTurn(timeoutMs);

    mic_.stop();
    active_.store(false);
    setState(State::Idle);
    return result;
}

// ---------------------------------------------------------------------------
// Continuous interaction
// ---------------------------------------------------------------------------

void VoiceInteractionManager::startContinuous() {
    if (active_.load()) {
        return;
    }

    active_.store(true);
    vad_.reset();
    utteranceReady_ = false;

    const bool micStarted = mic_.start();
    if (!micStarted) {
        std::cout << "[VoiceInteractionManager] Mic unavailable — "
                     "continuous mode not started.\n";
        active_.store(false);
        return;
    }

    setState(State::Listening);
    notifier_.speakPhrase(audio::NotificationPhrase::Listening);

    interactionThread_ = std::thread(&VoiceInteractionManager::continuousLoop,
                                     this);
    std::cout << "[VoiceInteractionManager] Continuous mode started\n";
}

void VoiceInteractionManager::stop() {
    if (!active_.load()) {
        return;
    }
    active_.store(false);

    // Wake up the interaction thread if it's waiting.
    {
        std::lock_guard<std::mutex> lock(mutex_);
        utteranceReady_ = true;
    }
    cv_.notify_all();

    if (interactionThread_.joinable()) {
        interactionThread_.join();
    }

    mic_.stop();
    vad_.reset();
    setState(State::Idle);
    std::cout << "[VoiceInteractionManager] Stopped\n";
}

bool VoiceInteractionManager::isActive() const {
    return active_.load();
}

VoiceInteractionManager::State VoiceInteractionManager::state() const {
    return state_.load();
}

// ---------------------------------------------------------------------------
// Callbacks
// ---------------------------------------------------------------------------

void VoiceInteractionManager::setStateCallback(StateCallback cb) {
    stateCallback_ = std::move(cb);
}

void VoiceInteractionManager::setTranscriptCallback(TranscriptCallback cb) {
    transcriptCallback_ = std::move(cb);
}

void VoiceInteractionManager::setResponseCallback(ResponseCallback cb) {
    responseCallback_ = std::move(cb);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void VoiceInteractionManager::setVadThreshold(float t) {
    vad_.setEnergyThreshold(t);
}

void VoiceInteractionManager::setSilenceTimeoutMs(int ms) {
    vad_.setSilenceTimeoutMs(ms);
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void VoiceInteractionManager::setState(State s) {
    state_.store(s);
    if (stateCallback_) {
        stateCallback_(s);
    }
}

std::string VoiceInteractionManager::processOneTurn(int timeoutMs) {
    // Wait for the VAD to deliver a complete utterance.
    {
        std::unique_lock<std::mutex> lock(mutex_);
        const bool gotUtterance = cv_.wait_for(
            lock,
            std::chrono::milliseconds(timeoutMs),
            [this]() { return utteranceReady_; });

        if (!gotUtterance || pendingUtterance_.empty()) {
            return "I didn't catch that. Could you try again?";
        }
    }

    // Transcribe.
    setState(State::Transcribing);
    const std::string transcript = whisper_.transcribe(pendingUtterance_);

    std::cout << "[VoiceInteractionManager] Transcript: " << transcript << '\n';
    if (transcriptCallback_) {
        transcriptCallback_(transcript);
    }

    if (transcript.empty()) {
        return "I couldn't understand that. Could you repeat?";
    }

    // Send to LLM.
    setState(State::Thinking);
    notifier_.speakPhrase(audio::NotificationPhrase::Processing);
    const std::string response = aiClient_.prompt(transcript);

    std::cout << "[VoiceInteractionManager] Response: " << response << '\n';
    if (responseCallback_) {
        responseCallback_(response);
    }

    // Speak the response.
    setState(State::Speaking);
    tts_.speakText(response);
    notifier_.speakText(response);

    return response;
}

void VoiceInteractionManager::continuousLoop() {
    while (active_.load()) {
        vad_.reset();
        utteranceReady_ = false;
        setState(State::Listening);

        const std::string result = processOneTurn(30000);  // 30s timeout

        if (!active_.load()) {
            break;
        }

        if (result.empty()) {
            continue;
        }

        // Brief pause before listening again to avoid echo pickup.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void VoiceInteractionManager::onUtteranceReady(std::vector<float> utterance) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingUtterance_ = std::move(utterance);
        utteranceReady_ = true;
    }
    cv_.notify_one();
}

} // namespace atlas::voice
