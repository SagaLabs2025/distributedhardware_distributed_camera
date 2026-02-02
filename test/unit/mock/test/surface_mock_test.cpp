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

#include "surface_mock.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace OHOS;

// 测试用的Buffer消费者监听器
class TestConsumerListener : public IBufferConsumerListener {
public:
    void OnBufferAvailable() override {
        std::cout << "[Consumer] Buffer available!" << std::endl;
        bufferAvailableCount_++;
    }

    int GetBufferAvailableCount() const { return bufferAvailableCount_; }

private:
    int bufferAvailableCount_ = 0;
};

// 测试Producer-Consumer模式
void TestProducerConsumerPattern() {
    std::cout << "=== Test Producer-Consumer Pattern ===" << std::endl;

    // 创建Consumer Surface
    auto consumerSurface = MockSurfaceFactory::CreateIConsumerSurface("test_surface");
    consumerSurface->SetDefaultWidthAndHeight(1920, 1080);
    consumerSurface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

    // 注册监听器
    auto listener = std::make_shared<TestConsumerListener>();
    consumerSurface->RegisterConsumerListener(listener);

    // 获取Producer
    auto producer = consumerSurface->GetProducer();

    // Producer请求缓冲区
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    BufferRequestConfig requestConfig = {
        .width = 1920,
        .height = 1080,
        .strideAlignment = 8,
        .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
        .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
        .timeout = 1000
    };

    GSError ret = producer->RequestBuffer(buffer, fence, requestConfig);
    if (ret == GSError::GSERROR_OK) {
        std::cout << "[Producer] Buffer requested successfully" << std::endl;
        std::cout << "  Buffer size: " << buffer->GetSize() << " bytes" << std::endl;
        std::cout << "  Width: " << buffer->GetWidth() << std::endl;
        std::cout << "  Height: " << buffer->GetHeight() << std::endl;

        // 填充数据（NV12格式）
        void* data = buffer->GetVirAddr();
        if (data != nullptr) {
            auto yPlane = static_cast<uint8_t*>(data);
            // 填充Y平面
            for (int i = 0; i < 1920 * 1080; i++) {
                yPlane[i] = 128; // 灰色
            }

            // 填充UV平面
            auto uvPlane = yPlane + 1920 * 1080;
            for (int i = 0; i < 1920 * 1080 / 2; i++) {
                uvPlane[i] = 128;
            }
            std::cout << "[Producer] Data filled" << std::endl;
        }

        // 设置额外数据
        buffer->GetExtraData()->ExtraSet("timeStamp", 12345678);
        std::cout << "[Producer] Extra data set" << std::endl;

        // Producer刷新缓冲区
        BufferFlushConfig flushConfig = {
            .damage = {0, 0, 1920, 1080},
            .timestamp = 12345678
        };
        ret = producer->FlushBuffer(buffer, fence, flushConfig);
        if (ret == GSError::GSERROR_OK) {
            std::cout << "[Producer] Buffer flushed" << std::endl;
        }
    }

    // Consumer获取缓冲区
    sptr<SurfaceBuffer> consumerBuffer;
    sptr<SyncFence> consumerFence;
    int64_t timestamp = 0;
    Rect damage;

    ret = consumerSurface->AcquireBuffer(consumerBuffer, consumerFence, timestamp, damage);
    if (ret == GSError::GSERROR_OK) {
        std::cout << "[Consumer] Buffer acquired" << std::endl;
        std::cout << "  Timestamp: " << timestamp << std::endl;
        std::cout << "  Damage: x=" << damage.x << " y=" << damage.y
                  << " w=" << damage.w << " h=" << damage.h << std::endl;

        // 读取数据
        void* data = consumerBuffer->GetVirAddr();
        if (data != nullptr) {
            auto yPlane = static_cast<uint8_t*>(data);
            std::cout << "  First pixel value: " << static_cast<int>(yPlane[0]) << std::endl;
        }

        // 获取额外数据
        int64_t ts = 0;
        if (consumerBuffer->GetExtraData()->ExtraGet("timeStamp", ts)) {
            std::cout << "  Extra timestamp: " << ts << std::endl;
        }

        // Consumer释放缓冲区
        ret = consumerSurface->ReleaseBuffer(consumerBuffer, consumerFence);
        if (ret == GSError::GSERROR_OK) {
            std::cout << "[Consumer] Buffer released" << std::endl;
        }
    }

    std::cout << "Buffer available notifications: " << listener->GetBufferAvailableCount() << std::endl;
    std::cout << std::endl;
}

