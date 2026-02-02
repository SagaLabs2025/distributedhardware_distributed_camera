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

#ifndef OHOS_MOCK_VIDEO_SOURCE_H
#define OHOS_MOCK_VIDEO_SOURCE_H

#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include "data_buffer.h"

namespace OHOS {
namespace DistributedHardware {

class MockVideoSource {
public:
    struct VideoConfig {
        int32_t width = 1280;
        int32_t height = 720;
        int32_t fps = 30;
        std::string format = "H264"; // H264, YUV420, MJPEG
        std::string videoFile = ""; // Path to pre-recorded video file
    };

    MockVideoSource() = default;
    ~MockVideoSource() = default;

    static std::shared_ptr<MockVideoSource> GetInstance();

    // Initialize with configuration
    bool Initialize(const VideoConfig& config);

    // Start/Stop video streaming
    bool StartStreaming();
    bool StopStreaming();

    // Get next frame
    std::shared_ptr<DataBuffer> GetNextFrame();

    // Check if initialized and running
    bool IsInitialized() const { return initialized_; }
    bool IsRunning() const { return running_; }

private:
    void StreamThread();
    std::shared_ptr<DataBuffer> GenerateTestFrame();
    std::shared_ptr<DataBuffer> LoadFrameFromFile();

    VideoConfig config_;
    std::atomic<bool> initialized_{false};
    std::atomic<bool> running_{false};
    std::thread streamThread_;
    std::mutex sourceLock_;

    // Frame generation state
    uint32_t frameCounter_ = 0;
    size_t currentFrameOffset_ = 0;
};

// Global instance for easy access
extern std::shared_ptr<MockVideoSource> g_mockVideoSource;

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_MOCK_VIDEO_SOURCE_H