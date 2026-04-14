cmake_minimum_required(VERSION 3.20)
project(atlas_final VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_QT_UI "Build Qt/QML UI application" OFF)

add_library(atlas_core
    src/ai/AIModelClient.cpp
    src/audio/VoiceNotifier.cpp
    src/calendar/CalendarModule.cpp
    src/core/Application.cpp
    src/core/ServiceOrchestrator.cpp
    src/email/EmailModule.cpp
    src/storage/CredentialStore.cpp
    src/voice/VoicePipeline.cpp
)

target_include_directories(atlas_core PUBLIC include)

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

add_executable(atlas_assistant src/main.cpp)
target_link_libraries(atlas_assistant PRIVATE atlas_core)

enable_testing()
add_test(NAME atlas_smoke_run COMMAND atlas_assistant --smoke-test)

if(BUILD_QT_UI)
    find_package(Qt6 6.4 COMPONENTS Quick REQUIRED)
    qt_standard_project_setup(REQUIRES 6.4)

    qt_add_executable(atlas_ui src/qt_main.cpp)
    qt_add_qml_module(atlas_ui
        URI AtlasUI
        VERSION 1.0
        QML_FILES qml/Main.qml
    )
    target_link_libraries(atlas_ui PRIVATE Qt6::Quick)
endif()