// 测试YUV420格式
void TestYUV420Format() {
    std::cout << "=== Test YUV420 Format ===" << std::endl;

    auto surface = MockSurfaceFactory::CreateIConsumerSurface("yuv420_surface");
    surface->SetDefaultWidthAndHeight(640, 480);
    surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_P);

    auto producer = surface->GetProducer();

    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    BufferRequestConfig config = {
        .width = 640,
        .height = 480,
        .strideAlignment = 8,
        .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_P,
        .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
        .timeout = 1000
    };

    GSError ret = producer->RequestBuffer(buffer, fence, config);
    if (ret == GSError::GSERROR_OK) {
        std::cout << "YUV420 buffer size: " << buffer->GetSize() << " bytes" << std::endl;
        std::cout << "Expected size: " << (640 * 480 * 3 / 2) << " bytes" << std::endl;

        // 填充YUV420数据
        void* data = buffer->GetVirAddr();
        auto yPlane = static_cast<uint8_t*>(data);
        auto uPlane = yPlane + 640 * 480;
        auto vPlane = uPlane + (640 * 480 / 4);

        // 填充Y平面（亮度）
        for (int i = 0; i < 640 * 480; i++) {
            yPlane[i] = 128;
        }

        // 填充U平面（色度）
        for (int i = 0; i < 640 * 480 / 4; i++) {
            uPlane[i] = 64;
        }

        // 填充V平面（色度）
        for (int i = 0; i < 640 * 480 / 4; i++) {
            vPlane[i] = 192;
        }

        BufferFlushConfig flushConfig = {
            .damage = {0, 0, 640, 480},
            .timestamp = 0
        };
        producer->FlushBuffer(buffer, fence, flushConfig);
        std::cout << "YUV420 buffer flushed" << std::endl;
    }

    std::cout << std::endl;
}

// 测试多缓冲区队列
void TestMultipleBuffers() {
    std::cout << "=== Test Multiple Buffer Queue ===" << std::endl;

    auto surface = MockSurfaceFactory::CreateIConsumerSurface("multi_buffer_surface");
    surface->SetQueueSize(5);
    surface->SetDefaultWidthAndHeight(1280, 720);
    surface->SetDefaultFormat(GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP);

    auto producer = surface->GetProducer();

    // 请求多个缓冲区
    std::vector<sptr<SurfaceBuffer>> buffers;

    for (int i = 0; i < 3; i++) {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        BufferRequestConfig config = {
            .width = 1280,
            .height = 720,
            .strideAlignment = 8,
            .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
            .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
            .timeout = 1000
        };

        GSError ret = producer->RequestBuffer(buffer, fence, config);
        if (ret == GSError::GSERROR_OK) {
            buffers.push_back(buffer);
            std::cout << "Buffer " << i << " requested, size: " << buffer->GetSize() << std::endl;

            // 填充并刷新
            BufferFlushConfig flushConfig = {
                .damage = {0, 0, 1280, 720},
                .timestamp = i * 1000
            };
            producer->FlushBuffer(buffer, fence, flushConfig);
            std::cout << "Buffer " << i << " flushed" << std::endl;
        }
    }

    // 消费者获取所有缓冲区
    for (int i = 0; i < 3; i++) {
        sptr<SurfaceBuffer> buffer;
        sptr<SyncFence> fence;
        int64_t timestamp = 0;
        Rect damage;

        GSError ret = surface->AcquireBuffer(buffer, fence, timestamp, damage);
        if (ret == GSError::GSERROR_OK) {
            std::cout << "Buffer " << i << " acquired, timestamp: " << timestamp << std::endl;
            surface->ReleaseBuffer(buffer, fence);
            std::cout << "Buffer " << i << " released" << std::endl;
        }
    }

    std::cout << "Active surface count: " << MockSurfaceFactory::GetActiveSurfaceCount() << std::endl;
    std::cout << std::endl;
}

// 测试元数据
void TestMetadata() {
    std::cout << "=== Test Metadata ===" << std::endl;

    auto surface = MockSurfaceFactory::CreateIConsumerSurface("metadata_surface");
    auto producer = surface->GetProducer();

    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    BufferRequestConfig config = {
        .width = 1920,
        .height = 1080,
        .strideAlignment = 8,
        .format = GraphicPixelFormat::PIXEL_FMT_YCBCR_420_SP,
        .usage = static_cast<BufferUsage>(BufferUsage::CPU_READ | BufferUsage::CPU_WRITE),
        .timeout = 1000
    };

    producer->RequestBuffer(buffer, fence, config);

    // 设置元数据
    std::vector<uint8_t> metadata1 = {1, 2, 3, 4, 5};
    buffer->SetMetadata(100, metadata1);

    std::vector<uint8_t> metadata2 = {10, 20, 30, 40};
    buffer->SetMetadata(200, metadata2);

    std::cout << "Metadata set for keys 100 and 200" << std::endl;

    // 获取元数据
    std::vector<uint8_t> value;
    if (buffer->GetMetadata(100, value) == GSError::GSERROR_OK) {
        std::cout << "Metadata 100: ";
        for (auto v : value) {
            std::cout << static_cast<int>(v) << " ";
        }
        std::cout << std::endl;
    }

    value.clear();
    if (buffer->GetMetadata(200, value) == GSError::GSERROR_OK) {
        std::cout << "Metadata 200: ";
        for (auto v : value) {
            std::cout << static_cast<int>(v) << " ";
        }
        std::cout << std::endl;
    }

    std::cout << std::endl;
}

int main() {
    std::cout << "Surface Mock Test Suite" << std::endl;
    std::cout << "=======================" << std::endl;
    std::cout << std::endl;

    TestProducerConsumerPattern();
    TestYUV420Format();
    TestMultipleBuffers();
    TestMetadata();

    std::cout << "=======================" << std::endl;
    std::cout << "All tests completed!" << std::endl;

    // 清理
    MockSurfaceFactory::Reset();
    std::cout << "Factory reset, active surfaces: " << MockSurfaceFactory::GetActiveSurfaceCount() << std::endl;

    return 0;
}
