#pragma once

// ---------------------------------------------------------------------------
// ModelManager — scans a directory for .onnx models and manages their lifecycle
// ---------------------------------------------------------------------------
// On startup, scans a configurable directory (default: ./models/wakewords/)
// for .onnx files and loads each one into a WakeWordEngine.  Supports adding
// and removing models at runtime (e.g. via drag-and-drop) and can poll the
// directory in the background to pick up newly dropped model files without
// recompilation or restart.
// ---------------------------------------------------------------------------

#include <atomic>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace atlas::wake {

// Forward declaration.
class WakeWordEngine;

class ModelManager {
public:
    /// \p engine  The wake word engine to load models into.
    /// \p modelsDir  Directory to scan for .onnx files.
    explicit ModelManager(WakeWordEngine& engine,
                          const std::string& modelsDir = "./models/wakewords");

    ~ModelManager();

    // Non-copyable.
    ModelManager(const ModelManager&) = delete;
    ModelManager& operator=(const ModelManager&) = delete;

    // ---- Directory scanning ------------------------------------------------

    /// Scan the models directory and load all .onnx files found.
    /// Returns the number of models successfully loaded.
    int scanAndLoad();

    /// Scan an arbitrary directory for .onnx files and load them.
    /// Returns the number of models successfully loaded.
    void loadModelsFromDirectory(const std::string& path);

    /// Re-scan the models directory and load any new .onnx files that have
    /// appeared since the last scan.  Already-loaded models are not reloaded.
    void refreshModels();

    // ---- Single-model management -------------------------------------------

    /// Add a single model file.  If the file is outside the models directory
    /// it is copied there first.  Returns true on success.
    bool addModel(const std::string& filePath);

    /// Remove a model by file name (not full path).  Deletes the file from
    /// the models directory and unloads it from the engine.
    bool removeModel(const std::string& fileName);

    // ---- Queries -----------------------------------------------------------

    /// Return a list of model file names currently in the models directory.
    std::vector<std::string> listModels() const;

    /// Return the canonical paths of all models that have been loaded into
    /// the engine by this manager.
    std::vector<std::string> getLoadedModels() const;

    /// Return the models directory path.
    const std::string& modelsDirectory() const noexcept;

    // ---- Background file watching ------------------------------------------

    /// Start a background thread that polls the models directory every
    /// \p intervalSeconds seconds and loads any new .onnx files found.
    void startWatching(int intervalSeconds = 5);

    /// Stop the background watcher thread.
    void stopWatching();

    /// Returns true if the background watcher is active.
    bool isWatching() const noexcept;

private:
    WakeWordEngine& engine_;
    std::string modelsDir_;

    /// Canonical paths of models that have already been loaded.
    mutable std::mutex loadedMutex_;
    std::set<std::string> loadedPaths_;

    /// Background watcher state.
    std::atomic<bool> watching_{false};
    int watchIntervalSec_{5};
    std::thread watchThread_;

    /// Ensure the models directory exists; create it if necessary.
    bool ensureDirectory() const;

    /// Helper: attempt to load a single .onnx file if not already loaded.
    /// Returns true if the model was loaded (or was already loaded).
    bool tryLoad(const std::string& canonicalPath);

    /// Entry point for the background watcher thread.
    void watchLoop();
};

} // namespace atlas::wake
