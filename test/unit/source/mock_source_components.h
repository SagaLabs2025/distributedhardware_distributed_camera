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

/**
 * @file mock_source_components.h
 * @brief Source端测试用Mock组件定义
 *
 * 本文件包含所有用于Source端测试的Mock类定义，包括：
 * - Mock分布式硬件框架组件
 * - Mock HDF接口
 * - Mock Surface接口
 * - Mock相机框架接口
 */

#ifndef OHOS_MOCK_SOURCE_COMPONENTS_H
#define OHOS_MOCK_SOURCE_COMPONENTS_H

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <cstdint>

#include "iremote_broker.h"
#include "iremote_object.h"

namespace OHOS {
namespace DistributedHardware {

// ============================================
// 前向声明
// ============================================
class IDCameraSourceCallback;

// ============================================
// 常量定义
// ============================================
constexpr int32_t MEDIA_OK = 0;
constexpr int32_t MEDIA_ERROR = -1;

enum DCameraState {
    DCAMERA_STATE_IDLE = 0,
    DCAMERA_STATE_OPEN = 1,
    DCAMERA_STATE_CONFIG = 2,
    DCAMERA_STATE_CAPTURE = 3,
    DCAMERA_STATE_CLOSED = 4
};

enum DCStreamType {
    CONTROL = 0,
    SNAPSHOT_FRAME = 1,
    CONTINUOUS_FRAME = 2
};

enum DCEncodeType {
    ENCODE_TYPE_H264 = 0,
    ENCODE_TYPE_H265 = 1,
    ENCODE_TYPE_JPEG = 2
};

enum CameraEventType {
    CAMERA_EVENT_OPEN = 0,
    CAMERA_EVENT_CLOSE = 1,
    CAMERA_EVENT_ERROR = 2
};

constexpr int32_t DISTRIBUTED_HARDWARE_CAMERA_SOURCE_SA_ID = 4803;

// ============================================
// 数据结构定义
// ============================================

struct DCStreamInfo {
    int32_t streamId_;
    int32_t width_;
    int32_t height_;
    int32_t stride_;
    int32_t format_;
    int32_t dataspace_;
    DCEncodeType encodeType_;
    DCStreamType type_;
};

struct DCCaptureInfo {
    std::vector<int> streamIds_;
    int32_t width_;
    int32_t height_;
    int32_t stride_;
    int32_t format_;
    int32_t dataspace_;
    DCEncodeType encodeType_;
    DCStreamType type_;
};

struct DCameraSettings {
    int32_t type_;
    std::string value_;
};

struct DCameraIndex {
    std::string devId_;
    std::string dhId_;
};

struct DCameraEvent {
    int32_t type_;
    int32_t result_;
    std::string content_;
};

struct EnableParam {
    std::string sinkVersion;
    std::string sinkAttrs;
    std::string sourceAttrs;
    std::string sourceVersion;
};

struct DCameraChannelInfo {
    std::string channelId;
    int32_t channelType;
};

struct DCameraInfo {
    std::string dhId;
    std::string cameraName;
    int32_t cameraPosition;
};

struct DCameraOpenInfo {
    std::string dhId;
    int32_t width;
    int32_t height;
};

struct DCameraRegistParam {
    std::string devId;
    std::string dhId;
    std::string reqId;
    std::string sinkParams;
    std::string srcParams;

    DCameraRegistParam(const std::string& d, const std::string& dh,
                      const std::string& r, const std::string& sp,
                      const std::string& spp)
        : devId(d), dhId(dh), reqId(r), sinkParams(sp), srcParams(spp) {}
};

struct WorkModeParam {
    int32_t mode;
    int32_t param;
    int32_t param1;
    bool param2;

    WorkModeParam(int32_t m, int32_t p, int32_t p1, bool p2)
        : mode(m), param(p), param1(p1), param2(p2) {}
};

struct CameraEvent {
    CameraEventType type;
    int32_t result;
    std::string message;
};

// ============================================
// Mock分布式硬件框架回调接口
// ============================================

class IDCameraSourceCallback : public IRemoteBroker {
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"ohos.distributedhardware.dcamerasourcecallback");

