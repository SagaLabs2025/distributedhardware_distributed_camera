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
 * @file softbus_mock_integration.cpp
 *
 * @brief SoftBus Mock与分布式相机集成示例
 *
 * 本示例展示如何使用SoftBus Mock来测试分布式相机的通道功能。
 * 演示了三通道（Control/Snapshot/Continuous）的建立和使用。
 */

#include "softbus_mock.h"
#include "socket.h"
#include "trans_type.h"
#include "distributed_hardware_log.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>

using namespace OHOS::DistributedHardware;

// 集成测试类 - 模拟分布式相机的Source和Sink端
class DCameraSoftbusIntegration {
public:
    struct ChannelInfo {
        int32_t socket;
        std::string sourceName;
        std::string sinkName;
        SoftbusChannelType type;
    };

    DCameraSoftbusIntegration()
        : initialized_(false)
        , sourceControlSocket_(-1)
        , sourceSnapshotSocket_(-1)
        , sourceContinuousSocket_(-1)
        , sinkControlSocket_(-1)
        , sinkSnapshotSocket_(-1)
        , sinkContinuousSocket_(-1) {
    }

    ~DCameraSoftbusIntegration() {
        Cleanup();
    }

    // 初始化集成环境
    bool Initialize() {
        DHLOGI("Initializing DCamera SoftBus Integration");

        // 配置SoftBus Mock
        SoftbusMockConfig config;
        config.localIp = "127.0.0.1";
        config.basePort = 52000;
        config.maxSockets = 64;
        config.receiveBufferSize = 4 * 1024 * 1024;  // 4MB
        config.sendBufferSize = 4 * 1024 * 1024;     // 4MB
        config.enableDataCheck = true;
        config.socketTimeout = 30000;

        int32_t result = SoftbusMock::GetInstance().Initialize(config);
        if (result != 0) {
            DHLOGE("Failed to initialize SoftBus Mock");
            return false;
        }

        initialized_ = true;
        DHLOGI("DCamera SoftBus Integration initialized successfully");
        return true;
    }

    // 创建Sink端（被控端）服务器
    bool CreateSinkServers() {
        DHLOGI("Creating Sink servers");

        // 创建控制通道服务器
        if (!CreateSinkServer("DCameraSinkControl", CHANNEL_TYPE_CONTROL,
                              sinkControlSocket_)) {
            DHLOGE("Failed to create control channel server");
            return false;
        }

        // 创建抓拍通道服务器
        if (!CreateSinkServer("DCameraSinkSnapshot", CHANNEL_TYPE_SNAPSHOT,
                              sinkSnapshotSocket_)) {
            DHLOGE("Failed to create snapshot channel server");
            return false;
        }

        // 创建连续通道服务器
        if (!CreateSinkServer("DCameraSinkContinuous", CHANNEL_TYPE_CONTINUOUS,
                              sinkContinuousSocket_)) {
            DHLOGE("Failed to create continuous channel server");
            return false;
        }

        DHLOGI("All Sink servers created successfully");
        return true;
    }

    // 创建Source端（主控端）客户端
    bool CreateSourceClients() {
        DHLOGI("Creating Source clients");

        // 创建控制通道客户端
        if (!CreateSourceClient("DCameraSourceControl", "DCameraSinkControl",
                                CHANNEL_TYPE_CONTROL, sourceControlSocket_)) {
            DHLOGE("Failed to create control channel client");
            return false;
        }

        // 创建抓拍通道客户端
        if (!CreateSourceClient("DCameraSourceSnapshot", "DCameraSinkSnapshot",
                                CHANNEL_TYPE_SNAPSHOT, sourceSnapshotSocket_)) {
            DHLOGE("Failed to create snapshot channel client");
            return false;
        }

        // 创建连续通道客户端
        if (!CreateSourceClient("DCameraSourceContinuous", "DCameraSinkContinuous",
                                CHANNEL_TYPE_CONTINUOUS, sourceContinuousSocket_)) {
            DHLOGE("Failed to create continuous channel client");
            return false;
        }

        DHLOGI("All Source clients created successfully");
        return true;
    }

