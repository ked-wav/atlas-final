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
```

## Project Layout

- `src/main.cpp` – application entry point
- `include` + `src` – core/voice/ai/email/calendar/storage/audio modules
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

## Deployment Notes

- **macOS**: install dependencies with Homebrew (`cmake`, `qt`, `portaudio`, `curl`, `sqlite`, `libical`), build with CMake.
- **Windows**: install Qt + CMake + vcpkg packages (`curl`, `sqlite3`, `portaudio`, `libical`) and build with MSVC.
- **Android (Qt)**: build Qt Quick target with Android kit; ensure runtime microphone permission and network permission.
- **iOS (Qt)**: build Qt Quick target with iOS kit; configure microphone usage string and secure credential storage with Keychain.

## Notes

The codebase includes integration points and inline comments for known challenges:
- Whisper latency and quantized models
- Silero VAD threshold tuning
- CalDAV provider inconsistencies
- Qt mobile microphone permission differences
- LLM timeout UX and thinking state
- OS-specific secure credential storage
