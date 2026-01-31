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

#ifndef HDF_MOCK_DRIVER_H
#define HDF_MOCK_DRIVER_H

#include "distributed_camera_source.h"
#include <memory>

// 前向声明
class SocketAdapter;

// HDF返回码
enum DCamRetCode {
    SUCCESS = 0,
    FAILED = 7,
};

// HDF模拟驱动 - 模拟原始代码 DCameraProviderCallbackImpl 的行为
// 对应原始文件: dcamera_provider_callback_impl.cpp
class HDFMockDriver {
public:
    HDFMockDriver(ISourceCallback* uiCallback, SocketAdapter* socketAdapter);
    ~HDFMockDriver();

    // ===== 模拟 DCameraProviderCallbackImpl 的回调实现 =====

    // 对应原始代码: dcamera_provider_callback_impl.cpp:40
    int32_t OpenSession(const DHBase& dhBase);

    // 对应原始代码: dcamera_provider_callback_impl.cpp:100
    int32_t ConfigureStreams(const DHBase& dhBase,
                             const std::vector<DCStreamInfo>& streamInfos);

    // 对应原始代码: dcamera_provider_callback_impl.cpp:180
    int32_t StartCapture(const DHBase& dhBase,
                         const std::vector<DCCaptureInfo>& captureInfos);

    int32_t StopCapture(const DHBase& dhBase,
                        const std::vector<int>& streamIds);

private:
    bool CheckDHBase(const DHBase& dhBase);
    bool CheckStreamInfo(const DCStreamInfo& stream);
    bool CheckCaptureInfo(const DCCaptureInfo& captureInfo);

    ISourceCallback* uiCallback_;
    SocketAdapter* socketAdapter_;
};

#endif // HDF_MOCK_DRIVER_H
