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

#include "distributed_camera_service.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace OHOS::DistributedHardware;

// Source callback implementation
class SourceCallback : public IDCameraSourceCallback {
public:
    void OnSourceError(int32_t errorCode, const std::string& errorMsg) override {
        std::cout << "[SOURCE CALLBACK] Error: " << errorCode << " - " << errorMsg << std::endl;
    }

    void OnSourceEvent(const std::string& event) override {
        std::cout << "[SOURCE CALLBACK] Event: " << event << std::endl;
    }
};

// Sink callback implementation
class SinkCallback : public IDCameraSinkCallback {
public:
    void OnSinkError(int32_t errorCode, const std::string& errorMsg) override {
        std::cout << "[SINK CALLBACK] Error: " << errorCode << " - " << errorMsg << std::endl;
    }

    void OnSinkEvent(const std::string& event) override {
        std::cout << "[SINK CALLBACK] Event: " << event << std::endl;
    }

    void OnVideoDataReceived(std::shared_ptr<DataBuffer> buffer) override {
        std::cout << "[SINK CALLBACK] Video data received: " << buffer->Size() << " bytes" << std::endl;
    }
};

int main() {
    std::cout << "=== Distributed Camera Services Test ===" << std::endl;

    // Create source service
    auto sourceService = DistributedCameraServiceFactory::CreateSourceService();
    auto sourceCallback = std::make_shared<SourceCallback>();

    // Initialize source
    int32_t result = sourceService->InitSource("{}", sourceCallback);
    std::cout << "Source Init: " << (result == 0 ? "SUCCESS" : "FAILED") << std::endl;

    if (result == 0) {
        // Register hardware
        result = sourceService->RegisterDistributedHardware(
            "LOCAL_SINK_DEVICE", "MOCK_CAMERA_001", "REQ_001", "{}");
        std::cout << "Source Register: " << (result == 0 ? "SUCCESS" : "FAILED") << std::endl;

        // Wait a bit for video streaming
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Create sink service
    auto sinkService = DistributedCameraServiceFactory::CreateSinkService();
    auto sinkCallback = std::make_shared<SinkCallback>();

    // Initialize sink
    result = sinkService->InitSink("{}", sinkCallback);
    std::cout << "Sink Init: " << (result == 0 ? "SUCCESS" : "FAILED") << std::endl;

    if (result == 0) {
        // Subscribe to hardware
        result = sinkService->SubscribeLocalHardware("MOCK_CAMERA_001", "{}");
        std::cout << "Sink Subscribe: " << (result == 0 ? "SUCCESS" : "FAILED") << std::endl;

        // Get camera info
        std::string cameraInfo;
        result = sinkService->GetCameraInfo("MOCK_CAMERA_001", cameraInfo);
        std::cout << "Sink GetCameraInfo: " << (result == 0 ? "SUCCESS" : "FAILED")
                  << ", Info: " << cameraInfo << std::endl;

        // Wait a bit for video reception
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Cleanup
    if (sourceService) {
        sourceService->ReleaseSource();
        std::cout << "Source Release completed" << std::endl;
    }

    if (sinkService) {
        sinkService->ReleaseSink();
        std::cout << "Sink Release completed" << std::endl;
    }

    std::cout << "=== Test completed ===" << std::endl;
    return 0;
}