/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OHOS_MOCK_CAMERA_FRAMEWORK_V2_H
#define OHOS_MOCK_CAMERA_FRAMEWORK_V2_H

#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <functional>
#include <unordered_map>

namespace OHOS {
namespace CameraStandard {

// 前向声明
class Surface;
class CameraDevice;
class CameraMetadata;

// ============================================
// 错误码定义
// ============================================
enum CameraErrorCode {
    CAMERA_OK = 0,
    CAMERA_INVALID_ARG = 1,
    CAMERA_NOT_PERMITTED = 2,
    SERVICE_FATL_ERROR = 3,
    DEVICE_DISCONNECTED = 4,
    DEVICE_IN_USE = 5,
    CONFLICT_CAMERA = 6,
    DEVICE_BUSY = 7,
    CAMERA_CLOSED = 8
};

// ============================================
// 相机格式定义
// ============================================
enum CameraFormat {
    CAMERA_FORMAT_YUV_420 = 0,
    CAMERA_FORMAT_YUV_422 = 1,
    CAMERA_FORMAT_YUV_444 = 2,
    CAMERA_FORMAT_RGB_565 = 3,
    CAMERA_FORMAT_RGB_888 = 4,
    CAMERA_FORMAT_RGBA_8888 = 5,
    CAMERA_FORMAT_JPEG = 6,
    CAMERA_FORMAT_BLOB = 7,
    CAMERA_FORMAT_NV12 = 8,
    CAMERA_FORMAT_NV21 = 9,
    CAMERA_FORMAT_YCRCB_420_SP = 10,
    CAMERA_FORMAT_YCRCB_422_SP = 11
};

// ============================================
// 场景模式定义
// ============================================
enum SceneMode {
    NORMAL_MODE = 0,
    PORTRAIT_MODE = 1,
    NIGHT_MODE = 2,
    VIDEO_MODE = 3,
    CAPTURE_MODE = 4,
    PROFESSIONAL_MODE = 5
};

// ============================================
// Size 结构体
// ============================================
struct Size {
    int32_t width;
    int32_t height;

    Size() : width(0), height(0) {}
    Size(int32_t w, int32_t h) : width(w), height(h) {}

    bool operator==(const Size& other) const {
        return width == other.width && height == other.height;
    }
};

// ============================================
// Profile 结构体
// ============================================
struct Profile {
    CameraFormat format;
    Size size;

    Profile() : format(CAMERA_FORMAT_YUV_420), size() {}
    Profile(CameraFormat f, const Size& s) : format(f), size(s) {}
};

// ============================================
// PhotoCaptureSetting 结构体
// ============================================
struct PhotoCaptureSetting {
    int32_t rotation;
    int32_t quality;
    bool hasLocation;
    double latitude;
    double longitude;

    PhotoCaptureSetting()
        : rotation(0), quality(100), hasLocation(false), latitude(0.0), longitude(0.0) {}
};

// ============================================
// ManagerCallback - 相机管理器回调
// ============================================
class ManagerCallback {
public:
    ManagerCallback() = default;
    virtual ~ManagerCallback() = default;

    virtual void OnCameraStatusChanged(const std::string& cameraId, int32_t status) {}
    virtual void OnCameraServiceDied(const std::string& cameraId) {}
};

// ============================================
// StateCallback - 相机状态回调
// ============================================
class StateCallback {
public:
    StateCallback() = default;
    virtual ~StateCallback() = default;

    virtual void OnCreated(std::shared_ptr<CameraDevice> camera) {}
    virtual void OnCreateFailed(const std::string& cameraId, int32_t errorCode) {}
    virtual void OnReleased(std::shared_ptr<CameraDevice> camera) {}
    virtual void OnConfigured(std::shared_ptr<CameraDevice> camera) {}
    virtual void OnConfigureFailed(const std::string& cameraId, int32_t errorCode) {}
    virtual void OnFatalError(int32_t errorCode) {}
};

// ============================================
// SessionCallback - 会话回调
// ============================================
class SessionCallback {
public:
    SessionCallback() = default;
    virtual ~SessionCallback() = default;

    virtual void OnError(int32_t errorCode) {}
};

// ============================================
// FocusCallback - 对焦回调
// ============================================
class FocusCallback {
public:
    FocusCallback() = default;
    virtual ~FocusCallback() = default;

    virtual void OnFocusState(int32_t focusState) {}
};

// ============================================
// PreviewOutputCallback - 预览输出回调
// ============================================
class PreviewOutputCallback {
public:
    PreviewOutputCallback() = default;
    virtual ~PreviewOutputCallback() = default;

    virtual void OnFrameStarted() {}
    virtual void OnFrameEnded(int32_t frameCount) {}
    virtual void OnError(int32_t errorCode) {}
};

// ============================================
// PhotoOutputCallback - 拍照输出回调
// ============================================
class PhotoOutputCallback {
public:
    PhotoOutputCallback() = default;
    virtual ~PhotoOutputCallback() = default;