    // 测试控制通道
    bool TestControlChannel() {
        DHLOGI("Testing Control Channel");

        const char* controlCmd = "{\"cmd\":\"start_preview\",\"params\":{}}";
        int32_t result = SendBytes(sourceControlSocket_, controlCmd,
                                   static_cast<uint32_t>(std::strlen(controlCmd)) + 1);

        if (result <= 0) {
            DHLOGE("Failed to send control command");
            return false;
        }

        DHLOGI("Control command sent successfully");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return true;
    }

    // 测试抓拍通道
    bool TestSnapshotChannel() {
        DHLOGI("Testing Snapshot Channel");

        // 模拟发送拍照数据
        const char* snapshotData = "MOCK_SNAPSHOT_IMAGE_DATA";
        int32_t result = SendBytes(sourceSnapshotSocket_, snapshotData,
                                   static_cast<uint32_t>(std::strlen(snapshotData)) + 1);

        if (result <= 0) {
            DHLOGE("Failed to send snapshot data");
            return false;
        }

        DHLOGI("Snapshot data sent successfully");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return true;
    }

    // 测试连续通道（视频流）
    bool TestContinuousChannel() {
        DHLOGI("Testing Continuous Channel (Video Stream)");

        // 发送多帧视频数据
        for (int i = 0; i < 10; i++) {
            // 模拟一帧视频数据
            const size_t frameSize = 1024;
            char* frameData = new char[frameSize];
            std::memset(frameData, 0xAA, frameSize);

            StreamData streamData = {};
            streamData.buf = frameData;
            streamData.bufLen = frameSize;

            StreamFrameInfo frameInfo = {};
            frameInfo.frameType = (i == 0) ? 1 : 2;  // 第一帧为I帧，其他为P帧
            frameInfo.timeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            frameInfo.seqNum = i;
            frameInfo.seqSubNum = 0;
            frameInfo.level = 0;
            frameInfo.bitMap = 0;
            frameInfo.tvCount = 0;
            frameInfo.tvList = nullptr;

            int32_t result = SendStream(sourceContinuousSocket_, &streamData,
                                        nullptr, &frameInfo);
            delete[] frameData;

            if (result != 0) {
                DHLOGE("Failed to send video frame %d", i);
                return false;
            }

            DHLOGI("Video frame %d sent successfully (type=%d)", i, frameInfo.frameType);

            // 模拟帧率控制
            std::this_thread::sleep_for(std::chrono::milliseconds(33));  // ~30fps
        }

        DHLOGI("Continuous channel test completed");
        return true;
    }

    // 获取统计信息
    void PrintStatistics() {
        const auto& stats = SoftbusMock::GetInstance().GetStatistics();

        DHLOGI("========== DCamera Communication Statistics ==========");
        DHLOGI("Total Bytes Sent:      %llu bytes", stats.totalBytesSent);
        DHLOGI("Total Bytes Received:  %llu bytes", stats.totalBytesReceived);
        DHLOGI("Total Packets Sent:    %llu", stats.totalPacketsSent);
        DHLOGI("Total Packets Rcvd:    %llu", stats.totalPacketsReceived);
        DHLOGI("Active Connections:    %u", stats.activeConnections);
        DHLOGI("Total Sockets Created: %u", stats.totalSocketsCreated);
        DHLOGI("Send Errors:           %llu", stats.sendErrors);
        DHLOGI("Receive Errors:        %llu", stats.receiveErrors);
        DHLOGI("==================================================");
    }

