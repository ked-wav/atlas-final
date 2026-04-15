cmake_minimum_required(VERSION 3.20)
project(atlas_final VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_QT_UI "Build Qt/QML UI application" OFF)

# ---------------------------------------------------------------------------
# ONNX Runtime (optional)
# ---------------------------------------------------------------------------
# To enable ONNX Runtime wake word inference:
#   1. Download the ONNX Runtime release for your platform from
#      https://github.com/microsoft/onnxruntime/releases
#   2. Extract it and set ONNXRUNTIME_ROOT to the extracted directory, e.g.:
#        cmake -DONNXRUNTIME_ROOT=/path/to/onnxruntime-linux-x64-1.17.0 ..
#   3. Alternatively, install the onnxruntime package via your system package
#      manager and let CMake find it automatically.
#
# When ONNX Runtime is not found the project still compiles — inference
# calls return 0 and the wake word engine is effectively a no-op.
# ---------------------------------------------------------------------------
set(ONNXRUNTIME_ROOT "" CACHE PATH "Path to ONNX Runtime installation")

add_library(atlas_core
    src/ai/AIModelClient.cpp
    src/audio/MicRecorder.cpp
    src/audio/MicStream.cpp
    src/audio/PiperSynthesizer.cpp
    src/audio/SpeechDetector.cpp
    src/audio/VoiceNotifier.cpp
    src/audio/WhisperTranscriber.cpp
    src/calendar/CalendarModule.cpp
    src/core/Application.cpp
    src/core/ServiceOrchestrator.cpp
    src/email/EmailModule.cpp
    src/storage/CredentialStore.cpp
    src/voice/VoiceInteractionManager.cpp
    src/voice/VoicePipeline.cpp
    src/wake/OnnxWakeModel.cpp
    src/wake/WakeWordEngine.cpp
    src/wake/WakeWordClient.cpp
    src/wake/WakeWordManager.cpp
    src/wake/ModelManager.cpp
)

target_include_directories(atlas_core PUBLIC include)

find_package(Threads REQUIRED)
target_link_libraries(atlas_core PUBLIC Threads::Threads)

# Winsock2 for WakeWordClient TCP sockets on Windows
if(WIN32)
    target_link_libraries(atlas_core PUBLIC ws2_32)
endif()

find_package(CURL QUIET)
if(CURL_FOUND)
    target_link_libraries(atlas_core PRIVATE CURL::libcurl)
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_CURL=1)
endif()

find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    target_link_libraries(atlas_core PRIVATE SQLite::SQLite3)
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_SQLITE3=1)
endif()

# ---------------------------------------------------------------------------
# PortAudio (optional — real-time microphone capture)
# ---------------------------------------------------------------------------
set(PORTAUDIO_ROOT "" CACHE PATH "Path to PortAudio installation")

if(PORTAUDIO_ROOT)
    find_path(PORTAUDIO_INCLUDE_DIR NAMES portaudio.h
        PATHS "${PORTAUDIO_ROOT}/include" NO_DEFAULT_PATH)
    find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio_x64
        PATHS "${PORTAUDIO_ROOT}/lib" NO_DEFAULT_PATH)
else()
    find_path(PORTAUDIO_INCLUDE_DIR NAMES portaudio.h)
    find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio_x64)
endif()

if(PORTAUDIO_INCLUDE_DIR AND PORTAUDIO_LIBRARY)
    message(STATUS "PortAudio found: ${PORTAUDIO_LIBRARY}")
    target_include_directories(atlas_core PRIVATE "${PORTAUDIO_INCLUDE_DIR}")
    target_link_libraries(atlas_core PRIVATE "${PORTAUDIO_LIBRARY}")
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_PORTAUDIO=1)
else()
    message(STATUS "PortAudio NOT found — microphone capture disabled.  "
                   "Set PORTAUDIO_ROOT to enable.")
endif()

# ---------------------------------------------------------------------------
# whisper.cpp (optional — speech-to-text)
# ---------------------------------------------------------------------------
set(WHISPER_ROOT "" CACHE PATH "Path to whisper.cpp installation")

if(WHISPER_ROOT)
    find_path(WHISPER_INCLUDE_DIR NAMES whisper.h
        PATHS "${WHISPER_ROOT}/include" "${WHISPER_ROOT}" NO_DEFAULT_PATH)
    find_library(WHISPER_LIBRARY NAMES whisper
        PATHS "${WHISPER_ROOT}/lib" "${WHISPER_ROOT}/build" NO_DEFAULT_PATH)
else()
    find_path(WHISPER_INCLUDE_DIR NAMES whisper.h)
    find_library(WHISPER_LIBRARY NAMES whisper)