    virtual void OnCaptureStarted(int32_t captureId) {}
    virtual void OnCaptureEnded(int32_t captureId, int32_t frameCount) {}
    virtual void OnFrameShutter(int32_t captureId, uint64_t timestamp) {}
    virtual void OnCaptureError(int32_t captureId, int32_t errorCode) {}
};

// ============================================
// CameraInputCallback - 相机输入回调
// ============================================
class CameraInputCallback {
public:
    CameraInputCallback() = default;
    virtual ~CameraInputCallback() = default;

    virtual void OnError(int32_t errorCode) {}
};

// ============================================
// CameraDevice - 相机设备
// ============================================
class CameraDevice {
public:
    CameraDevice() = default;
    virtual ~CameraDevice() = default;

    virtual std::string GetID() const {
        return cameraId_;
    }

    void SetID(const std::string& cameraId) {
        cameraId_ = cameraId;
    }

private:
    std::string cameraId_;
};

// ============================================
// CameraInput - 相机输入
// ============================================
class CameraInput {
public:
    CameraInput() : isOpened_(false) {}
    virtual ~CameraInput() = default;

    virtual int32_t Open() {
        if (isOpened_) {
            return CAMERA_BUSY;
        }
        isOpened_ = true;
        return CAMERA_OK;
    }

    virtual int32_t Close() {
        if (!isOpened_) {
            return CAMERA_CLOSED;
        }
        isOpened_ = false;
        return CAMERA_OK;
    }

    virtual int32_t Release() {
        isOpened_ = false;
        return CAMERA_OK;
    }

    virtual std::string GetCameraId() const {
        return cameraId_;
    }

    void SetCameraId(const std::string& cameraId) {
        cameraId_ = cameraId;
    }

    virtual int32_t SetCameraSettings(const std::string& metadata) {
        metadata_ = metadata;
        return CAMERA_OK;
    }

    virtual void SetErrorCallback(std::shared_ptr<CameraInputCallback> callback) {
        errorCallback_ = callback;
    }

    bool IsOpened() const {
        return isOpened_;
    }

private:
    std::string cameraId_;
    bool isOpened_;
    std::string metadata_;
    std::shared_ptr<CameraInputCallback> errorCallback_;
};

// ============================================
// PreviewOutput - 预览输出
// ============================================
class PreviewOutput {
public:
    PreviewOutput() : isStarted_(false), frameRateMin_(0), frameRateMax_(0) {}
    virtual ~PreviewOutput() = default;

    virtual int32_t Start() {
        if (isStarted_) {
            return CAMERA_BUSY;
        }
        isStarted_ = true;
        return CAMERA_OK;
    }

    virtual int32_t Stop() {
        if (!isStarted_) {
            return CAMERA_CLOSED;
        }
        isStarted_ = false;
        return CAMERA_OK;
    }

    virtual int32_t Release() {
        isStarted_ = false;
        return CAMERA_OK;
    }

    virtual int32_t SetFrameRate(int32_t min, int32_t max) {
        frameRateMin_ = min;
        frameRateMax_ = max;
        return CAMERA_OK;
    }

    virtual void SetCallback(std::shared_ptr<PreviewOutputCallback> callback) {
        callback_ = callback;
    }

    void SetSurface(std::shared_ptr<Surface> surface) {
        surface_ = surface;
    }

    bool IsStarted() const {
        return isStarted_;
    }

    std::shared_ptr<Surface> GetSurface() const {
        return surface_;
    }

private:
    bool isStarted_;
    int32_t frameRateMin_;
    int32_t frameRateMax_;
    std::shared_ptr<Surface> surface_;
    std::shared_ptr<PreviewOutputCallback> callback_;
};

// ============================================
// PhotoOutput - 拍照输出
// ============================================
class PhotoOutput {
public:
    PhotoOutput() : isCapturing_(false) {}
    virtual ~PhotoOutput() = default;

    virtual int32_t Capture() {
        if (isCapturing_) {
            return CAMERA_BUSY;
        }
        isCapturing_ = true;
        return CAMERA_OK;
    }

    virtual int32_t Release() {
        isCapturing_ = false;
        return CAMERA_OK;
    }

    virtual void SetCallback(std::shared_ptr<PhotoOutputCallback> callback) {
        callback_ = callback;
    }

    virtual int32_t Cancel() {
        isCapturing_ = false;
        return CAMERA_OK;
    }

