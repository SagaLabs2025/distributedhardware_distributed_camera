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

#ifndef OHOS_PLATFORM_INTERFACE_H
#define OHOS_PLATFORM_INTERFACE_H

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace OHOS {
namespace DistributedHardware {

// Forward declarations
class IDataBuffer;

// ==================== Device Management Interface ====================
struct DeviceInfo {
    std::string networkId;
    std::string udid;
    std::string name;
    int32_t deviceType;
    int32_t deviceTypeId;
};

class IDeviceManager {
public:
    virtual ~IDeviceManager() = default;

    // Get trusted device list
    virtual int32_t GetTrustedDeviceList(const std::string& pkgName,
                                        const std::string& extra,
                                        std::vector<DeviceInfo>& deviceList) = 0;

    // Initialize device manager
    virtual int32_t InitDeviceManager(const std::string& pkgName,
                                     void* dmInitCallback) = 0;

    // Get UDID by network ID
    virtual int32_t GetUdidByNetworkId(const std::string& pkgName,
                                      const std::string& networkId,
                                      std::string& udid) = 0;

    // Check access control
    virtual bool CheckSrcAccessControl(void* caller, void* callee) = 0;
};

// ==================== Hardware Driver Framework Interface ====================
struct CameraCapability {
    std::string cameraId;
    int32_t width;
    int32_t height;
    int32_t fps;
    std::string format;
    std::vector<std::string> supportedFormats;
};

struct StreamConfig {
    int32_t streamId;
    int32_t width;
    int32_t height;
    int32_t fps;
    std::string format;
};

struct CaptureConfig {
    int32_t streamId;
    std::string captureMode;
    std::vector<uint8_t> settings;
};

class IHdfDeviceManager {
public:
    virtual ~IHdfDeviceManager() = default;

    // Load/unload distributed camera HDF
    virtual int32_t LoadDCameraHDF(const std::string& dhId) = 0;
    virtual int32_t UnloadDCameraHDF(const std::string& dhId) = 0;

    // Camera discovery
    virtual int32_t GetCameraIds(std::vector<std::string>& cameraIds) = 0;
    virtual int32_t GetCameraCapabilities(const std::string& cameraId,
                                         CameraCapability& capabilities) = 0;

    // Stream management
    virtual int32_t OpenSession(const std::string& dhId) = 0;
    virtual int32_t CloseSession(const std::string& dhId) = 0;
    virtual int32_t ConfigureStreams(const std::string& dhId,
                                   const std::vector<StreamConfig>& streamConfigs) = 0;
    virtual int32_t ReleaseStreams(const std::string& dhId,
                                  const std::vector<int32_t>& streamIds) = 0;

    // Capture control
    virtual int32_t StartCapture(const std::string& dhId,
                                const std::vector<CaptureConfig>& captureConfigs) = 0;
    virtual int32_t StopCapture(const std::string& dhId,
                               const std::vector<int32_t>& streamIds) = 0;

    // Settings update
    virtual int32_t UpdateSettings(const std::string& dhId,
                                  const std::vector<uint8_t>& settings) = 0;

    // Event notification
    virtual int32_t NotifyEvent(const std::string& dhId,
                               const std::string& eventType,
                               const std::vector<uint8_t>& eventData) = 0;
};

// ==================== Communication Interface ====================
enum class SessionMode {
    CONTROL_SESSION = 0,
    DATA_CONTINUE_SESSION = 1,
    DATA_SNAPSHOT_SESSION = 2
};

enum class TransDataType {
    BYTES = 0,
    MESSAGE = 1,
    STREAM = 2
};

struct PeerInfo {
    std::string deviceId;
    std::string sessionName;
    int32_t socketId;
};

class ICommunicationAdapter {
public:
    virtual ~ICommunicationAdapter() = default;

    // Server operations
    virtual int32_t CreateServer(const std::string& sessionName,
                                SessionMode mode,
                                const std::string& peerDevId,
                                const std::string& peerSessionName) = 0;

    // Client operations
    virtual int32_t CreateClient(const std::string& myDhId,
                                const std::string& myDevId,
                                const std::string& peerSessionName,
                                const std::string& peerDevId,
                                SessionMode mode) = 0;

    // Session management
    virtual int32_t DestroyServer(const std::string& sessionName) = 0;
    virtual int32_t CloseSession(int32_t socketId) = 0;

    // Data transmission
    virtual int32_t SendBytes(int32_t socketId,
                             std::shared_ptr<IDataBuffer> buffer) = 0;
    virtual int32_t SendStream(int32_t socketId,
                              std::shared_ptr<IDataBuffer> buffer) = 0;

    // Network information
    virtual int32_t GetLocalNetworkId(std::string& myDevId) = 0;

    // Event callbacks
    virtual void OnBind(int32_t socketId, const PeerInfo& peerInfo) = 0;
    virtual void OnShutDown(int32_t socketId, int32_t reason) = 0;
    virtual void OnBytesReceived(int32_t socketId,
                               const uint8_t* data,
                               uint32_t dataLen) = 0;
    virtual void OnMessageReceived(int32_t socketId,
                                 const uint8_t* data,
                                 uint32_t dataLen) = 0;
    virtual void OnStreamReceived(int32_t socketId,
                                const uint8_t* data,
                                uint32_t dataLen,
                                const uint8_t* extData,
                                uint32_t extLen) = 0;
};

// ==================== Codec Interface ====================
enum class VideoCodecType {
    H264 = 0,
    H265 = 1,
    VP8 = 2,
    VP9 = 3
};

enum class VideoPixelFormat {
    NV12 = 0,
    NV21 = 1,
    YUV420P = 2,
    RGB32 = 3,
    RGBA = 4
};

struct VideoConfig {
    int32_t width;
    int32_t height;
    int32_t fps;
    VideoCodecType codecType;
    VideoPixelFormat pixelFormat;
    int64_t bitrate; // bits per second
    int32_t keyFrameInterval; // in frames
};

struct CodecBufferInfo {
    uint32_t index;
    int32_t offset;
    int32_t size;
    int64_t presentationTimestamp; // microseconds
    bool isKeyFrame;
};

class IVideoEncoder {
public:
    virtual ~IVideoEncoder() = default;

    virtual int32_t Init(const VideoConfig& config) = 0;
    virtual int32_t Configure(const VideoConfig& config) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Release() = 0;

    virtual int32_t FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                                   int64_t timestampUs) = 0;
    virtual int32_t GetOutputBuffer(CodecBufferInfo& bufferInfo,
                                   std::shared_ptr<IDataBuffer>& outputBuffer) = 0;

    // Callbacks
    virtual void SetErrorCallback(std::function<void()> onError) = 0;
    virtual void SetInputBufferAvailableCallback(
        std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) = 0;
    virtual void SetOutputFormatChangedCallback(
        std::function<void(const VideoConfig&)> onFormatChanged) = 0;
    virtual void SetOutputBufferAvailableCallback(
        std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) = 0;
};

class IVideoDecoder {
public:
    virtual ~IVideoDecoder() = default;

    virtual int32_t Init(const VideoConfig& config) = 0;
    virtual int32_t Configure(const VideoConfig& config) = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t Release() = 0;

    virtual int32_t FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                                   int64_t timestampUs) = 0;
    virtual int32_t GetOutputBuffer(CodecBufferInfo& bufferInfo,
                                   std::shared_ptr<IDataBuffer>& outputBuffer) = 0;

    // Callbacks
    virtual void SetErrorCallback(std::function<void()> onError) = 0;
    virtual void SetInputBufferAvailableCallback(
        std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) = 0;
    virtual void SetOutputFormatChangedCallback(
        std::function<void(const VideoConfig&)> onFormatChanged) = 0;
    virtual void SetOutputBufferAvailableCallback(
        std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) = 0;
};

// ==================== Data Buffer Interface ====================
class IDataBuffer {
public:
    virtual ~IDataBuffer() = default;
    virtual uint8_t* Data() = 0;
    virtual const uint8_t* ConstData() const = 0;
    virtual size_t Size() const = 0;
    virtual void Resize(size_t newSize) = 0;
    virtual bool IsValid() const = 0;
};

// ==================== Factory Interfaces ====================
class IPlatformFactory {
public:
    virtual ~IPlatformFactory() = default;

    virtual std::shared_ptr<IDeviceManager> CreateDeviceManager() = 0;
    virtual std::shared_ptr<IHdfDeviceManager> CreateHdfDeviceManager() = 0;
    virtual std::shared_ptr<ICommunicationAdapter> CreateCommunicationAdapter() = 0;
    virtual std::shared_ptr<IVideoEncoder> CreateVideoEncoder() = 0;
    virtual std::shared_ptr<IVideoDecoder> CreateVideoDecoder() = 0;
    virtual std::shared_ptr<IDataBuffer> CreateDataBuffer(size_t initialSize = 0) = 0;
};

// Global factory instance
extern std::shared_ptr<IPlatformFactory> g_platformFactory;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_PLATFORM_INTERFACE_H