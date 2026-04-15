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
option(ONNXRUNTIME_ROOT "Path to ONNX Runtime installation" "")

add_library(atlas_core
    src/ai/AIModelClient.cpp
    src/audio/MicStream.cpp
    src/audio/VoiceNotifier.cpp
    src/calendar/CalendarModule.cpp
    src/core/Application.cpp
    src/core/ServiceOrchestrator.cpp
    src/email/EmailModule.cpp
    src/storage/CredentialStore.cpp
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
    target_link_libraries(atlas_ui PRIVATE Qt6::Quick Qt6::Widgets atlas_core)
endif()