    bool IsCapturing() const {
        return isCapturing_;
    }

private:
    bool isCapturing_;
    std::shared_ptr<PhotoOutputCallback> callback_;
};

// ============================================
// CaptureSession - 捕获会话 (支持三阶段提交)
// ============================================
class CaptureSession {
public:
    enum ConfigState {
        CONFIG_STATE_IDLE = 0,
        CONFIG_STATE_CONFIGURING = 1,
        CONFIG_STATE_CONFIGURED = 2,
        CONFIG_STATE_STARTED = 3
    };

    CaptureSession()
        : configState_(CONFIG_STATE_IDLE),
          input_(nullptr),
          sessionCallback_(nullptr),
          focusCallback_(nullptr) {}

    virtual ~CaptureSession() = default;

    // 三阶段提交第一阶段：开始配置
    virtual int32_t BeginConfig() {
        if (configState_ == CONFIG_STATE_CONFIGURING) {
            return DEVICE_BUSY;
        }
        if (configState_ == CONFIG_STATE_STARTED) {
            // 需要先停止
            return DEVICE_BUSY;
        }

        configState_ = CONFIG_STATE_CONFIGURING;
        // 清空之前的配置
        input_ = nullptr;
        outputs_.clear();
        return CAMERA_OK;
    }

    // 添加输入
    virtual int32_t AddInput(std::shared_ptr<CameraInput> input) {
        if (configState_ != CONFIG_STATE_CONFIGURING) {
            return DEVICE_DISCONNECTED;
        }
        if (input == nullptr) {
            return CAMERA_INVALID_ARG;
        }
        if (input_ != nullptr) {
            return CONFLICT_CAMERA; // 已经有输入了
        }
        input_ = input;
        return CAMERA_OK;
    }

    // 添加输出
    virtual int32_t AddOutput(std::shared_ptr<PreviewOutput> output) {
        if (configState_ != CONFIG_STATE_CONFIGURING) {
            return DEVICE_DISCONNECTED;
        }
        if (output == nullptr) {
            return CAMERA_INVALID_ARG;
        }
        outputs_.push_back(output);
        return CAMERA_OK;
    }

    virtual int32_t AddOutput(std::shared_ptr<PhotoOutput> output) {
        if (configState_ != CONFIG_STATE_CONFIGURING) {
            return DEVICE_DISCONNECTED;
        }
        if (output == nullptr) {
            return CAMERA_INVALID_ARG;
        }
        photoOutputs_.push_back(output);
        return CAMERA_OK;
    }

    // 三阶段提交第三阶段：提交配置
    virtual int32_t CommitConfig() {
        if (configState_ != CONFIG_STATE_CONFIGURING) {
            return DEVICE_DISCONNECTED;
        }
        if (input_ == nullptr) {
            return CAMERA_INVALID_ARG;
        }
        if (outputs_.empty() && photoOutputs_.empty()) {
            return CAMERA_INVALID_ARG;
        }

        configState_ = CONFIG_STATE_CONFIGURED;
        return CAMERA_OK;
    }

    // 开始捕获
    virtual int32_t Start() {
        if (configState_ != CONFIG_STATE_CONFIGURED) {
            return DEVICE_DISCONNECTED;
        }
        configState_ = CONFIG_STATE_STARTED;

        // 启动所有输出
        for (auto& output : outputs_) {
            if (output) {
                output->Start();
            }
        }

        return CAMERA_OK;
    }

    // 停止捕获
    virtual int32_t Stop() {
        if (configState_ != CONFIG_STATE_STARTED) {
            return CAMERA_CLOSED;
        }

        // 停止所有输出
        for (auto& output : outputs_) {
            if (output) {
                output->Stop();
            }
        }

        configState_ = CONFIG_STATE_CONFIGURED;
        return CAMERA_OK;
    }

    // 释放会话
    virtual int32_t Release() {
        Stop();
        configState_ = CONFIG_STATE_IDLE;
        input_ = nullptr;
        outputs_.clear();
        photoOutputs_.clear();
        return CAMERA_OK;
    }

    // 设置回调
    virtual void SetCallback(std::shared_ptr<SessionCallback> callback) {
        sessionCallback_ = callback;
    }

    virtual void SetFocusCallback(std::shared_ptr<FocusCallback> callback) {
        focusCallback_ = callback;
    }

    ConfigState GetConfigState() const {
        return configState_;
    }

    std::shared_ptr<CameraInput> GetInput() const {
        return input_;
    }

    const std::vector<std::shared_ptr<PreviewOutput>>& GetOutputs() const {
        return outputs_;
    }

private:
    ConfigState configState_;
    std::shared_ptr<CameraInput> input_;
    std::vector<std::shared_ptr<PreviewOutput>> outputs_;
    std::vector<std::shared_ptr<PhotoOutput>> photoOutputs_;
    std::shared_ptr<SessionCallback> sessionCallback_;
    std::shared_ptr<FocusCallback> focusCallback_;
};

// ============================================
// CameraManager - 相机管理器
// ============================================
class CameraManager {
public:
    static CameraManager* GetInstance() {
        static CameraManager instance;
        return &instance;
    }

