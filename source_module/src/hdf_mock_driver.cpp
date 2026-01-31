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

#include "hdf_mock_driver.h"
#include "socket_adapter.h"

#include "distributed_hardware_log.h"
#include "anonymous_string.h"

HDFMockDriver::HDFMockDriver(ISourceCallback* uiCallback, SocketAdapter* socketAdapter)
    : uiCallback_(uiCallback), socketAdapter_(socketAdapter) {
    DHLOGI("[HDF_MOCK] HDFMockDriver created");
}

HDFMockDriver::~HDFMockDriver() {
    DHLOGI("[HDF_MOCK] HDFMockDriver destroyed");
}

bool HDFMockDriver::CheckDHBase(const DHBase& dhBase) {
    if (dhBase.deviceId_.empty() || dhBase.dhId_.empty()) {
        DHLOGE("[HDF_MOCK] dhBase is invalid");
        return false;
    }
    return true;
}

bool HDFMockDriver::CheckStreamInfo(const DCStreamInfo& stream) {
    if (stream.streamId_ < 0 || stream.width_ < 0 || stream.height_ < 0) {
        DHLOGE("[HDF_MOCK] stream is invalid");
        return false;
    }
    return true;
}

bool HDFMockDriver::CheckCaptureInfo(const DCCaptureInfo& captureInfo) {
    if (captureInfo.streamIds_.empty() || captureInfo.width_ < 0 || captureInfo.height_ < 0) {
        DHLOGE("[HDF_MOCK] captureInfo is invalid");
        return false;
    }
    return true;
}

// 对应原始代码: dcamera_provider_callback_impl.cpp:40-63
int32_t HDFMockDriver::OpenSession(const DHBase& dhBase) {
    DHLOGI("[HDF_MOCK] OpenSession Start, devId: %{public}s dhId: %{public}s",
            GetAnonyString(dhBase.deviceId_).c_str(),
            GetAnonyString(dhBase.dhId_).c_str());

    if (!CheckDHBase(dhBase)) {
        DHLOGE("[HDF_MOCK] OpenSession failed: invalid dhBase");
        return DCamRetCode::FAILED;
    }

    // 建立与Sink端的Socket连接
    if (socketAdapter_->ConnectToSink() != 0) {
        DHLOGE("[HDF_MOCK] Failed to connect to Sink");
        return DCamRetCode::FAILED;
    }

    // 通知UI测试程序
    if (uiCallback_) {
        uiCallback_->OnSourceStateChanged("OPENED");
    }

    DHLOGI("[HDF_MOCK] OpenSession End, devId: %{public}s dhId: %{public}s",
            GetAnonyString(dhBase.deviceId_).c_str(),
            GetAnonyString(dhBase.dhId_).c_str());
    return DCamRetCode::SUCCESS;
}

// 对应原始代码: dcamera_provider_callback_impl.cpp:100-140
int32_t HDFMockDriver::ConfigureStreams(const DHBase& dhBase,
                                        const std::vector<DCStreamInfo>& streamInfos) {
    DHLOGI("[HDF_MOCK] ConfigureStreams devId: %{public}s dhId: %{public}s",
            GetAnonyString(dhBase.deviceId_).c_str(),
            GetAnonyString(dhBase.dhId_).c_str());

    if (!CheckDHBase(dhBase) || streamInfos.empty()) {
        DHLOGE("[HDF_MOCK] ConfigureStreams failed: invalid input");
        return DCamRetCode::FAILED;
    }

    for (const auto& stream : streamInfos) {
        if (!CheckStreamInfo(stream)) {
            DHLOGE("[HDF_MOCK] streamInfo is invalid");
            return DCamRetCode::FAILED;
        }
    }

    // 将配置信息发送给Sink端 (通过SoftBus消息)
    if (socketAdapter_->SendConfigureMessage(streamInfos) != 0) {
        DHLOGE("[HDF_MOCK] Failed to send configure message");
        return DCamRetCode::FAILED;
    }

    // 通知UI测试程序
    if (uiCallback_) {
        uiCallback_->OnSourceStateChanged("CONFIGURED");
    }

    return DCamRetCode::SUCCESS;
}

// 对应原始代码: dcamera_provider_callback_impl.cpp:180-227
int32_t HDFMockDriver::StartCapture(const DHBase& dhBase,
                                    const std::vector<DCCaptureInfo>& captureInfos) {
    DHLOGI("[HDF_MOCK] StartCapture devId: %{public}s dhId: %{public}s",
            GetAnonyString(dhBase.deviceId_).c_str(),
            GetAnonyString(dhBase.dhId_).c_str());

    if (!CheckDHBase(dhBase) || captureInfos.empty()) {
        DHLOGE("[HDF_MOCK] StartCapture failed: invalid input");
        return DCamRetCode::FAILED;
    }

    for (const auto& captureInfo : captureInfos) {
        if (!CheckCaptureInfo(captureInfo)) {
            DHLOGE("[HDF_MOCK] captureInfo is invalid");
            return DCamRetCode::FAILED;
        }
    }

    // 【关键】发送启动捕获消息给Sink端 (通过SoftBus消息)
    // Sink端接收后触发原始代码: dcamera_sink_data_process.cpp:72
    // DHLOGI("StartCapture dhId: %{public}s, width: %{public}d...")
    if (socketAdapter_->SendStartCaptureMessage(captureInfos) != 0) {
        DHLOGE("[HDF_MOCK] Failed to send start capture message");
        return DCamRetCode::FAILED;
    }

    // 开始接收Sink端的编码数据
    if (socketAdapter_->StartReceiving() != 0) {
        DHLOGE("[HDF_MOCK] Failed to start receiving");
        return DCamRetCode::FAILED;
    }

    // 通知UI测试程序
    if (uiCallback_) {
        uiCallback_->OnSourceStateChanged("CAPTURING");
    }

    DHLOGI("[HDF_MOCK] StartCapture success");
    return DCamRetCode::SUCCESS;
}

int32_t HDFMockDriver::StopCapture(const DHBase& dhBase,
                                   const std::vector<int>& streamIds) {
    DHLOGI("[HDF_MOCK] StopCapture devId: %{public}s dhId: %{public}s",
            GetAnonyString(dhBase.deviceId_).c_str(),
            GetAnonyString(dhBase.dhId_).c_str());

    if (!CheckDHBase(dhBase)) {
        DHLOGE("[HDF_MOCK] StopCapture failed: invalid dhBase");
        return DCamRetCode::FAILED;
    }

    // 停止接收
    socketAdapter_->StopReceiving();

    // 发送停止消息给Sink端
    socketAdapter_->SendStopCaptureMessage();

    // 通知UI测试程序
    if (uiCallback_) {
        uiCallback_->OnSourceStateChanged("STOPPED");
    }

    return DCamRetCode::SUCCESS;
}
