#pragma once

// ---------------------------------------------------------------------------
// ModelManager — scans a directory for .onnx models and manages their lifecycle
// ---------------------------------------------------------------------------
// On startup, scans a configurable directory (default: ./models/) for .onnx
// files and loads each one into a WakeWordEngine.  Supports adding and
// removing models at runtime (e.g. via drag-and-drop).
// ---------------------------------------------------------------------------

#include <string>
#include <vector>

namespace atlas::wake {

// Forward declaration.
class WakeWordEngine;

class ModelManager {
public:
    /// \p engine  The wake word engine to load models into.
    /// \p modelsDir  Directory to scan for .onnx files.
    explicit ModelManager(WakeWordEngine& engine,
                          const std::string& modelsDir = "./models");

    /// Scan the models directory and load all .onnx files found.
    /// Returns the number of models successfully loaded.
    int scanAndLoad();

    /// Add a single model file.  If the file is outside the models directory
    /// it is copied there first.  Returns true on success.
    bool addModel(const std::string& filePath);

    /// Remove a model by file name (not full path).  Deletes the file from
    /// the models directory and unloads it from the engine.
    bool removeModel(const std::string& fileName);

    /// Return a list of model file names currently in the models directory.
    std::vector<std::string> listModels() const;

    /// Return the models directory path.
    const std::string& modelsDirectory() const noexcept;

private:
    WakeWordEngine& engine_;
    std::string modelsDir_;

    /// Ensure the models directory exists; create it if necessary.
    bool ensureDirectory() const;
};

} // namespace atlas::wake
