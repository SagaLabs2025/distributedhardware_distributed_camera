/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef DISTRIBUTED_CAMERA_SOURCE_H
#define DISTRIBUTED_CAMERA_SOURCE_H

#include <string>
#include <vector>
#include <functional>
#include <memory>

// ===== 数据结构定义 =====

/**
 * 流信息
 * 对应原始代码 DCStreamInfo
 */
struct DCStreamInfo {
    int32_t streamId_;           // 流ID
    int32_t width_;              // 宽度
    int32_t height_;             // 高度
    int32_t frameRate_;          // 帧率
    std::string pixelFormat_;    // 像素格式 (NV12, etc.)

    DCStreamInfo() : streamId_(0), width_(1920), height_(1080), frameRate_(30), pixelFormat_("NV12") {}
    DCStreamInfo(int id, int w, int h, int fps, const std::string& fmt)
        : streamId_(id), width_(w), height_(h), frameRate_(fps), pixelFormat_(fmt) {}
};

/**
 * 捕获信息
 * 对应原始代码 DCCaptureInfo
 */
struct DCCaptureInfo {
    int32_t streamId_;           // 流ID
    bool captureStarted_;        // 是否开始捕获

    DCCaptureInfo() : streamId_(0), captureStarted_(false) {}
    DCCaptureInfo(int id, bool started) : streamId_(id), captureStarted_(started) {}
};

/**
 * 分布式硬件基础信息
 * 对应原始代码 DHBase
 */
struct DHBase {
    std::string deviceId_;       // 设备ID
    std::string dhId_;           // 分布式硬件ID

    DHBase() = default;
    DHBase(const std::string& devId, const std::string& dhId)
        : deviceId_(devId), dhId_(dhId) {}
};

// ===== Source回调接口 =====

/**
 * Source回调接口
 * UI测试程序实现此接口以接收Source端的事件
 */
class ISourceCallback {
public:
    virtual ~ISourceCallback() = default;

    // Source错误回调
    virtual void OnSourceError(int32_t errorCode, const std::string& errorMsg) = 0;

    // 状态变化回调
    virtual void OnSourceStateChanged(const std::string& state) = 0;

    // 解码后的YUV数据回调
    virtual void OnDecodedFrameAvailable(const uint8_t* yuvData,
                                       int width,
                                       int height) = 0;
};

// ===== Source服务接口 =====

/**
 * 分布式相机Source服务接口
 * 对应原始代码的 DCameraSourceDev
 */
class IDistributedCameraSource {
public:
    virtual ~IDistributedCameraSource() = default;

    // 初始化Source服务
    virtual int32_t InitSource(const std::string& params,
                              ISourceCallback* callback) = 0;

    // 释放Source服务
    virtual int32_t ReleaseSource() = 0;

    // 注册分布式硬件（由分布式硬件框架调用）
    virtual int32_t RegisterDistributedHardware(const std::string& devId,
                                                const std::string& dhId) = 0;

    // 注销分布式硬件
    virtual int32_t UnregisterDistributedHardware(const std::string& devId,
                                                  const std::string& dhId) = 0;

    // 【对外导出】启动捕获 - UI测试程序调用此接口
    virtual int32_t StartCapture() = 0;

    // 停止捕获
    virtual int32_t StopCapture() = 0;
};

#endif // DISTRIBUTED_CAMERA_SOURCE_H
