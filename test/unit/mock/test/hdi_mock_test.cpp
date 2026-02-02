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

#include "hdi_mock.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace OHOS::DistributedHardware;

/**
 * @brief HDI Mock功能演示
 *
 * 演示内容：
 * 1. 模拟虚拟相机HDF设备
 * 2. 实现AcquireBuffer/ShutterBuffer零拷贝数据传输
 * 3. 模拟三通道流配置（Control/Snapshot/Continuous）
 */

void PrintTestHeader(const std::string& testName) {
    std::cout << "\n========================================\n";
    std::cout << "测试: " << testName << "\n";
    std::cout << "========================================\n";
}

void TestEnableDCameraDevice() {
    PrintTestHeader("使能虚拟相机设备");

    auto mockProvider = MockHdiProvider::GetInstance();
    auto mockCallback = std::make_shared<MockProviderCallback>();

    DHBase dhBase;
    dhBase.deviceId_ = "test_network_id_001";
    dhBase.dhId_ = "test_dh_id_001";

    std::string abilityInfo = "{}";  // JSON格式的相机能力信息

    int32_t result = mockProvider->EnableDCameraDevice(dhBase, abilityInfo, mockCallback);

    std::cout << "使能设备结果: " << result
              << " (0=SUCCESS)" << std::endl;
    std::cout << "设备是否已启用: " << (mockProvider->IsDeviceEnabled(dhBase.dhId_) ? "是" : "否")
              << std::endl;
}

void TestConfigureTripleStreams() {
    PrintTestHeader("配置三通道流");

    auto mockProvider = MockHdiProvider::GetInstance();

    DHBase dhBase;
    dhBase.deviceId_ = "test_network_id_001";
    dhBase.dhId_ = "test_dh_id_001";

    // 创建默认三通道流配置
    auto streamInfos = TripleStreamConfig::CreateDefaultTripleStreams();

    std::cout << "三通道流配置:" << std::endl;
    std::cout << "  - Control通道 (ID=" << TripleStreamConfig::CONTROL_STREAM_ID
              << "): 用于控制命令" << std::endl;
    std::cout << "  - Snapshot通道 (ID=" << TripleStreamConfig::SNAPSHOT_STREAM_ID
              << "): " << TripleStreamConfig::SNAPSHOT_MAX_WIDTH
              << "x" << TripleStreamConfig::SNAPSHOT_MAX_HEIGHT
              << ", JPEG编码" << std::endl;
    std::cout << "  - Continuous通道 (ID=" << TripleStreamConfig::CONTINUOUS_STREAM_ID
              << "): " << TripleStreamConfig::CONTINUOUS_MAX_WIDTH
              << "x" << TripleStreamConfig::CONTINUOUS_MAX_HEIGHT
              << ", H.265编码" << std::endl;

    int32_t result = mockProvider->TriggerConfigureStreams(dhBase, streamInfos);

    std::cout << "配置流结果: " << result
              << " (0=SUCCESS)" << std::endl;
    std::cout << "活跃流数量: " << mockProvider->GetActiveStreamCount() << std::endl;

    auto streamIds = mockProvider->GetConfiguredStreamIds();
    std::cout << "已配置的流ID: ";
    for (int id : streamIds) {
        std::cout << id << " ";
    }
    std::cout << std::endl;
}

void TestAcquireAndShutterBuffer() {
    PrintTestHeader("零拷贝缓冲区获取与提交");

    auto mockProvider = MockHdiProvider::GetInstance();

    DHBase dhBase;
    dhBase.deviceId_ = "test_network_id_001";
    dhBase.dhId_ = "test_dh_id_001";

    // 获取Continuous通道的缓冲区
    int streamId = TripleStreamConfig::CONTINUOUS_STREAM_ID;

    DCameraBuffer buffer;
    int32_t acquireResult = mockProvider->AcquireBuffer(dhBase, streamId, buffer);

    std::cout << "获取缓冲区结果: " << acquireResult
              << " (0=SUCCESS)" << std::endl;
    std::cout << "缓冲区索引: " << buffer.index_ << std::endl;
    std::cout << "缓冲区大小: " << buffer.size_ << " 字节" << std::endl;
    std::cout << "虚拟地址: " << (buffer.virAddr_ ? "有效" : "无效") << std::endl;

    if (buffer.virAddr_) {
        // 零拷贝测试：直接操作内存
        std::cout << "执行零拷贝数据写入..." << std::endl;

        // 模拟YUV420数据填充
        uint8_t* data = static_cast<uint8_t*>(buffer.virAddr_);
        size_t ySize = TripleStreamConfig::CONTINUOUS_MAX_WIDTH *
                       TripleStreamConfig::CONTINUOUS_MAX_HEIGHT;
        size_t uvSize = ySize / 2;

        // 填充Y平面（灰色）
        std::memset(data, 128, ySize);
        // 填充UV平面
        std::memset(data + ySize, 128, uvSize);

        std::cout << "数据填充完成 (YUV420格式)" << std::endl;
    }

    std::cout << "缓冲区获取次数: " << mockProvider->GetBufferAcquireCount() << std::endl;

    // 提交缓冲区
    int32_t shutterResult = mockProvider->ShutterBuffer(dhBase, streamId, buffer);

    std::cout << "提交缓冲区结果: " << shutterResult
              << " (0=SUCCESS)" << std::endl;
    std::cout << "缓冲区提交次数: " << mockProvider->GetBufferShutterCount() << std::endl;
}

