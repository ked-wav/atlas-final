#!/usr/bin/env python3
"""
Atlas Wake Word Detection Service
==================================
Listens for the wake word "atlas" using openWakeWord and sends a "WAKE\n"
message over TCP to localhost:5055 whenever the wake word is detected with
sufficient confidence.

Requirements:
    pip install openwakeword sounddevice numpy

Usage:
    python wakeword_service.py

Architecture note:
    This Python service is the *fallback* integration path.  The preferred
    approach is to export the openWakeWord ONNX model and run inference
    directly in C++ via ONNX Runtime.  See the "ONNX C++ Migration" comments
    at the bottom of this file for a step-by-step guide.
"""

import socket
import sys
import threading
import time

import numpy as np
import sounddevice as sd
from openwakeword.model import Model

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
WAKE_WORD = "hey_jarvis"  # Placeholder — openWakeWord does not ship a pre-trained "atlas" model.
# To use a custom "atlas" wake word:
#   1. Train or fine-tune an openWakeWord model for the word "atlas".
#   2. Export it as an ONNX file (e.g., atlas.onnx).
#   3. Replace the Model() call below with:
#          Model(wakeword_models=["/path/to/atlas.onnx"])
#   4. The dictionary key returned by predict() will then be "atlas".
#
# Until a custom model is trained, "hey_jarvis" is used as a functional
# stand-in so the full pipeline can be tested end-to-end.
CONFIDENCE_THRESHOLD = 0.5
SAMPLE_RATE = 16000       # 16 kHz mono — required by openWakeWord
CHANNELS = 1
TCP_HOST = "127.0.0.1"
TCP_PORT = 5055

# ---------------------------------------------------------------------------
# Performance notes — chunk size trade-offs
# ---------------------------------------------------------------------------
# Chunk size (in samples) controls the granularity of audio fed to the model.
#
#   Smaller chunks (e.g. 512–1280 samples / 32–80 ms):
#     + Lower latency: faster reaction after the user finishes speaking.
#     − Higher CPU usage: inference runs more often per second.
#     − Potentially lower accuracy: the model receives less context per call.
#
#   Larger chunks (e.g. 2560–4096 samples / 160–256 ms):
#     + Better accuracy: more audio context per inference step.
#     + Lower CPU usage.
#     − Higher perceived latency between utterance and detection.
#
# A chunk size of 1280 samples (80 ms @ 16 kHz) is a good default that
# balances latency and accuracy for real-time wake word detection.

CHUNK_SIZE = 1280  # 80 ms of audio at 16 kHz

# ---------------------------------------------------------------------------
# Performance notes — threshold affects false positives
# ---------------------------------------------------------------------------
# The CONFIDENCE_THRESHOLD determines how aggressively the system triggers.
#
#   Lower threshold (e.g. 0.3):
#     + Catches more true positives (higher recall).
#     − Fires on similar-sounding words or background noise (more false positives).
#
#   Higher threshold (e.g. 0.7–0.8):
#     + Very few false positives (high precision).
#     − May miss quieter or less clearly spoken wake words (lower recall).
#
# 0.5 is a balanced starting point.  Tune based on your mic quality and
# environment noise floor.  Consider adding a per-environment calibration
# step that adjusts the threshold dynamically.

# ---------------------------------------------------------------------------
# Performance notes — latency vs. accuracy
# ---------------------------------------------------------------------------
# End-to-end latency = audio capture chunk time + model inference time.
# On modern CPUs, openWakeWord inference takes < 5 ms per chunk, so the
# dominant factor is chunk duration (CHUNK_SIZE / SAMPLE_RATE).
#
# Accuracy depends on the model, audio quality, and chunk size.  Using the
# streaming API (predict()) with overlapping frames lets the model maintain
# internal state across chunks, improving accuracy over isolated predictions.

# ---------------------------------------------------------------------------
# TCP client — sends "WAKE\n" to the C++ listener
# ---------------------------------------------------------------------------
_tcp_lock = threading.Lock()
_tcp_socket: socket.socket | None = None


def _connect_tcp() -> socket.socket | None:
    """Try to connect to the C++ wake-word listener."""
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2.0)
        s.connect((TCP_HOST, TCP_PORT))
        s.settimeout(None)
        print(f"[wakeword] Connected to {TCP_HOST}:{TCP_PORT}")
        return s
    except OSError as exc:
        print(f"[wakeword] TCP connect failed: {exc}")
        return None


