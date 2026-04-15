# Atlas Final

## System Architecture

```text
+----------------------+        +-------------------+
| Qt UI (QML)          |<------>| Core Application  |
| Home/Voice/Email/... |        | Event Loop +      |
+----------+-----------+        | Service Orchestr. |
           |                    +---+-----------+---+
           |                        |           |
           v                        v           v
+----------------------+   +----------------+  +----------------+
| Audio Feedback       |   | Voice Pipeline |  | AI Module      |
| VoiceNotifier        |   | PortAudio/VAD  |->| llama.cpp or   |
| ("Listening", etc.)  |   | whisper.cpp    |  | Ollama wrapper |
+----------------------+   +-------+--------+  +----------------+
                                    |
                                    v
                         +----------+----------+
                         | Email + Calendar    |
                         | IMAP/SMTP + CalDAV  |
                         | libcurl/libical     |
                         +----------+----------+
                                    |
                                    v
                         +----------+----------+
                         | Storage             |
                         | Secure credential   |
                         | wrapper + SQLite    |
                         +---------------------+

+---------------------------------------------------+
| Wake Word Detection (ONNX Runtime)                |
|                                                   |
|  MicStream --> WakeWordEngine --> wake callback    |
|                    ^                              |
|                    |                              |
|              ModelManager                         |
|         (scans models/wakewords/)                 |
+---------------------------------------------------+
```

## Project Layout

- `src/main.cpp` – application entry point
- `include` + `src` – core/voice/ai/email/calendar/storage/audio/wake modules
- `models/wakewords/` – drop `.onnx` wake word models here
- `wakeword/` – legacy Python wake word service (deprecated; see C++ engine)
- `qml/Main.qml` – Qt UI screens and accessibility-oriented layout
- `CMakeLists.txt` – modular CMake build

## Build

```bash
cd atlas-final
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run CLI app:

```bash
./build/atlas_assistant
```

## Optional Qt UI Build

```bash
cd atlas-final
cmake -S . -B build-qt -DBUILD_QT_UI=ON
cmake --build build-qt
./build-qt/atlas_ui
```

## Building a Windows EXE

### Native build on Windows

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The executable is at `build/Release/atlas_assistant.exe`.

### Cross-compile from Linux with MinGW

Install the MinGW-w64 toolchain (e.g. `sudo apt install mingw-w64`), then:

```bash
cmake -S . -B build-win64 -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake
cmake --build build-win64
```

The executable is at `build-win64/atlas_assistant.exe`.

### CI / GitHub Actions

Every push to `main` automatically builds a Windows `.exe` via the
**Build Windows EXE** workflow. Download the artifact from the Actions tab.

## Deployment Notes

- **macOS**: install dependencies with Homebrew (`cmake`, `qt`, `portaudio`, `curl`, `sqlite`, `libical`), build with CMake.
- **Windows**: install Qt + CMake + vcpkg packages (`curl`, `sqlite3`, `portaudio`, `libical`) and build with MSVC.
- **Android (Qt)**: build Qt Quick target with Android kit; ensure runtime microphone permission and network permission.
- **iOS (Qt)**: build Qt Quick target with iOS kit; configure microphone usage string and secure credential storage with Keychain.

## Custom Wake Words (ONNX Runtime)

Atlas supports custom wake word detection using ONNX models. Drop `.onnx`
wake word model files into the `models/wakewords/` directory and the
assistant will automatically load and use them — no recompilation required.

### Quick Start

1. Train or obtain an ONNX wake word model (e.g. via
   [openWakeWord](https://github.com/dscripka/openWakeWord)).
2. Place the `.onnx` file in `models/wakewords/`:
   ```
   models/
     wakewords/
       atlas.onnx
       custom1.onnx
   ```
3. Start the assistant. Models are loaded automatically on startup.
4. New models dropped into the directory while the app is running are
   detected within a few seconds (background polling). Deleted model files
   are automatically unloaded.

### Enabling ONNX Runtime

By default, the project compiles without ONNX Runtime and wake word
inference is a no-op. To enable real inference:

```bash
# Download ONNX Runtime for your platform, then:
cmake -S . -B build -DONNXRUNTIME_ROOT=/path/to/onnxruntime-linux-x64-1.17.0
cmake --build build
```

### Architecture

| Component | File | Role |
|---|---|---|
| `OnnxWakeModel` | `include/wake/OnnxWakeModel.h` | Loads a single `.onnx` model and runs inference on audio frames |
| `WakeWordEngine` | `include/wake/WakeWordEngine.h` | Manages multiple models, runs inference, fires callbacks on detection |
| `ModelManager` | `include/wake/ModelManager.h` | Scans `models/wakewords/`, hot-loads new models, removes deleted ones |
| `WakeWordManager` | `include/wake/WakeWordManager.h` | High-level controller that wires mic → engine → callback |
| `MicStream` | `include/audio/MicStream.h` | Captures 16 kHz mono float32 audio and feeds frames to the engine |

### CLI Commands

- `listen` — start wake word detection (also starts automatically on `run`)
- `talk` — manually trigger a voice interaction turn

### Legacy Python Service

The `wakeword/wakeword_service.py` script is a fallback that uses
openWakeWord via Python and communicates over TCP. The C++ ONNX engine
replaces this entirely. See `WakeWordClient` for the TCP client that
connects to the legacy service.

## Notes

The codebase includes integration points and inline comments for known challenges:
- Whisper latency and quantized models
- Silero VAD threshold tuning
- CalDAV provider inconsistencies
- Qt mobile microphone permission differences
- LLM timeout UX and thinking state
- OS-specific secure credential storage
