// src/wake/ModelManager.cpp — directory-based ONNX model manager
// ---------------------------------------------------------------------------
// Scans a directory for .onnx files and loads them into a WakeWordEngine.
// Supports runtime addition and removal.  When a file is dropped via the UI,
// it is first copied into the managed directory and then loaded.
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
// Construction
// ---------------------------------------------------------------------------

ModelManager::ModelManager(WakeWordEngine& engine, const std::string& modelsDir)
    : engine_(engine), modelsDir_(modelsDir) {
    ensureDirectory();
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
                if (engine_.addModel(p.string())) {
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

        return engine_.addModel(dst.string());

    } catch (const fs::filesystem_error& e) {
        std::cerr << "[ModelManager] File operation error: "
                  << e.what() << '\n';
        return false;
    }
}

bool ModelManager::removeModel(const std::string& fileName) {
    const fs::path target = fs::path(modelsDir_) / fileName;

    // Unload from engine.
    engine_.removeModel(target.string());

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

const std::string& ModelManager::modelsDirectory() const noexcept {
    return modelsDir_;
}

} // namespace atlas::wake