void TestCaptureWorkflow() {
    PrintTestHeader("完整捕获流程");

    auto mockProvider = MockHdiProvider::GetInstance();
    auto mockCallback = std::make_shared<MockProviderCallback>();

    DHBase dhBase;
    dhBase.deviceId_ = "test_network_id_001";
    dhBase.dhId_ = "test_dh_id_001";

    // 1. 打开会话
    std::cout << "1. 打开会话..." << std::endl;
    mockProvider->TriggerOpenSession(dhBase);
    std::cout << "   会话状态: " << (mockCallback->IsSessionOpen() ? "已打开" : "未打开") << std::endl;

    // 2. 配置流
    std::cout << "\n2. 配置三通道流..." << std::endl;
    auto streamInfos = TripleStreamConfig::CreateDefaultTripleStreams();
    mockProvider->TriggerConfigureStreams(dhBase, streamInfos);
    std::cout << "   流配置状态: " << (mockCallback->IsStreamsConfigured() ? "已配置" : "未配置") << std::endl;

    // 3. 开始捕获
    std::cout << "\n3. 开始捕获..." << std::endl;
    std::vector<DCCaptureInfo> captureInfos;

    // Snapshot捕获信息
    DCCaptureInfo snapshotCapture;
    snapshotCapture.streamIds_ = {TripleStreamConfig::SNAPSHOT_STREAM_ID};
    snapshotCapture.width_ = TripleStreamConfig::SNAPSHOT_MAX_WIDTH;
    snapshotCapture.height_ = TripleStreamConfig::SNAPSHOT_MAX_HEIGHT;
    snapshotCapture.encodeType_ = DCEncodeType::ENCODE_TYPE_JPEG;
    snapshotCapture.type_ = DCStreamType::SNAPSHOT_FRAME;
    snapshotCapture.isCapture_ = true;
    captureInfos.push_back(snapshotCapture);

    // Continuous捕获信息
    DCCaptureInfo continuousCapture;
    continuousCapture.streamIds_ = {TripleStreamConfig::CONTINUOUS_STREAM_ID};
    continuousCapture.width_ = TripleStreamConfig::CONTINUOUS_MAX_WIDTH;
    continuousCapture.height_ = TripleStreamConfig::CONTINUOUS_MAX_HEIGHT;
    continuousCapture.encodeType_ = DCEncodeType::ENCODE_TYPE_H265;
    continuousCapture.type_ = DCStreamType::CONTINUOUS_FRAME;
    continuousCapture.isCapture_ = true;
    captureInfos.push_back(continuousCapture);

    mockProvider->TriggerStartCapture(dhBase, captureInfos);
    std::cout << "   捕获状态: " << (mockCallback->IsCaptureStarted() ? "已启动" : "未启动") << std::endl;

    // 4. 模拟视频帧处理
    std::cout << "\n4. 模拟视频帧处理..." << std::endl;
    for (int i = 0; i < 3; ++i) {
        DCameraBuffer buffer;
        if (mockProvider->AcquireBuffer(dhBase, TripleStreamConfig::CONTINUOUS_STREAM_ID, buffer)
            == static_cast<int32_t>(DCamRetCode::SUCCESS)) {
            std::cout << "   帧 " << (i + 1) << ": 获取缓冲区成功" << std::endl;
            mockProvider->ShutterBuffer(dhBase, TripleStreamConfig::CONTINUOUS_STREAM_ID, buffer);
        }
    }

    // 5. 停止捕获
    std::cout << "\n5. 停止捕获..." << std::endl;
    std::vector<int> streamIds = {
        TripleStreamConfig::SNAPSHOT_STREAM_ID,
        TripleStreamConfig::CONTINUOUS_STREAM_ID
    };
    mockProvider->TriggerStopCapture(dhBase, streamIds);

    // 6. 关闭会话
    std::cout << "\n6. 关闭会话..." << std::endl;
    mockProvider->TriggerCloseSession(dhBase);

    std::cout << "\n流程完成统计:" << std::endl;
    std::cout << "  OpenSession调用: " << mockCallback->GetOpenSessionCount() << std::endl;
    std::cout << "  ConfigureStreams调用: " << mockCallback->GetConfigureStreamsCount() << std::endl;
    std::cout << "  StartCapture调用: " << mockCallback->GetStartCaptureCount() << std::endl;
    std::cout << "  StopCapture调用: " << mockCallback->GetStopCaptureCount() << std::endl;
}