def _send_wake() -> None:
    """Send the WAKE signal over TCP, reconnecting if necessary."""
    global _tcp_socket
    with _tcp_lock:
        if _tcp_socket is None:
            _tcp_socket = _connect_tcp()
        if _tcp_socket is None:
            print("[wakeword] Cannot send WAKE — no TCP connection")
            return
        try:
            _tcp_socket.sendall(b"WAKE\n")
            print("[wakeword] Sent WAKE")
        except OSError:
            print("[wakeword] Send failed — reconnecting")
            try:
                _tcp_socket.close()
            except OSError:
                pass
            _tcp_socket = _connect_tcp()
            if _tcp_socket is not None:
                try:
                    _tcp_socket.sendall(b"WAKE\n")
                except OSError:
                    pass

# ---------------------------------------------------------------------------
# Audio capture & inference
# ---------------------------------------------------------------------------


def main() -> None:
    print("[wakeword] Loading openWakeWord model …")
    oww_model = Model(
        # To use a custom "atlas" ONNX model instead of the built-in one:
        #   wakeword_models=["/path/to/atlas.onnx"]
        inference_framework="onnx",
    )
    print(f"[wakeword] Model loaded.  Listening for wake word (threshold={CONFIDENCE_THRESHOLD}) …")

    def audio_callback(indata: np.ndarray, frames: int, time_info: dict, status: sd.CallbackFlags) -> None:
        """Called by sounddevice for each audio chunk."""
        if status:
            print(f"[wakeword] Audio status: {status}", file=sys.stderr)

        # indata shape: (frames, channels) — we need (frames,) float32
        audio_chunk = indata[:, 0].astype(np.float32)

        # openWakeWord expects int16 samples in the range [-32768, 32767].
        audio_int16 = (audio_chunk * 32767).astype(np.int16)

        # Run prediction — the model maintains internal state across calls.
        prediction = oww_model.predict(audio_int16)

        for model_name, confidence in prediction.items():
            if confidence > CONFIDENCE_THRESHOLD:
                print(f"[wakeword] Detected '{model_name}' (confidence={confidence:.3f})")
                _send_wake()
                # Reset model state to avoid retriggering on the same utterance.
                oww_model.reset()

    # Open the microphone stream.  sounddevice will call audio_callback
    # on a background thread for each CHUNK_SIZE-sample block.
    with sd.InputStream(
        samplerate=SAMPLE_RATE,
        channels=CHANNELS,
        dtype="float32",
        blocksize=CHUNK_SIZE,
        callback=audio_callback,
    ):
        print("[wakeword] Microphone stream open.  Press Ctrl+C to stop.")
        try:
            while True:
                time.sleep(0.1)
        except KeyboardInterrupt:
            print("\n[wakeword] Shutting down.")

    # Clean up TCP
    global _tcp_socket
    with _tcp_lock:
        if _tcp_socket is not None:
            try:
                _tcp_socket.close()
            except OSError:
                pass


if __name__ == "__main__":
    main()


# ===========================================================================
# ONNX C++ Migration Guide
# ===========================================================================
#
# To eliminate the Python dependency entirely and run wake word detection in
# pure C++, follow these steps:
#
# 1. EXPORT THE MODEL
#    openWakeWord models are already ONNX files.  Locate or train your
#    "atlas" model and place the .onnx file in a known path (e.g.
#    models/atlas_wakeword.onnx).
#
# 2. ADD ONNX RUNTIME TO YOUR C++ BUILD
#    Download the ONNX Runtime C API package for your platform from
#    https://github.com/microsoft/onnxruntime/releases.
#    In CMakeLists.txt:
#        find_package(onnxruntime REQUIRED)
#        target_link_libraries(atlas_core PRIVATE onnxruntime)
#
# 3. CREATE AN OnnxWakeWordDetector CLASS
#    - In the constructor, create an Ort::Session with the .onnx model.
#    - Allocate input/output tensors matching the model's expected shapes
#      (typically int16 audio chunks of 1280 samples).
#    - Implement a `predict(const int16_t* samples, size_t count) -> float`
#      method that:
#        a. Copies audio data into the input tensor.
#        b. Calls session.Run().
#        c. Reads the output tensor's confidence score.
#    - Maintain any hidden-state tensors the model requires across calls
#      (check the model's input/output names with Netron or onnxruntime
#      tools).
#
# 4. REPLACE WakeWordClient WITH OnnxWakeWordDetector
#    - Instead of connecting via TCP, WakeWordManager creates an
#      OnnxWakeWordDetector and feeds it audio from PortAudio directly.
#    - On each audio callback, call detector.predict() and fire the
#      wake-word callback when confidence > threshold.
#
# 5. REMOVE THE PYTHON SERVICE
#    Once the C++ path is validated, the Python service and TCP socket
#    client can be removed entirely.
#
# This migration eliminates network latency, removes the Python runtime
# dependency, and keeps the entire pipeline in a single process.
# ===========================================================================
