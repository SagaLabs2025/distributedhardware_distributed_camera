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

#include "platform_interface.h"
#include "platform_compatibility_adapter.h"
#include <iostream>
#include <memory>

namespace OHOS {
namespace DistributedHardware {

// Example usage of the platform abstraction layer

class PlatformExample {
public:
    void RunExample() {
        // Get the platform factory
        auto factory = g_platformFactory;

        // Create device manager
        auto deviceManager = factory->CreateDeviceManager();

        // Get trusted devices
        std::vector<DeviceInfo> devices;
        int32_t result = deviceManager->GetTrustedDeviceList("test_app", "", devices);
        if (result == 0) {
            std::cout << "Found " << devices.size() << " trusted devices" << std::endl;
        }

        // Create HDF device manager
        auto hdfManager = factory->CreateHdfDeviceManager();

        // Get camera IDs
        std::vector<std::string> cameraIds;
        result = hdfManager->GetCameraIds(cameraIds);
        if (result == 0) {
            std::cout << "Found " << cameraIds.size() << " cameras" << std::endl;
        }

        // Create communication adapter
        auto commAdapter = factory->CreateCommunicationAdapter();

        // Create client connection
        int32_t socketId = commAdapter->CreateClient(
            "test_dh_id", "local_device", "remote_session", "remote_device",
            SessionMode::CONTROL_SESSION);

        if (socketId >= 0) {
            std::cout << "Connected with socket ID: " << socketId << std::endl;

            // Send test data
            auto testData = factory->CreateDataBuffer(1024);
            testData->FillWithPattern(0x55);
            result = commAdapter->SendBytes(socketId, testData);

            if (result == 0) {
                std::cout << "Data sent successfully" << std::endl;
            }

            // Close connection
            commAdapter->CloseSession(socketId);
        }

        // Create video encoder
        auto encoder = factory->CreateVideoEncoder();

        VideoConfig config;
        config.width = 1920;
        config.height = 1080;
        config.fps = 30;
        config.codecType = VideoCodecType::H264;
        config.pixelFormat = VideoPixelFormat::NV12;
        config.bitrate = 5000000; // 5 Mbps
        config.keyFrameInterval = 30;

        result = encoder->Init(config);
        if (result == 0) {
            std::cout << "Video encoder initialized successfully" << std::endl;

            // Start encoding
            encoder->Start();

            // Feed input frame
            auto inputFrame = factory->CreateDataBuffer(1920 * 1080 * 3 / 2); // NV12 size
            inputFrame->FillWithPattern(0x77);
            encoder->FeedInputBuffer(inputFrame, 0);

            // Get encoded output
            CodecBufferInfo bufferInfo;
            std::shared_ptr<IDataBuffer> outputBuffer;
            result = encoder->GetOutputBuffer(bufferInfo, outputBuffer);
            if (result == 0) {
                std::cout << "Encoded frame size: " << outputBuffer->Size() << " bytes" << std::endl;
            }

            // Stop and release
            encoder->Stop();
            encoder->Release();
        }

        // Create video decoder
        auto decoder = factory->CreateVideoDecoder();

        result = decoder->Init(config);
        if (result == 0) {
            std::cout << "Video decoder initialized successfully" << std::endl;

            // Start decoding
            decoder->Start();

            // Feed encoded data (from encoder output)
            if (outputBuffer && outputBuffer->IsValid()) {
                decoder->FeedInputBuffer(outputBuffer, 0);

                // Get decoded output
                CodecBufferInfo decodedInfo;
                std::shared_ptr<IDataBuffer> decodedBuffer;
                result = decoder->GetOutputBuffer(decodedInfo, decodedBuffer);
                if (result == 0) {
                    std::cout << "Decoded frame size: " << decodedBuffer->Size() << " bytes" << std::endl;
                }
            }

            // Stop and release
            decoder->Stop();
            decoder->Release();
        }
    }
};

} // namespace DistributedHardware
} // namespace OHOS