    // 清理资源
    void Cleanup() {
        if (!initialized_) {
            return;
        }

        DHLOGI("Cleaning up DCamera SoftBus Integration");

        // 关闭Source客户端
        if (sourceControlSocket_ >= 0) {
            Shutdown(sourceControlSocket_);
        }
        if (sourceSnapshotSocket_ >= 0) {
            Shutdown(sourceSnapshotSocket_);
        }
        if (sourceContinuousSocket_ >= 0) {
            Shutdown(sourceContinuousSocket_);
        }

        // 关闭Sink服务器
        if (sinkControlSocket_ >= 0) {
            Shutdown(sinkControlSocket_);
        }
        if (sinkSnapshotSocket_ >= 0) {
            Shutdown(sinkSnapshotSocket_);
        }
        if (sinkContinuousSocket_ >= 0) {
            Shutdown(sinkContinuousSocket_);
        }

        // 等待线程结束
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // 反初始化Mock
        SoftbusMock::GetInstance().Deinitialize();

        initialized_ = false;
        DHLOGI("Cleanup completed");
    }

private:
    // Sink端回调函数
    static void SinkOnBind(int32_t socket, PeerSocketInfo info) {
        DHLOGI("Sink: Client connected on socket %d (peer=%s)", socket, info.name);
    }

    static void SinkOnShutdown(int32_t socket, ShutdownReason reason) {
        DHLOGI("Sink: Connection closed on socket %d (reason=%d)", socket, reason);
    }

    static void SinkOnBytes(int32_t socket, const void* data, uint32_t dataLen) {
        DHLOGI("Sink: Received %u bytes on socket %d", dataLen, socket);

        // 打印接收到的控制命令
        if (data && dataLen > 0) {
            std::string cmd(static_cast<const char*>(data), std::min(dataLen, 100u));
            DHLOGI("Sink: Command: %s", cmd.c_str());
        }
    }

    static void SinkOnStream(int32_t socket, const StreamData* data,
                             const StreamData* ext, const StreamFrameInfo* param) {
        if (param) {
            DHLOGI("Sink: Received stream frame on socket %d (type=%d, seq=%d)",
                   socket, param->frameType, param->seqNum);
        }
    }

    // Source端回调函数
    static void SourceOnBind(int32_t socket, PeerSocketInfo info) {
        DHLOGI("Source: Connected to sink on socket %d", socket);
    }

    static void SourceOnShutdown(int32_t socket, ShutdownReason reason) {
        DHLOGI("Source: Connection closed on socket %d (reason=%d)", socket, reason);
    }

    static void SourceOnBytes(int32_t socket, const void* data, uint32_t dataLen) {
        DHLOGI("Source: Received %u bytes on socket %d", dataLen, socket);
    }

    static void SourceOnStream(int32_t socket, const StreamData* data,
                               const StreamData* ext, const StreamFrameInfo* param) {
        DHLOGI("Source: Stream callback on socket %d", socket);
    }

    // 创建单个Sink服务器
    bool CreateSinkServer(const std::string& name, SoftbusChannelType type, int32_t& socket) {
        SocketInfo info = {};
        std::string nameCopy = name;
        info.name = const_cast<char*>(nameCopy.c_str());
        info.peerName = const_cast<char*>(strdup(GetSourceName(name).c_str()));
        info.peerNetworkId = const_cast<char*>(strdup("DCAMERA_PEER_001"));
        info.pkgName = const_cast<char*>(strdup("ohos.dhardware.dcamera"));

        switch (type) {
            case CHANNEL_TYPE_CONTROL:
                info.dataType = DATA_TYPE_BYTES;
                break;
            case CHANNEL_TYPE_SNAPSHOT:
                info.dataType = DATA_TYPE_BYTES;
                break;
            case CHANNEL_TYPE_CONTINUOUS:
                info.dataType = DATA_TYPE_VIDEO_STREAM;
                break;
        }

        socket = Socket(info);
        if (socket < 0) {
            DHLOGE("Failed to create socket for %s", name.c_str());
            return false;
        }

        ISocketListener listener = {};
        listener.OnBind = SinkOnBind;
        listener.OnShutdown = SinkOnShutdown;
        listener.OnBytes = SinkOnBytes;
        listener.OnStream = SinkOnStream;

        QosTV qos[] = {
            {QOS_TYPE_MIN_BW, type == CHANNEL_TYPE_CONTINUOUS ? 10000000 : 1000000},
            {QOS_TYPE_MAX_LATENCY, 100}
        };

        int32_t result = Listen(socket, qos, 2, &listener);
        if (result != 0) {
            DHLOGE("Failed to listen on socket %d", socket);
            return false;
        }

        DHLOGI("Sink server created for %s (socket=%d)", name.c_str(), socket);
        return true;
    }

