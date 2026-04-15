#pragma once

// ---------------------------------------------------------------------------
// OnnxWakeModel — loads a single .onnx wake word model and runs inference
// ---------------------------------------------------------------------------
// Wraps ONNX Runtime to load a user-provided wake word model from disk and
// run inference on a float audio frame.  Each instance manages one model.
//
// Performance notes:
//   - Use quantized (int8) ONNX models to reduce CPU usage on edge devices.
//   - Inference time for a single frame is typically < 5 ms on modern CPUs.
//   - Consider pinning the inference thread to a core if running many models.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

// Forward-declare ONNX Runtime types so consumers don't need the ORT headers.
namespace Ort {
class Env;
class Session;
class SessionOptions;
class MemoryInfo;
} // namespace Ort

namespace atlas::wake {

class OnnxWakeModel {
public:
    OnnxWakeModel();
    ~OnnxWakeModel();

    // Non-copyable; movable.
    OnnxWakeModel(const OnnxWakeModel&) = delete;
    OnnxWakeModel& operator=(const OnnxWakeModel&) = delete;
    OnnxWakeModel(OnnxWakeModel&&) noexcept;
    OnnxWakeModel& operator=(OnnxWakeModel&&) noexcept;

    /// Load an ONNX model from \p path.  Returns false on failure.
    /// Thread-safe: may be called once; subsequent calls return false.
    bool loadModel(const std::string& path);

    /// Run inference on a single audio frame.
    /// \p audioFrame contains 32-bit float PCM samples (mono, 16 kHz).
    /// Returns a confidence score in [0, 1].  Returns 0 on error.
    ///
    /// Performance: keep frame sizes small (512–1280 samples / 32–80 ms)
    /// for low latency.  Larger frames improve accuracy but increase delay.
    float predict(const std::vector<float>& audioFrame);

    /// Returns true if a model has been loaded successfully.
    bool isLoaded() const noexcept;

    /// Returns the file path of the loaded model (empty if not loaded).
    const std::string& modelPath() const noexcept;

    /// Returns a human-readable name derived from the file name.
    const std::string& modelName() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace atlas::wake