endif()

if(WHISPER_INCLUDE_DIR AND WHISPER_LIBRARY)
    message(STATUS "whisper.cpp found: ${WHISPER_LIBRARY}")
    target_include_directories(atlas_core PRIVATE "${WHISPER_INCLUDE_DIR}")
    target_link_libraries(atlas_core PRIVATE "${WHISPER_LIBRARY}")
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_WHISPER=1)
else()
    message(STATUS "whisper.cpp NOT found — speech-to-text uses placeholder.  "
                   "Set WHISPER_ROOT to enable.")
endif()

# ---------------------------------------------------------------------------
# Piper TTS (optional — text-to-speech synthesis)
# ---------------------------------------------------------------------------
set(PIPER_ROOT "" CACHE PATH "Path to Piper TTS installation")

if(PIPER_ROOT)
    find_path(PIPER_INCLUDE_DIR NAMES piper.h
        PATHS "${PIPER_ROOT}/include" "${PIPER_ROOT}" NO_DEFAULT_PATH)
    find_library(PIPER_LIBRARY NAMES piper
        PATHS "${PIPER_ROOT}/lib" "${PIPER_ROOT}/build" NO_DEFAULT_PATH)
else()
    find_path(PIPER_INCLUDE_DIR NAMES piper.h)
    find_library(PIPER_LIBRARY NAMES piper)
endif()

if(PIPER_INCLUDE_DIR AND PIPER_LIBRARY)
    message(STATUS "Piper TTS found: ${PIPER_LIBRARY}")
    target_include_directories(atlas_core PRIVATE "${PIPER_INCLUDE_DIR}")
    target_link_libraries(atlas_core PRIVATE "${PIPER_LIBRARY}")
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_PIPER=1)
else()
    message(STATUS "Piper TTS NOT found — TTS uses text logging.  "
                   "Set PIPER_ROOT to enable.")
endif()

# ---------------------------------------------------------------------------
# ONNX Runtime detection
# ---------------------------------------------------------------------------
if(ONNXRUNTIME_ROOT)
    # User-specified path — look for headers and library there.
    find_path(ONNXRUNTIME_INCLUDE_DIR
        NAMES onnxruntime_cxx_api.h
        PATHS "${ONNXRUNTIME_ROOT}/include"
        NO_DEFAULT_PATH
    )
    find_library(ONNXRUNTIME_LIBRARY
        NAMES onnxruntime
        PATHS "${ONNXRUNTIME_ROOT}/lib"
        NO_DEFAULT_PATH
    )
else()
    # Try system paths.
    find_path(ONNXRUNTIME_INCLUDE_DIR NAMES onnxruntime_cxx_api.h)
    find_library(ONNXRUNTIME_LIBRARY NAMES onnxruntime)
endif()

if(ONNXRUNTIME_INCLUDE_DIR AND ONNXRUNTIME_LIBRARY)
    message(STATUS "ONNX Runtime found: ${ONNXRUNTIME_LIBRARY}")
    target_include_directories(atlas_core PRIVATE "${ONNXRUNTIME_INCLUDE_DIR}")
    target_link_libraries(atlas_core PRIVATE "${ONNXRUNTIME_LIBRARY}")
    target_compile_definitions(atlas_core PRIVATE ATLAS_HAS_ONNXRUNTIME=1)
else()
    message(STATUS "ONNX Runtime NOT found — wake word inference disabled.  "
                   "Set ONNXRUNTIME_ROOT to enable.")
endif()

add_executable(atlas_assistant src/main.cpp)
target_link_libraries(atlas_assistant PRIVATE atlas_core)

enable_testing()
add_test(NAME atlas_smoke_run COMMAND atlas_assistant --smoke-test)

if(BUILD_QT_UI)
    find_package(Qt6 6.4 COMPONENTS Quick Widgets REQUIRED)
    qt_standard_project_setup(REQUIRES 6.4)

    qt_add_executable(atlas_ui
        src/qt_main.cpp
        ui/MainWindow.cpp
        ui/MainWindow.h
        ui/AssistantWidget.cpp
        ui/AssistantWidget.h
    )
    qt_add_qml_module(atlas_ui
        URI AtlasUI
        VERSION 1.0
        QML_FILES qml/Main.qml
    )
    target_include_directories(atlas_ui PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/ui
    )
    target_compile_definitions(atlas_ui PRIVATE BUILD_QT_WIDGETS_UI=1)
    target_link_libraries(atlas_ui PRIVATE Qt6::Quick Qt6::Widgets atlas_core)
endif()
