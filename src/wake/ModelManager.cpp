// src/wake/ModelManager.cpp — directory-based ONNX model manager
// ---------------------------------------------------------------------------
// Scans a directory for .onnx files and loads them into a WakeWordEngine.
// Supports runtime addition and removal.  A background polling thread
// (startWatching / stopWatching) automatically picks up new models dropped
// into the watched directory without requiring a restart.
// ---------------------------------------------------------------------------

#include "wake/ModelManager.h"
#include "wake/WakeWordEngine.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

ModelManager::ModelManager(WakeWordEngine& engine, const std::string& modelsDir)
    : engine_(engine), modelsDir_(modelsDir) {
    ensureDirectory();
}

ModelManager::~ModelManager() {
    stopWatching();
}

// ---------------------------------------------------------------------------
// Directory helpers
// ---------------------------------------------------------------------------

bool ModelManager::ensureDirectory() const {
    try {
        if (!fs::exists(modelsDir_)) {
            fs::create_directories(modelsDir_);
            std::cout << "[ModelManager] Created models directory: "
                      << modelsDir_ << '\n';
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] Cannot create models directory: "
                  << e.what() << '\n';
        return false;
    }
}

// ---------------------------------------------------------------------------
// tryLoad — internal helper to load a single model if not already loaded
// ---------------------------------------------------------------------------

bool ModelManager::tryLoad(const std::string& canonicalPath) {
    {
        std::lock_guard<std::mutex> lock(loadedMutex_);
        if (loadedPaths_.count(canonicalPath)) {
            return true; // already loaded
        }
    }

    if (engine_.addModel(canonicalPath)) {
        std::lock_guard<std::mutex> lock(loadedMutex_);
        loadedPaths_.insert(canonicalPath);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Scan & load
// ---------------------------------------------------------------------------

int ModelManager::scanAndLoad() {
    if (!ensureDirectory()) {
        return 0;
    }

    int count = 0;
    try {
        for (const auto& entry : fs::directory_iterator(modelsDir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto& p = entry.path();
            if (p.extension() == ".onnx") {
                const std::string canonical = fs::weakly_canonical(p).string();
                if (tryLoad(canonical)) {
                    ++count;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] Error scanning directory: "
                  << e.what() << '\n';
    }

    std::cout << "[ModelManager] Loaded " << count << " model(s) from "
              << modelsDir_ << '\n';
    return count;
}

// ---------------------------------------------------------------------------
// loadModelsFromDirectory — scan an arbitrary directory
// ---------------------------------------------------------------------------

void ModelManager::loadModelsFromDirectory(const std::string& path) {
    try {
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cerr << "[ModelManager] Not a valid directory: " << path << '\n';
            return;
        }

        int count = 0;
        for (const auto& entry : fs::directory_iterator(path)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto& p = entry.path();
            if (p.extension() == ".onnx") {
                const std::string canonical = fs::weakly_canonical(p).string();
                if (tryLoad(canonical)) {
                    ++count;
                }
            }
        }

        std::cout << "[ModelManager] Loaded " << count
                  << " model(s) from " << path << '\n';
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] Error scanning directory " << path
                  << ": " << e.what() << '\n';
    }
}

// ---------------------------------------------------------------------------
// refreshModels — re-scan and pick up only new files
// ---------------------------------------------------------------------------

void ModelManager::refreshModels() {
    if (!ensureDirectory()) {
        return;
    }

    try {
        for (const auto& entry : fs::directory_iterator(modelsDir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            const auto& p = entry.path();
            if (p.extension() == ".onnx") {
                const std::string canonical = fs::weakly_canonical(p).string();
                if (tryLoad(canonical)) {
                    // tryLoad logs internally via WakeWordEngine
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] Error refreshing models: "
                  << e.what() << '\n';
    }
}

// ---------------------------------------------------------------------------
// Add / remove at runtime
// ---------------------------------------------------------------------------

bool ModelManager::addModel(const std::string& filePath) {
    if (!ensureDirectory()) {
        return false;
    }

    try {
        const fs::path src(filePath);

        // Validate extension.
        if (src.extension() != ".onnx") {
            std::cerr << "[ModelManager] Rejected non-ONNX file: "
                      << filePath << '\n';
            return false;
        }

        // Determine destination inside the managed directory.
        const fs::path dst = fs::path(modelsDir_) / src.filename();

        // Copy the file if it isn't already in the managed directory.
        // Use weakly_canonical to avoid throwing when paths don't exist yet.
        if (fs::weakly_canonical(src.parent_path()) != fs::weakly_canonical(modelsDir_)) {
            fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            std::cout << "[ModelManager] Copied " << src.filename().string()
                      << " into " << modelsDir_ << '\n';
        }

        const std::string canonical = fs::weakly_canonical(dst).string();
        return tryLoad(canonical);

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] File operation error: "
                  << e.what() << '\n';
        return false;
    }
}

bool ModelManager::removeModel(const std::string& fileName) {
    const fs::path target = fs::path(modelsDir_) / fileName;
    const std::string canonical = fs::weakly_canonical(target).string();

    // Unload from engine.
    engine_.removeModel(canonical);

    // Remove from our loaded set.
    {
        std::lock_guard<std::mutex> lock(loadedMutex_);
        loadedPaths_.erase(canonical);
    }

    // Remove from disk.
    try {
        if (fs::exists(target)) {
            fs::remove(target);
            std::cout << "[ModelManager] Deleted " << fileName
                      << " from " << modelsDir_ << '\n';
        }
        return true;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] Cannot delete file: "
                  << e.what() << '\n';
        return false;
    }
}

// ---------------------------------------------------------------------------
// Listing
// ---------------------------------------------------------------------------

std::vector<std::string> ModelManager::listModels() const {
    std::vector<std::string> names;
    try {
        for (const auto& entry : fs::directory_iterator(modelsDir_)) {
            if (entry.is_regular_file() && entry.path().extension() == ".onnx") {
                names.push_back(entry.path().filename().string());
            }
        }
    } catch (const fs::filesystem_error&) {
        // Directory may not exist yet — return empty list.
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> ModelManager::getLoadedModels() const {
    std::lock_guard<std::mutex> lock(loadedMutex_);
    return {loadedPaths_.begin(), loadedPaths_.end()};
}

const std::string& ModelManager::modelsDirectory() const noexcept {
    return modelsDir_;
}

// ---------------------------------------------------------------------------
// Background file watching
// ---------------------------------------------------------------------------

void ModelManager::startWatching(int intervalSeconds) {
    if (watching_.load()) {
        return;
    }

    watchIntervalSec_ = std::max(1, intervalSeconds);
    watching_.store(true);
    watchThread_ = std::thread(&ModelManager::watchLoop, this);

    std::cout << "[ModelManager] File watcher started (polling every "
              << watchIntervalSec_ << "s)\n";
}

void ModelManager::stopWatching() {
    if (!watching_.load()) {
        return;
    }

    watching_.store(false);
    if (watchThread_.joinable()) {
        watchThread_.join();
    }
    std::cout << "[ModelManager] File watcher stopped\n";
}

bool ModelManager::isWatching() const noexcept {
    return watching_.load();
}

void ModelManager::watchLoop() {
    while (watching_.load()) {
        // Sleep in small increments so we can exit quickly when stopped.
        for (int i = 0; i < watchIntervalSec_ * 10 && watching_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (!watching_.load()) {
            break;
        }

        refreshModels();
    }
}

} // namespace atlas::wake
