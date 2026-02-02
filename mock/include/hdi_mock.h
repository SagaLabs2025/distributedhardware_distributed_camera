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

#ifndef OHOS_HDI_MOCK_H
#define OHOS_HDI_MOCK_H

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include "buffer_handle.h"

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief HDF返回码枚举
 */
enum class DCamRetCode : int32_t {
    SUCCESS = 0,
    CAMERA_BUSY = 1,
    INVALID_ARGUMENT = 2,
    METHOD_NOT_SUPPORTED = 3,
    CAMERA_OFFLINE = 4,
    EXCEED_MAX_NUMBER = 5,
    DEVICE_NOT_INIT = 6,
    FAILED = 7,
};

/**
 * @brief 编码类型枚举
 */
enum class DCEncodeType : int32_t {
    ENCODE_TYPE_NULL = 0,
    ENCODE_TYPE_H264 = 1,
    ENCODE_TYPE_H265 = 2,
    ENCODE_TYPE_JPEG = 3,
    ENCODE_TYPE_MPEG4_ES = 4,
};

/**
 * @brief 流类型枚举
 */
enum class DCStreamType : int32_t {
    CONTINUOUS_FRAME = 0,  // 连续捕获流（预览/视频）
    SNAPSHOT_FRAME = 1,    // 单次捕获流（拍照）
};

/**
 * @brief 设置类型枚举
 */
enum class DCSettingsType : int32_t {
    UPDATE_METADATA = 0,
    ENABLE_METADATA = 1,
    DISABLE_METADATA = 2,
    METADATA_RESULT = 3,
    SET_FLASH_LIGHT = 4,
    FPS_RANGE = 5,
    UPDATE_FRAME_METADATA = 6,
};

/**
 * @brief 分布式硬件基础信息
 */
struct DHBase {
    std::string deviceId_;
    std::string dhId_;
};

/**
 * @brief 相机设置结构
 */
struct DCameraSettings {
    DCSettingsType type_;
    std::string value_;
};

/**
 * @brief 流信息结构
 */
struct DCStreamInfo {
    int streamId_;
    int width_;
    int height_;
    int stride_;
    int format_;
    int dataspace_;
    DCEncodeType encodeType_;
    DCStreamType type_;

    DCStreamInfo() : streamId_(0), width_(0), height_(0), stride_(0),
                     format_(0), dataspace_(0),
                     encodeType_(DCEncodeType::ENCODE_TYPE_NULL),
                     type_(DCStreamType::CONTINUOUS_FRAME) {}

    DCStreamInfo(int id, int w, int h, DCEncodeType enc, DCStreamType t)
        : streamId_(id), width_(w), height_(h), stride_(w),
          format_(0), dataspace_(0), encodeType_(enc), type_(t) {}
};

/**
 * @brief 捕获信息结构
 */
struct DCCaptureInfo {
    std::vector<int> streamIds_;
    int width_;
    int height_;
    int stride_;
    int format_;
    int dataspace_;
    bool isCapture_;
    DCEncodeType encodeType_;
    DCStreamType type_;
    std::vector<DCameraSettings> captureSettings_;

    DCCaptureInfo() : width_(0), height_(0), stride_(0), format_(0),
                      dataspace_(0), isCapture_(false),
                      encodeType_(DCEncodeType::ENCODE_TYPE_NULL),
                      type_(DCStreamType::CONTINUOUS_FRAME) {}
};

/**
 * @brief HDF事件结构
 */
struct DCameraHDFEvent {
    int type_;
    int result_;
    std::string content_;

    DCameraHDFEvent() : type_(0), result_(0) {}
    DCameraHDFEvent(int t, int r, const std::string& c = "")
        : type_(t), result_(r), content_(c) {}
};

/**
 * @brief 相机缓冲区结构
 */
struct DCameraBuffer {
    int index_;                      // 缓冲区索引
    unsigned int size_;              // 缓冲区大小
    BufferHandle* bufferHandle_;     // Native缓冲区句柄
    void* virAddr_;                  // 虚拟地址（零拷贝使用）

    DCameraBuffer() : index_(0), size_(0), bufferHandle_(nullptr), virAddr_(nullptr) {}
};

/**
 * @brief HDI Provider回调接口
 */
class IDCameraProviderCallback {
public:
    virtual ~IDCameraProviderCallback() = default;

    virtual int32_t OpenSession(const DHBase& dhBase) = 0;
    virtual int32_t CloseSession(const DHBase& dhBase) = 0;
    virtual int32_t ConfigureStreams(const DHBase& dhBase,
                                     const std::vector<DCStreamInfo>& streamInfos) = 0;
    virtual int32_t ReleaseStreams(const DHBase& dhBase,
                                   const std::vector<int>& streamIds) = 0;
    virtual int32_t StartCapture(const DHBase& dhBase,
                                 const std::vector<DCCaptureInfo>& captureInfos) = 0;
    virtual int32_t StopCapture(const DHBase& dhBase,
                                const std::vector<int>& streamIds) = 0;
    virtual int32_t UpdateSettings(const DHBase& dhBase,
                                   const std::vector<DCameraSettings>& settings) = 0;
};