    CameraManager() : managerCallback_(nullptr) {}
    ~CameraManager() = default;

    // 获取支持的相机列表
    virtual std::vector<std::shared_ptr<CameraDevice>> GetSupportedCameras() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::shared_ptr<CameraDevice>> result;
        for (const auto& deviceId : mockCameras_) {
            auto device = std::make_shared<CameraDevice>();
            device->SetID(deviceId);
            result.push_back(device);
        }
        return result;
    }

    // 创建相机输入
    virtual int32_t CreateCameraInput(std::shared_ptr<CameraDevice> camera,
                                      std::shared_ptr<CameraInput>* input) {
        if (camera == nullptr || input == nullptr) {
            return CAMERA_INVALID_ARG;
        }

        std::string cameraId = camera->GetID();

        // 检查相机是否存在
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (mockCameras_.find(cameraId) == mockCameras_.end()) {
                return CAMERA_INVALID_ARG;
            }
        }

        // 检查相机是否已被使用
        if (activeInputs_.find(cameraId) != activeInputs_.end()) {
            return CONFLICT_CAMERA;
        }

        auto cameraInput = std::make_shared<CameraInput>();
        cameraInput->SetCameraId(cameraId);

        *input = cameraInput;
        activeInputs_[cameraId] = cameraInput;

        return CAMERA_OK;
    }

    // 创建捕获会话
    virtual std::shared_ptr<CaptureSession> CreateCaptureSession(SceneMode mode = NORMAL_MODE) {
        auto session = std::make_shared<CaptureSession>();
        sessions_.push_back(session);
        return session;
    }

    // 创建预览输出
    virtual int32_t CreatePreviewOutput(const Profile& profile,
                                        std::shared_ptr<Surface> surface,
                                        std::shared_ptr<PreviewOutput>* output) {
        if (surface == nullptr || output == nullptr) {
            return CAMERA_INVALID_ARG;
        }

        auto previewOutput = std::make_shared<PreviewOutput>();
        previewOutput->SetSurface(surface);

        *output = previewOutput;
        return CAMERA_OK;
    }

    // 创建拍照输出
    virtual int32_t CreatePhotoOutput(const Profile& profile,
                                      std::shared_ptr<void> producer,
                                      std::shared_ptr<PhotoOutput>* output) {
        if (producer == nullptr || output == nullptr) {
            return CAMERA_INVALID_ARG;
        }

        auto photoOutput = std::make_shared<PhotoOutput>();

        *output = photoOutput;
        return CAMERA_OK;
    }

    // 设置管理器回调
    virtual void SetCallback(std::shared_ptr<ManagerCallback> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        managerCallback_ = callback;
    }

    // ============================================
    // Mock 配置方法 (用于测试)
    // ============================================

    // 添加模拟相机
    void AddMockCamera(const std::string& cameraId) {
        std::lock_guard<std::mutex> lock(mutex_);
        mockCameras_.insert(cameraId);
    }

    // 移除模拟相机
    void RemoveMockCamera(const std::string& cameraId) {
        std::lock_guard<std::mutex> lock(mutex_);
        mockCameras_.erase(cameraId);
    }

    // 清空所有模拟相机
    void ClearMockCameras() {
        std::lock_guard<std::mutex> lock(mutex_);
        mockCameras_.clear();
    }

    // 设置创建输入的返回值（用于测试错误处理）
    void SetCreateInputResult(int32_t result) {
        std::lock_guard<std::mutex> lock(mutex_);
        createInputResult_ = result;
    }

    // 获取活跃的输入
    std::shared_ptr<CameraInput> GetActiveInput(const std::string& cameraId) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = activeInputs_.find(cameraId);
        if (it != activeInputs_.end()) {
            return it->second;
        }
        return nullptr;
    }

    // 清除所有活跃输入
    void ClearActiveInputs() {
        std::lock_guard<std::mutex> lock(mutex_);
        activeInputs_.clear();
    }

    // 获取会话列表
    const std::vector<std::shared_ptr<CaptureSession>>& GetSessions() const {
        return sessions_;
    }

    // 清除所有会话
    void ClearSessions() {
        sessions_.clear();
    }

private:
    std::mutex mutex_;
    std::unordered_set<std::string> mockCameras_;
    std::unordered_map<std::string, std::shared_ptr<CameraInput>> activeInputs_;
    std::vector<std::shared_ptr<CaptureSession>> sessions_;
    std::shared_ptr<ManagerCallback> managerCallback_;
    int32_t createInputResult_ = CAMERA_OK;
};

} // namespace CameraStandard
} // namespace OHOS

#endif // OHOS_MOCK_CAMERA_FRAMEWORK_V2_H