    virtual ~IDCameraSourceCallback() = default;

    virtual int32_t OnRegisterResult(const std::string& devId,
                                     const std::string& dhId,
                                     int32_t result) = 0;
    virtual int32_t OnUnregisterResult(const std::string& devId,
                                       const std::string& dhId,
                                       int32_t result) = 0;
    virtual int32_t OnCameraEvent(const std::string& devId,
                                  const std::string& dhId,
                                  const std::string& event) = 0;
};

// ============================================
// Mock HDF Provider回调接口
// ============================================

namespace HDI {
namespace Camera {
namespace V1_1 {

struct CameraMetadata {
    int32_t width;
    int32_t height;
    int32_t format;
};

class IDCameraProviderCallback : public IRemoteBroker {
public:
    virtual ~IDCameraProviderCallback() = default;

    virtual int32_t OnCameraEvent(const std::string& dhId,
                                  const CameraEvent& event) = 0;
};

} // namespace V1_1
} // namespace Camera
} // namespace HDI

// ============================================
// Mock相机控制器接口
// ============================================

class ICameraController {
public:
    virtual ~ICameraController() = default;

    virtual int32_t Init(std::vector<DCameraIndex>& indexs) = 0;
    virtual int32_t UnInit() = 0;
    virtual int32_t StartCapture(std::vector<std::shared_ptr<DCCaptureInfo>>& captureInfos,
                                 int32_t sceneMode) = 0;
    virtual int32_t StopCapture() = 0;
    virtual int32_t ChannelNeg(std::shared_ptr<DCameraChannelInfo>& info) = 0;
    virtual int32_t DCameraNotify(std::shared_ptr<DCameraEvent>& events) = 0;
    virtual int32_t UpdateSettings(std::vector<std::shared_ptr<DCameraSettings>>& settings) = 0;
    virtual int32_t GetCameraInfo(std::shared_ptr<DCameraInfo>& camInfo) = 0;
    virtual int32_t OpenChannel(std::shared_ptr<DCameraOpenInfo>& openInfo) = 0;
    virtual int32_t CloseChannel() = 0;
    virtual int32_t PauseDistributedHardware(const std::string &networkId) = 0;
    virtual int32_t ResumeDistributedHardware(const std::string &networkId) = 0;
    virtual int32_t StopDistributedHardware(const std::string &networkId) = 0;
    virtual void SetTokenId(uint64_t token) = 0;
};

// ============================================
// Mock相机输入接口
// ============================================

class ICameraInput {
public:
    virtual ~ICameraInput() = default;

    virtual int32_t Init() = 0;
    virtual int32_t Release() = 0;
    virtual int32_t StartCapture() = 0;
    virtual int32_t StopCapture() = 0;
    virtual void SetCameraCallback(sptr<HDI::Camera::V1_1::IDCameraProviderCallback>& callback) = 0;
    virtual int32_t UpdateSettings(std::shared_ptr<DCameraSettings>& setting) = 0;
};

// ============================================
// Mock相机状态监听器接口
// ============================================

class ICameraStateListener {
public:
    virtual ~ICameraStateListener() = default;

    virtual int32_t OnCameraStateChange(std::shared_ptr<DCameraIndex>& index,
                                        const DCameraState& state) = 0;
};

// ============================================
// Mock分布式硬件框架套件
// ============================================

class DistributedHardwareFwkKit {
public:
    virtual ~DistributedHardwareFwkKit() = default;

    virtual int32_t RegisterPublisher(const std::string& dhId,
                                     const sptr<IRemoteObject>& callback) = 0;
    virtual int32_t UnregisterPublisher(const std::string& dhId) = 0;
    virtual int32_t QueryDeviceInfo(const std::string& dhId,
                                    std::string& info) = 0;
};

// ============================================
// Smart pointer类型定义
// ============================================
template<typename T>
using sptr = std::shared_ptr<T>;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_SOURCE_COMPONENTS_H
