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

#include "avcodec_mock.h"
#include <iostream>
#include <cassert>

using namespace OHOS::MediaAVCodec;

// 测试回调类
class TestEncoderCallback : public MediaCodecCallback {
public:
    int32_t outputCount_ = 0;
    int32_t errorCount_ = 0;
    int32_t formatChangeCount_ = 0;
    AVCodecBufferInfo lastBufferInfo_;
    std::vector<uint8_t> lastEncodedData_;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override {
        std::cout << "[TestEncoderCallback] OnError: type=" << static_cast<int>(errorType)
                  << ", code=" << errorCode << std::endl;
        errorCount_++;
    }

    void OnOutputFormatChanged(const Format& format) override {
        std::cout << "[TestEncoderCallback] OnOutputFormatChanged" << std::endl;
        formatChangeCount_++;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        // Surface模式下不需要处理输入Buffer
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        std::cout << "[TestEncoderCallback] OnOutputBufferAvailable: index=" << index
                  << ", size=" << buffer->GetSize() << std::endl;

        outputCount_++;
        lastBufferInfo_ = buffer->GetBufferAttr();

        // 保存编码数据
        lastEncodedData_.assign(buffer->GetAddr(), buffer->GetAddr() + buffer->GetSize());
    }
};

class TestDecoderCallback : public MediaCodecCallback {
public:
    int32_t outputCount_ = 0;
    int32_t errorCount_ = 0;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override {
        std::cout << "[TestDecoderCallback] OnError: type=" << static_cast<int>(errorType)
                  << ", code=" << errorCode << std::endl;
        errorCount_++;
    }