    // 创建单个Source客户端
    bool CreateSourceClient(const std::string& sourceName, const std::string& sinkName,
                            SoftbusChannelType type, int32_t& socket) {
        SocketInfo info = {};
        std::string nameCopy = sourceName;
        info.name = const_cast<char*>(nameCopy.c_str());
        info.peerName = const_cast<char*>(strdup(sinkName.c_str()));
        info.peerNetworkId = const_cast<char*>(strdup("DCAMERA_PEER_001"));
        info.pkgName = const_cast<char*>(strdup("ohos.dhardware.dcamera"));

        switch (type) {
            case CHANNEL_TYPE_CONTROL:
                info.dataType = DATA_TYPE_BYTES;
                break;
            case CHANNEL_TYPE_SNAPSHOT:
                info.dataType = DATA_TYPE_BYTES;
                break;
            case CHANNEL_TYPE_CONTINUOUS:
                info.dataType = DATA_TYPE_VIDEO_STREAM;
                break;
        }

        socket = Socket(info);
        if (socket < 0) {
            DHLOGE("Failed to create socket for %s", sourceName.c_str());
            return false;
        }

        ISocketListener listener = {};
        listener.OnBind = SourceOnBind;
        listener.OnShutdown = SourceOnShutdown;
        listener.OnBytes = SourceOnBytes;
        listener.OnStream = SourceOnStream;

        QosTV qos[] = {
            {QOS_TYPE_MIN_BW, type == CHANNEL_TYPE_CONTINUOUS ? 10000000 : 1000000},
            {QOS_TYPE_MAX_LATENCY, 100}
        };

        int32_t result = Bind(socket, qos, 2, &listener);
        if (result < 0) {
            DHLOGE("Failed to bind socket %d", socket);
            return false;
        }

        DHLOGI("Source client created for %s (socket=%d)", sourceName.c_str(), socket);
        return true;
    }

    // 根据Sink名称获取对应的Source名称
    std::string GetSourceName(const std::string& sinkName) {
        if (sinkName.find("SinkControl") != std::string::npos) {
            return "DCameraSourceControl";
        } else if (sinkName.find("SinkSnapshot") != std::string::npos) {
            return "DCameraSourceSnapshot";
        } else if (sinkName.find("SinkContinuous") != std::string::npos) {
            return "DCameraSourceContinuous";
        }
        return "Unknown";
    }

private:
    bool initialized_;

    // Socket IDs
    int32_t sourceControlSocket_;
    int32_t sourceSnapshotSocket_;
    int32_t sourceContinuousSocket_;
    int32_t sinkControlSocket_;
    int32_t sinkSnapshotSocket_;
    int32_t sinkContinuousSocket_;
};

// 主测试函数
int main(int argc, char* argv[]) {
    DHLOGI("========== DCamera SoftBus Integration Test ==========");

    DCameraSoftbusIntegration integration;

    // 初始化
    if (!integration.Initialize()) {
        DHLOGE("Initialization failed");
        return 1;
    }

    // 创建Sink端服务器
    if (!integration.CreateSinkServers()) {
        DHLOGE("Failed to create sink servers");
        return 1;
    }

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 创建Source端客户端
    if (!integration.CreateSourceClients()) {
        DHLOGE("Failed to create source clients");
        return 1;
    }

    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 测试控制通道
    DHLOGI("========== Testing Control Channel ==========");
    if (!integration.TestControlChannel()) {
        DHLOGE("Control channel test failed");
    }

    // 测试抓拍通道
    DHLOGI("========== Testing Snapshot Channel ==========");
    if (!integration.TestSnapshotChannel()) {
        DHLOGE("Snapshot channel test failed");
    }

    // 测试连续通道
    DHLOGI("========== Testing Continuous Channel ==========");
    if (!integration.TestContinuousChannel()) {
        DHLOGE("Continuous channel test failed");
    }

    // 打印统计信息
    integration.PrintStatistics();

    // 清理资源
    integration.Cleanup();

    DHLOGI("========== Integration Test Complete ==========");
    return 0;
}