void TestZeroCopyBufferManager() {
    PrintTestHeader("零拷贝缓冲区管理器");

    auto& bufferMgr = ZeroCopyBufferManager::GetInstance();

    // 创建缓冲区
    size_t bufferSize = 1920 * 1080 * 3 / 2;  // NV12格式
    DCameraBuffer buffer = bufferMgr.CreateBuffer(bufferSize);

    std::cout << "创建缓冲区:" << std::endl;
    std::cout << "  索引: " << buffer.index_ << std::endl;
    std::cout << "  大小: " << buffer.size_ << " 字节" << std::endl;

    // 设置数据
    std::vector<uint8_t> testData(bufferSize, 0x42);
    int32_t result = bufferMgr.SetBufferData(buffer, testData.data(), testData.size());
    std::cout << "设置数据结果: " << result << " (0=SUCCESS)" << std::endl;

    // 获取数据
    void* data = bufferMgr.GetBufferData(buffer);
    std::cout << "获取数据指针: " << (data ? "有效" : "无效") << std::endl;

    if (data) {
        uint8_t* byteData = static_cast<uint8_t*>(data);
        std::cout << "数据验证: "
                  << (byteData[0] == 0x42 ? "通过" : "失败") << std::endl;
    }

    // 统计信息
    std::cout << "\n缓冲区管理器统计:" << std::endl;
    std::cout << "  活跃缓冲区数: " << bufferMgr.GetActiveBufferCount() << std::endl;
    std::cout << "  总分配内存: " << bufferMgr.GetTotalAllocatedSize() << " 字节" << std::endl;

    // 释放缓冲区
    bufferMgr.ReleaseBuffer(buffer);
    std::cout << "释放缓冲区后活跃数: " << bufferMgr.GetActiveBufferCount() << std::endl;
}

void TestCustomStreamConfiguration() {
    PrintTestHeader("自定义流配置");

    // 创建自定义分辨率的三通道流
    auto streamInfos = TripleStreamConfig::CreateCustomTripleStreams(
        1920, 1080,   // Snapshot: 1920x1080
        1280, 720     // Continuous: 1280x720
    );

    std::cout << "自定义三通道流配置:" << std::endl;
    for (const auto& stream : streamInfos) {
        std::cout << "  流ID=" << stream.streamId_
                  << " 类型=" << (stream.type_ == DCStreamType::SNAPSHOT_FRAME ? "SNAPSHOT" :
                                 stream.type_ == DCStreamType::CONTINUOUS_FRAME ? "CONTINUOUS" : "CONTROL")
                  << " 分辨率=" << stream.width_ << "x" << stream.height_
                  << " 编码=" << (stream.encodeType_ == DCEncodeType::ENCODE_TYPE_JPEG ? "JPEG" :
                                 stream.encodeType_ == DCEncodeType::ENCODE_TYPE_H265 ? "H.265" : "NONE")
                  << std::endl;
    }
}

void TestErrorScenarios() {
    PrintTestHeader("错误场景测试");

    auto mockProvider = MockHdiProvider::GetInstance();
    auto mockCallback = std::make_shared<MockProviderCallback>();

    DHBase dhBase;
    dhBase.deviceId_ = "test_network_id_001";
    dhBase.dhId_ = "test_dh_id_001";

    // 1. 测试未启用设备时获取缓冲区
    std::cout << "1. 测试未启用设备时获取缓冲区..." << std::endl;
    DCameraBuffer buffer;
    int32_t result = mockProvider->AcquireBuffer(dhBase, 0, buffer);
    std::cout << "   结果: " << result
              << " (预期: " << static_cast<int32_t>(DCamRetCode::DEVICE_NOT_INIT) << ")" << std::endl;

    // 2. 使能设备后测试无效流ID
    std::cout << "\n2. 测试无效流ID..." << std::endl;
    mockProvider->EnableDCameraDevice(dhBase, "{}", mockCallback);
    result = mockProvider->AcquireBuffer(dhBase, 999, buffer);
    std::cout << "   结果: " << result
              << " (预期: " << static_cast<int32_t>(DCamRetCode::INVALID_ARGUMENT) << ")" << std::endl;

    // 3. 测试回调失败场景
    std::cout << "\n3. 测试回调失败场景..." << std::endl;
    mockCallback->SetCallbackResult(static_cast<int32_t>(DCamRetCode::FAILED));
    result = mockProvider->TriggerOpenSession(dhBase);
    std::cout << "   结果: " << result
              << " (预期: " << static_cast<int32_t>(DCamRetCode::FAILED) << ")" << std::endl;

    // 重置
    mockProvider->Reset();
    mockCallback->Reset();
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  HDF/HDI接口Mock功能测试\n";
    std::cout << "========================================\n";

    try {
        // 基本功能测试
        TestEnableDCameraDevice();

        // 三通道流配置测试
        TestConfigureTripleStreams();

        // 零拷贝缓冲区测试
        TestAcquireAndShutterBuffer();

        // 完整捕获流程测试
        TestCaptureWorkflow();

        // 零拷贝缓冲区管理器测试
        TestZeroCopyBufferManager();

        // 自定义流配置测试
        TestCustomStreamConfiguration();

        // 错误场景测试
        TestErrorScenarios();

        std::cout << "\n========================================\n";
        std::cout << "  所有测试完成!\n";
        std::cout << "========================================\n";

    } catch (const std::exception& e) {
        std::cerr << "测试异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
