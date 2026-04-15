// src/wake/OnnxWakeModel.cpp — ONNX wake word model loader and inference
// ---------------------------------------------------------------------------
// Performance notes:
//   - ONNX Runtime's C++ API is used for type safety and RAII cleanup.
//   - Each model maintains its own Ort::Session so models are independent.
//   - For quantized int8 models the runtime auto-selects the optimal kernel;
//     no code changes are required.
//   - If running on a device with an NPU or GPU, add the appropriate
//     Execution Provider (e.g. CUDA, CoreML, NNAPI) via SessionOptions.
// ---------------------------------------------------------------------------

#include "wake/OnnxWakeModel.h"

#include <algorithm>
#include <cmath>
#include <iostream>

// ---------------------------------------------------------------------------
// ONNX Runtime headers — guarded by ATLAS_HAS_ONNXRUNTIME so the rest of the
// project can still compile without the runtime installed.
// ---------------------------------------------------------------------------
#ifdef ATLAS_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace atlas::wake {

// ---------------------------------------------------------------------------
// Pimpl implementation
// ---------------------------------------------------------------------------

struct OnnxWakeModel::Impl {
    std::string path;
    std::string name;
    bool loaded{false};

#ifdef ATLAS_HAS_ONNXRUNTIME
    Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "OnnxWakeModel"};
    std::unique_ptr<Ort::Session> session;
    Ort::MemoryInfo memInfo{Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)};

    // Cached input/output metadata.
    std::vector<std::string> inputNames;
    std::vector<std::string> outputNames;
    std::vector<const char*> inputNamePtrs;
    std::vector<const char*> outputNamePtrs;
#endif
};

// ---------------------------------------------------------------------------
// Construction / destruction / move
// ---------------------------------------------------------------------------

OnnxWakeModel::OnnxWakeModel() : impl_(std::make_unique<Impl>()) {}
OnnxWakeModel::~OnnxWakeModel() = default;

OnnxWakeModel::OnnxWakeModel(OnnxWakeModel&&) noexcept = default;
OnnxWakeModel& OnnxWakeModel::operator=(OnnxWakeModel&&) noexcept = default;

// ---------------------------------------------------------------------------
// loadModel
// ---------------------------------------------------------------------------

bool OnnxWakeModel::loadModel(const std::string& path) {
    if (impl_->loaded) {
        std::cerr << "[OnnxWakeModel] Model already loaded (" << impl_->path << ")\n";
        return false;
    }

#ifdef ATLAS_HAS_ONNXRUNTIME
    try {
        Ort::SessionOptions opts;
        // Use a single thread for inference to avoid contention when running
        // multiple models in parallel via the WakeWordEngine.
        opts.SetIntraOpNumThreads(1);
        opts.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        impl_->session = std::make_unique<Ort::Session>(impl_->env, path.c_str(), opts);

        // Cache input tensor names.
        Ort::AllocatorWithDefaultOptions allocator;
        const std::size_t numInputs = impl_->session->GetInputCount();
        for (std::size_t i = 0; i < numInputs; ++i) {
            auto name = impl_->session->GetInputNameAllocated(i, allocator);
            impl_->inputNames.emplace_back(name.get());
        }

        // Cache output tensor names.
        const std::size_t numOutputs = impl_->session->GetOutputCount();
        for (std::size_t i = 0; i < numOutputs; ++i) {
            auto name = impl_->session->GetOutputNameAllocated(i, allocator);
            impl_->outputNames.emplace_back(name.get());
        }

        // Build raw-pointer arrays for Session::Run().
        impl_->inputNamePtrs.clear();
        for (const auto& n : impl_->inputNames) {
            impl_->inputNamePtrs.push_back(n.c_str());
        }
        impl_->outputNamePtrs.clear();
        for (const auto& n : impl_->outputNames) {
            impl_->outputNamePtrs.push_back(n.c_str());
        }

        impl_->path = path;

        // Derive a human-readable name from the file path.
        auto pos = path.find_last_of("/\\");
        std::string fileName = (pos != std::string::npos) ? path.substr(pos + 1) : path;
        // Strip .onnx extension.
        if (fileName.size() > 5 && fileName.substr(fileName.size() - 5) == ".onnx") {
            fileName = fileName.substr(0, fileName.size() - 5);
        }
        impl_->name = fileName;
        impl_->loaded = true;

        std::cout << "[OnnxWakeModel] Loaded model '" << impl_->name
                  << "' from " << path
                  << " (inputs=" << numInputs << ", outputs=" << numOutputs << ")\n";
        return true;

    } catch (const Ort::Exception& e) {
        std::cerr << "[OnnxWakeModel] ONNX Runtime error loading " << path
                  << ": " << e.what() << '\n';
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[OnnxWakeModel] Error loading " << path
                  << ": " << e.what() << '\n';
        return false;
    }
#else
    // Without ONNX Runtime we still record the model path so the rest of the
    // application can function (e.g. UI model list).  Inference will return 0.
    impl_->path = path;

    auto pos = path.find_last_of("/\\");
    std::string fileName = (pos != std::string::npos) ? path.substr(pos + 1) : path;
    if (fileName.size() > 5 && fileName.substr(fileName.size() - 5) == ".onnx") {
        fileName = fileName.substr(0, fileName.size() - 5);
    }
    impl_->name = fileName;
    impl_->loaded = true;

    std::cout << "[OnnxWakeModel] Loaded model stub '" << impl_->name
              << "' (ONNX Runtime not available — inference disabled)\n";
    return true;
#endif
}

// ---------------------------------------------------------------------------
// predict
// ---------------------------------------------------------------------------

float OnnxWakeModel::predict(const std::vector<float>& audioFrame) {
    if (!impl_->loaded) {
        return 0.0f;
    }

#ifdef ATLAS_HAS_ONNXRUNTIME
    try {
        // Build input tensor from the audio frame.
        // Most openWakeWord-style models expect shape [1, N] where N is the
        // number of samples.  Adjust if your model differs.
        const std::array<int64_t, 2> inputShape = {
            1, static_cast<int64_t>(audioFrame.size())
        };

        auto inputTensor = Ort::Value::CreateTensor<float>(
            impl_->memInfo,
            const_cast<float*>(audioFrame.data()),
            audioFrame.size(),
            inputShape.data(),
            inputShape.size()
        );

        auto outputTensors = impl_->session->Run(
            Ort::RunOptions{nullptr},
            impl_->inputNamePtrs.data(),
            &inputTensor,
            1,
            impl_->outputNamePtrs.data(),
            impl_->outputNamePtrs.size()
        );

        // The first output tensor is assumed to contain the confidence score.
        const float* outputData = outputTensors.front().GetTensorData<float>();
        float confidence = outputData[0];

        // Clamp to [0, 1].
        confidence = std::max(0.0f, std::min(1.0f, confidence));
        return confidence;

    } catch (const Ort::Exception& e) {
        std::cerr << "[OnnxWakeModel] Inference error (" << impl_->name
                  << "): " << e.what() << '\n';
        return 0.0f;
    } catch (const std::exception& e) {
        std::cerr << "[OnnxWakeModel] Inference error (" << impl_->name
                  << "): " << e.what() << '\n';
        return 0.0f;
    }
#else
    (void)audioFrame;
    return 0.0f;
#endif
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

bool OnnxWakeModel::isLoaded() const noexcept {
    return impl_->loaded;
}

const std::string& OnnxWakeModel::modelPath() const noexcept {
    return impl_->path;
}

const std::string& OnnxWakeModel::modelName() const noexcept {
    return impl_->name;
}

} // namespace atlas::wake