    void OnOutputFormatChanged(const Format& format) override {
        std::cout << "[TestDecoderCallback] OnOutputFormatChanged" << std::endl;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        std::cout << "[TestDecoderCallback] OnInputBufferAvailable: index=" << index << std::endl;
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {
        std::cout << "[TestDecoderCallback] OnOutputBufferAvailable: index=" << index
                  << ", size=" << buffer->GetSize() << std::endl;
        outputCount_++;
    }
};

void TestVideoEncoderFactory() {
    std::cout << "\n========== Test VideoEncoderFactory ==========" << std::endl;

    // 测试通过MIME类型创建H.265编码器
    auto h265Encoder = VideoEncoderFactory::CreateByMime("video/hevc");
    assert(h265Encoder != nullptr);
    std::cout << "[PASS] Created H.265 encoder by MIME type" << std::endl;

    // 测试通过MIME类型创建H.264编码器
    auto h264Encoder = VideoEncoderFactory::CreateByMime("video/avc");
    assert(h264Encoder != nullptr);
    std::cout << "[PASS] Created H.264 encoder by MIME type" << std::endl;

    // 测试通过名称创建编码器
    auto namedEncoder = VideoEncoderFactory::CreateByName("OMX.hisi.video.encoder.hevc");
    assert(namedEncoder != nullptr);
    std::cout << "[PASS] Created encoder by name" << std::endl;

    // 测试不支持的MIME类型
    auto invalidEncoder = VideoEncoderFactory::CreateByMime("invalid/mime");
    assert(invalidEncoder == nullptr);
    std::cout << "[PASS] Rejected invalid MIME type" << std::endl;
}

void TestVideoEncoderLifecycle() {
    std::cout << "\n========== Test VideoEncoder Lifecycle ==========" << std::endl;

    auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
    assert(encoder != nullptr);

    // 配置编码器
    Format format;
    format.PutIntValue("width", 1920);
    format.PutIntValue("height", 1080);
    format.PutDoubleValue("frame_rate", 30.0);
    format.PutIntValue("bitrate", 5000000);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    int32_t result = encoder->Configure(format);
    assert(result == 0);
    assert(encoder->IsConfigured());
    assert(encoder->GetConfigWidth() == 1920);
    assert(encoder->GetConfigHeight() == 1080);
    std::cout << "[PASS] Configured encoder" << std::endl;

    // 准备编码器
    result = encoder->Prepare();
    assert(result == 0);
    assert(encoder->IsPrepared());
    std::cout << "[PASS] Prepared encoder" << std::endl;

    // 启动编码器
    result = encoder->Start();
    assert(result == 0);
    assert(encoder->IsStarted());
    std::cout << "[PASS] Started encoder" << std::endl;

    // 停止编码器
    result = encoder->Stop();
    assert(result == 0);
    assert(!encoder->IsStarted());
    std::cout << "[PASS] Stopped encoder" << std::endl;

    // 释放编码器
    result = encoder->Release();
    assert(result == 0);
    std::cout << "[PASS] Released encoder" << std::endl;
}

void TestVideoEncoderSurface() {
    std::cout << "\n========== Test VideoEncoder Surface ==========" << std::endl;

    auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
    assert(encoder != nullptr);

    // 配置编码器
    Format format;
    format.PutIntValue("width", 1280);
    format.PutIntValue("height", 720);
    format.PutDoubleValue("frame_rate", 25.0);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    encoder->Configure(format);
    encoder->Prepare();

    // 创建输入Surface（关键接口）
    auto surface = encoder->CreateInputSurface();
    assert(surface != nullptr);
    std::cout << "[PASS] Created input surface" << std::endl;

    // 设置用户数据
    surface->SetUserData("test_key", reinterpret_cast<void*>(0x1234));
    void* data = surface->GetUserData("test_key");
    assert(data == reinterpret_cast<void*>(0x1234));
    std::cout << "[PASS] Surface user data works" << std::endl;
}

void TestVideoEncoderCallback() {
    std::cout << "\n========== Test VideoEncoder Callback ==========" << std::endl;

    auto encoder = VideoEncoderFactory::CreateByMime("video/hevc");
    auto callback = std::make_shared<TestEncoderCallback>();

    // 注册回调
    int32_t result = encoder->SetCallback(callback);
    assert(result == 0);
    std::cout << "[PASS] Registered callback" << std::endl;

    // 配置并启动编码器
    Format format;
    format.PutIntValue("width", 1920);
    format.PutIntValue("height", 1080);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    encoder->Configure(format);
    encoder->Prepare();
    encoder->Start();

    // 模拟输出编码数据
    std::vector<uint8_t> mockH265Data = {
        0x00, 0x00, 0x00, 0x01, 0x20, 0x01, 0x00, 0x01  // H.265 NAL头
    };

    encoder->SimulateEncodedOutput(0, mockH265Data, 0);

    assert(callback->outputCount_ == 1);
    assert(callback->lastEncodedData_ == mockH265Data);
    std::cout << "[PASS] Received encoded output" << std::endl;

    // 模拟错误
    encoder->SimulateError(AVCodecErrorType::ERROR_CODEC, -1);
    assert(callback->errorCount_ == 1);
    std::cout << "[PASS] Received error callback" << std::endl;
}

void TestVideoDecoderFactory() {
    std::cout << "\n========== Test VideoDecoderFactory ==========" << std::endl;

    // 测试通过MIME类型创建H.265解码器
    auto h265Decoder = VideoDecoderFactory::CreateByMime("video/hevc");
    assert(h265Decoder != nullptr);
    std::cout << "[PASS] Created H.265 decoder by MIME type" << std::endl;

    // 测试通过MIME类型创建H.264解码器
    auto h264Decoder = VideoDecoderFactory::CreateByMime("video/avc");
    assert(h264Decoder != nullptr);
    std::cout << "[PASS] Created H.264 decoder by MIME type" << std::endl;
}

void TestVideoDecoderLifecycle() {
    std::cout << "\n========== Test VideoDecoder Lifecycle ==========" << std::endl;

    auto decoder = VideoDecoderFactory::CreateByMime("video/hevc");
    assert(decoder != nullptr);

    // 配置解码器
    Format format;
    format.PutIntValue("width", 1920);
    format.PutIntValue("height", 1080);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    int32_t result = decoder->Configure(format);
    assert(result == 0);
    assert(decoder->IsConfigured());
    std::cout << "[PASS] Configured decoder" << std::endl;

    // 准备解码器
    result = decoder->Prepare();
    assert(result == 0);
    assert(decoder->IsPrepared());
    std::cout << "[PASS] Prepared decoder" << std::endl;

    // 启动解码器
    result = decoder->Start();
    assert(result == 0);
    assert(decoder->IsStarted());
    std::cout << "[PASS] Started decoder" << std::endl;

    // 停止解码器
    result = decoder->Stop();
    assert(result == 0);
    assert(!decoder->IsStarted());
    std::cout << "[PASS] Stopped decoder" << std::endl;
}

void TestVideoDecoderCallback() {
    std::cout << "\n========== Test VideoDecoder Callback ==========" << std::endl;

    auto decoder = VideoDecoderFactory::CreateByMime("video/hevc");
    auto callback = std::make_shared<TestDecoderCallback>();

    // 注册回调
    int32_t result = decoder->SetCallback(callback);
    assert(result == 0);
    std::cout << "[PASS] Registered callback" << std::endl;

    // 配置并启动解码器
    Format format;
    format.PutIntValue("width", 1920);
    format.PutIntValue("height", 1080);
    format.PutIntValue("pixel_format", static_cast<int32_t>(PixelFormat::NV12));

    decoder->Configure(format);
    decoder->Prepare();
    decoder->Start();

    // 模拟输出解码数据
    decoder->SimulateDecodedOutput(0, 1920, 1080, 0);

    assert(callback->outputCount_ == 1);
    std::cout << "[PASS] Received decoded output" << std::endl;

    // 模拟错误
    decoder->SimulateError(AVCodecErrorType::ERROR_CODEC, -1);
    assert(callback->errorCount_ == 1);
    std::cout << "[PASS] Received error callback" << std::endl;
}

void TestFormat() {
    std::cout << "\n========== Test Format ==========" << std::endl;

    Format format;

    // 测试整数
    format.PutIntValue("width", 1920);
    assert(format.GetIntValue("width", 0) == 1920);
    assert(format.Contains("width"));
    std::cout << "[PASS] Format int value" << std::endl;

    // 测试浮点数
    format.PutDoubleValue("frame_rate", 30.0);
    assert(format.GetDoubleValue("frame_rate", 0.0) == 30.0);
    std::cout << "[PASS] Format double value" << std::endl;

    // 测试字符串
    format.PutStringValue("codec", "hevc");
    assert(format.GetStringValue("codec", "") == "hevc");
    std::cout << "[PASS] Format string value" << std::endl;

    // 测试默认值
    assert(format.GetIntValue("nonexistent", 999) == 999);
    std::cout << "[PASS] Format default value" << std::endl;
}

void TestAVBuffer() {
    std::cout << "\n========== Test AVBuffer ==========" << std::endl;

    // 创建Buffer
    AVBuffer buffer(1024);
    assert(buffer.GetSize() == 1024);
    std::cout << "[PASS] Created AVBuffer" << std::endl;

    // 设置数据
    std::vector<uint8_t> testData = {1, 2, 3, 4, 5};
    buffer.SetData(testData.data(), testData.size());
    assert(buffer.GetSize() == testData.size());
    std::cout << "[PASS] Set buffer data" << std::endl;

    // 获取数据
    assert(std::memcmp(buffer.GetAddr(), testData.data(), testData.size()) == 0);
    std::cout << "[PASS] Get buffer data" << std::endl;

    // 设置和获取属性
    AVCodecBufferInfo info{};
    info.presentationTimeUs = 12345;
    info.size = 100;
    info.offset = 0;
    buffer.SetBufferAttr(info);

    AVCodecBufferInfo retrievedInfo = buffer.GetBufferAttr();
    assert(retrievedInfo.presentationTimeUs == 12345);
    assert(retrievedInfo.size == 100);
    std::cout << "[PASS] Buffer attributes" << std::endl;
}

int main() {
    std::cout << "============================================" << std::endl;
    std::cout << "   AVCodec Mock Framework Test Suite" << std::endl;
    std::cout << "============================================" << std::endl;

    try {
        // 测试基础类
        TestFormat();
        TestAVBuffer();

        // 测试编码器工厂
        TestVideoEncoderFactory();

        // 测试编码器生命周期
        TestVideoEncoderLifecycle();

        // 测试编码器Surface
        TestVideoEncoderSurface();

        // 测试编码器回调
        TestVideoEncoderCallback();

        // 测试解码器工厂
        TestVideoDecoderFactory();

        // 测试解码器生命周期
        TestVideoDecoderLifecycle();

        // 测试解码器回调
        TestVideoDecoderCallback();

        std::cout << "\n============================================" << std::endl;
        std::cout << "   ALL TESTS PASSED!" << std::endl;
        std::cout << "============================================" << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