/**
 * @brief HDI Provider接口
 */
class IDCameraProvider {
public:
    virtual ~IDCameraProvider() = default;

    virtual int32_t EnableDCameraDevice(const DHBase& dhBase,
                                        const std::string& abilityInfo,
                                        const std::shared_ptr<IDCameraProviderCallback>& callbackObj) = 0;
    virtual int32_t DisableDCameraDevice(const DHBase& dhBase) = 0;
    virtual int32_t AcquireBuffer(const DHBase& dhBase, int streamId, DCameraBuffer& buffer) = 0;
    virtual int32_t ShutterBuffer(const DHBase& dhBase, int streamId, const DCameraBuffer& buffer) = 0;
    virtual int32_t OnSettingsResult(const DHBase& dhBase, const DCameraSettings& result) = 0;
    virtual int32_t Notify(const DHBase& dhBase, const DCameraHDFEvent& event) = 0;
};

/**
 * @brief Mock HDI Provider实现
 * 用于测试虚拟相机HDF设备
 */
class MockHdiProvider : public IDCameraProvider {
public:
    static std::shared_ptr<MockHdiProvider> GetInstance();

    // IDCameraProvider接口实现
    int32_t EnableDCameraDevice(const DHBase& dhBase,
                                const std::string& abilityInfo,
                                const std::shared_ptr<IDCameraProviderCallback>& callbackObj) override;
    int32_t DisableDCameraDevice(const DHBase& dhBase) override;
    int32_t AcquireBuffer(const DHBase& dhBase, int streamId, DCameraBuffer& buffer) override;
    int32_t ShutterBuffer(const DHBase& dhBase, int streamId, const DCameraBuffer& buffer) override;
    int32_t OnSettingsResult(const DHBase& dhBase, const DCameraSettings& result) override;
    int32_t Notify(const DHBase& dhBase, const DCameraHDFEvent& event) override;

    // 测试控制方法
    void SetEnableResult(int32_t result);
    void SetAcquireBufferResult(int32_t result);
    void SetShutterBufferResult(int32_t result);
    void Reset();

    // 触发回调方法（用于模拟HDF驱动事件）
    int32_t TriggerOpenSession(const DHBase& dhBase);
    int32_t TriggerCloseSession(const DHBase& dhBase);
    int32_t TriggerConfigureStreams(const DHBase& dhBase,
                                    const std::vector<DCStreamInfo>& streamInfos);
    int32_t TriggerReleaseStreams(const DHBase& dhBase,
                                  const std::vector<int>& streamIds);
    int32_t TriggerStartCapture(const DHBase& dhBase,
                                const std::vector<DCCaptureInfo>& captureInfos);
    int32_t TriggerStopCapture(const DHBase& dhBase,
                               const std::vector<int>& streamIds);
    int32_t TriggerUpdateSettings(const DHBase& dhBase,
                                  const std::vector<DCameraSettings>& settings);

    // 状态查询方法
    bool IsDeviceEnabled(const std::string& dhId) const;
    size_t GetActiveStreamCount() const;
    size_t GetBufferAcquireCount() const;
    size_t GetBufferShutterCount() const;
    std::vector<int> GetConfiguredStreamIds() const;

    // 缓冲区管理（零拷贝实现）
    void* GetBufferData(const DCameraBuffer& buffer);
    size_t GetBufferSize(const DCameraBuffer& buffer);

private:
    MockHdiProvider() = default;
    ~MockHdiProvider() = default;

    // 创建模拟的Buffer
    DCameraBuffer CreateMockBuffer(int streamId, size_t size);
    void ReleaseMockBuffer(const DCameraBuffer& buffer);

    struct StreamState {
        DCStreamInfo streamInfo;
        bool isActive;
        size_t bufferCount;
        size_t maxBuffers;
    };

    std::mutex mutex_;
    std::map<std::string, bool> enabledDevices_;
    std::shared_ptr<IDCameraProviderCallback> callback_;
    std::map<int, StreamState> streams_;
    std::atomic<int> nextBufferIndex_{0};

    // 测试控制变量
    int32_t enableResult_{static_cast<int32_t>(DCamRetCode::SUCCESS)};
    int32_t acquireBufferResult_{static_cast<int32_t>(DCamRetCode::SUCCESS)};
    int32_t shutterBufferResult_{static_cast<int32_t>(DCamRetCode::SUCCESS)};

    // 统计信息
    std::atomic<size_t> bufferAcquireCount_{0};
    std::atomic<size_t> bufferShutterCount_{0};

    // 零拷贝缓冲区管理
    std::map<int, std::vector<uint8_t>> bufferDataPool_;
    std::mutex bufferPoolMutex_;
};

/**
 * @brief Mock Provider回调实现
 * 用于测试回调机制
 */
