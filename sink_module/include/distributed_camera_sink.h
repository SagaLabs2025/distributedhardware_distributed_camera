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

#ifndef DISTRIBUTED_CAMERA_SINK_H
#define DISTRIBUTED_CAMERA_SINK_H

#include <string>
#include <memory>

// ===== 前向声明 =====

// Sink回调接口 - UI测试程序实现 (仅用于错误通知)
class ISinkCallback {
public:
    virtual ~ISinkCallback() = default;

    // Sink错误回调
    virtual void OnSinkError(int32_t errorCode, const std::string& errorMsg) = 0;
};

// ===== Sink服务接口 =====

// Sink服务接口 - 最小化接口，不导出启动/停止捕获
// 根据原始代码分析：Sink端参数由Source端通过SoftBus消息传递
class IDistributedCameraSink {
public:
    virtual ~IDistributedCameraSink() = default;

    // 初始化Sink服务
    virtual int32_t InitSink(const std::string& params,
                            ISinkCallback* callback) = 0;

    // 释放Sink服务
    virtual int32_t ReleaseSink() = 0;

    // 【测试环境专用】启动捕获（模拟Source端发送SoftBus消息）
    virtual int32_t StartCapture(const std::string& dhId, int width, int height) = 0;

    // 【测试环境专用】停止捕获
    virtual int32_t StopCapture(const std::string& dhId) = 0;

    // 【内部方法，不对外导出】
    // 在真实环境中，StartCapture() 由 Source 端通过 SoftBus 消息触发
    // 对应原始代码：dcamera_sink_data_process.cpp:72
    // DHLOGI("StartCapture dhId: %{public}s, width: %{public}d...")
    //
    // StopCapture() 由 Source 端通过 SoftBus 消息触发
    //
    // 编码参数由 Source 端通过 SoftBus 消息传递
};

// ===== 导出工厂函数 =====

extern "C" __declspec(dllexport)
void* CreateSinkService();

extern "C" __declspec(dllexport)
void DestroySinkService(void* instance);

extern "C" __declspec(dllexport)
const char* GetSinkVersion();

#endif // DISTRIBUTED_CAMERA_SINK_H