class MockProviderCallback : public IDCameraProviderCallback {
public:
    MockProviderCallback();

    // IDCameraProviderCallback接口实现
    int32_t OpenSession(const DHBase& dhBase) override;
    int32_t CloseSession(const DHBase& dhBase) override;
    int32_t ConfigureStreams(const DHBase& dhBase,
                             const std::vector<DCStreamInfo>& streamInfos) override;
    int32_t ReleaseStreams(const DHBase& dhBase,
                           const std::vector<int>& streamIds) override;
    int32_t StartCapture(const DHBase& dhBase,
                         const std::vector<DCCaptureInfo>& captureInfos) override;
    int32_t StopCapture(const DHBase& dhBase,
                        const std::vector<int>& streamIds) override;
    int32_t UpdateSettings(const DHBase& dhBase,
                           const std::vector<DCameraSettings>& settings) override;

    // 测试验证方法
    bool IsSessionOpen() const { return sessionOpen_; }
    bool IsStreamsConfigured() const { return streamsConfigured_; }
    bool IsCaptureStarted() const { return captureStarted_; }
    size_t GetOpenSessionCount() const { return openSessionCount_; }
    size_t GetConfigureStreamsCount() const { return configureStreamsCount_; }
    size_t GetStartCaptureCount() const { return startCaptureCount_; }
    std::vector<DCStreamInfo> GetLastStreamInfos() const { return lastStreamInfos_; }
    std::vector<DCCaptureInfo> GetLastCaptureInfos() const { return lastCaptureInfos_; }

    void Reset();
    void SetCallbackResult(int32_t result);

private:
    std::mutex mutex_;
    std::atomic<size_t> openSessionCount_{0};
    std::atomic<size_t> closeSessionCount_{0};
    std::atomic<size_t> configureStreamsCount_{0};
    std::atomic<size_t> releaseStreamsCount_{0};
    std::atomic<size_t> startCaptureCount_{0};
    std::atomic<size_t> stopCaptureCount_{0};
    std::atomic<size_t> updateSettingsCount_{0};

    bool sessionOpen_{false};
    bool streamsConfigured_{false};
    bool captureStarted_{false};

    std::vector<DCStreamInfo> lastStreamInfos_;
    std::vector<DCCaptureInfo> lastCaptureInfos_;
    int32_t callbackResult_{static_cast<int32_t>(DCamRetCode::SUCCESS)};
};

/**
 * @brief 三通道流配置工具类
 * 用于模拟Control/Snapshot/Continuous三种通道
 */
class TripleStreamConfig {
public:
    static std::vector<DCStreamInfo> CreateDefaultTripleStreams();
    static std::vector<DCStreamInfo> CreateCustomTripleStreams(
        int snapshotWidth, int snapshotHeight,
        int continuousWidth, int continuousHeight);

    static DCStreamInfo CreateControlStream();
    static DCStreamInfo CreateSnapshotStream(int width, int height);
    static DCStreamInfo CreateContinuousStream(int width, int height);

    static constexpr int CONTROL_STREAM_ID = 0;
    static constexpr int SNAPSHOT_STREAM_ID = 1;
    static constexpr int CONTINUOUS_STREAM_ID = 2;

    // 分辨率常量
    static constexpr int SNAPSHOT_MAX_WIDTH = 4096;
    static constexpr int SNAPSHOT_MAX_HEIGHT = 3072;
    static constexpr int CONTINUOUS_MAX_WIDTH = 1920;
    static constexpr int CONTINUOUS_MAX_HEIGHT = 1080;

    // 编码类型
    static constexpr DCEncodeType SNAPSHOT_ENCODE_TYPE = DCEncodeType::ENCODE_TYPE_JPEG;
    static constexpr DCEncodeType CONTINUOUS_ENCODE_TYPE = DCEncodeType::ENCODE_TYPE_H265;
};

/**
 * @brief 缓冲区管理器（零拷贝实现）
 */
class ZeroCopyBufferManager {
public:
    static ZeroCopyBufferManager& GetInstance();

    // 创建/获取缓冲区
    DCameraBuffer CreateBuffer(size_t size);
    DCameraBuffer AcquireBuffer(int streamId, size_t size);
    int32_t ReleaseBuffer(const DCameraBuffer& buffer);

    // 零拷贝数据访问
    void* GetBufferData(const DCameraBuffer& buffer);
    size_t GetBufferSize(const DCameraBuffer& buffer);

    // 设置缓冲区数据
    int32_t SetBufferData(const DCameraBuffer& buffer, const void* data, size_t size);

    // 统计信息
    size_t GetActiveBufferCount() const;
    size_t GetTotalAllocatedSize() const;
    void Reset();

private:
    ZeroCopyBufferManager() = default;
    ~ZeroCopyBufferManager();

    std::mutex mutex_;
    std::map<int, std::vector<uint8_t>> bufferDataMap_;
    std::atomic<int> nextBufferId_{0};
    std::atomic<size_t> totalAllocatedSize_{0};
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_HDI_MOCK_H
