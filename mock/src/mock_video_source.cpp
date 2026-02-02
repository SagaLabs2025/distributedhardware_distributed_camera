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

#include "mock_video_source.h"
#include "call_tracker.h"
#include "distributed_hardware_log.h"
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iostream>

// 定义类名用于调用跟踪
static constexpr const char* CLASS_NAME = "MockVideoSource";

namespace OHOS {
namespace DistributedHardware {

std::shared_ptr<MockVideoSource> g_mockVideoSource = nullptr;

std::shared_ptr<MockVideoSource> MockVideoSource::GetInstance() {
    if (g_mockVideoSource == nullptr) {
        g_mockVideoSource = std::make_shared<MockVideoSource>();
        std::cout << "[MockVideoSource] >>>>> GetInstance - Created <<<<<" << std::endl;
    }
    return g_mockVideoSource;
}

bool MockVideoSource::Initialize(const VideoConfig& config) {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "Initialize",
        std::to_string(config.width) + "x" + std::to_string(config.height) + "@" +
        std::to_string(config.fps) + "fps " + config.format);

    std::cout << "[MockVideoSource] >>>>> Initialize START <<<<<" << std::endl;
    std::cout << "[MockVideoSource] Config: " << config.width << "x" << config.height
              << "@" << config.fps << "fps format:" << config.format << std::endl;

    std::lock_guard<std::mutex> lock(sourceLock_);

    if (initialized_) {
        std::cout << "[MockVideoSource] Already initialized" << std::endl;
        return true;
    }

    config_ = config;
    initialized_ = true;

    std::cout << "[MockVideoSource] >>>>> Initialize SUCCESS <<<<<" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "Initialize", "SUCCESS");
    return true;
}

bool MockVideoSource::StartStreaming() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StartStreaming");
    std::cout << "[MockVideoSource] >>>>> StartStreaming START <<<<<" << std::endl;

    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!initialized_) {
        std::cout << "[MockVideoSource] ERROR: Not initialized" << std::endl;
        CallTracker::GetInstance().RecordCall(CLASS_NAME, "StartStreaming", "FAILED: Not initialized");
        return false;
    }

    if (running_) {
        std::cout << "[MockVideoSource] Already running" << std::endl;
        return true;
    }

    running_ = true;
    currentFrameOffset_ = 0;
    frameCounter_ = 0;

    streamThread_ = std::thread(&MockVideoSource::StreamThread, this);
    std::cout << "[MockVideoSource] >>>>> StartStreaming SUCCESS (thread started) <<<<<" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StartStreaming", "SUCCESS - thread started");

    return true;
}

bool MockVideoSource::StopStreaming() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StopStreaming");
    std::cout << "[MockVideoSource] >>>>> StopStreaming START <<<<<" << std::endl;

    std::lock_guard<std::mutex> lock(sourceLock_);

    if (!running_) {
        std::cout << "[MockVideoSource] Not running" << std::endl;
        return true;
    }

    running_ = false;

    std::cout << "[MockVideoSource] Waiting for stream thread to finish..." << std::endl;
    if (streamThread_.joinable()) {
        streamThread_.join();
    }

    std::cout << "[MockVideoSource] >>>>> StopStreaming SUCCESS <<<<<" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StopStreaming", "SUCCESS");
    return true;
}

void MockVideoSource::StreamThread() {
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StreamThread", "Thread started");
    std::cout << "[MockVideoSource] >>>>> StreamThread STARTED <<<<<" << std::endl;

    auto frameDuration = std::chrono::microseconds(1000000 / config_.fps);

    while (running_) {
        auto start = std::chrono::steady_clock::now();

        // Generate or load frame
        auto frame = (config_.videoFile.empty()) ? GenerateTestFrame() : LoadFrameFromFile();

        if (frame) {
            // In a real implementation, this would feed the frame to the camera pipeline
            // For now, we just log it
            std::cout << "[MockVideoSource] Generated frame " << frameCounter_
                      << ", size: " << frame->Size() << " bytes" << std::endl;
        }

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        if (elapsed < frameDuration) {
            std::this_thread::sleep_for(frameDuration - elapsed);
        }

        frameCounter_++;
    }

    std::cout << "[MockVideoSource] >>>>> StreamThread EXITED <<<<<" << std::endl;
    CallTracker::GetInstance().RecordCall(CLASS_NAME, "StreamThread", "Thread exited");
}

std::shared_ptr<DataBuffer> MockVideoSource::GetNextFrame() {
    if (!initialized_ || !running_) {
        return nullptr;
    }

    return (config_.videoFile.empty()) ? GenerateTestFrame() : LoadFrameFromFile();
}

std::shared_ptr<DataBuffer> MockVideoSource::GenerateTestFrame() {
    // Generate a simple test pattern based on resolution and format
    size_t frameSize = 0;

    if (config_.format == "H264") {
        // H.264 encoded frame - generate a minimal valid frame
        // This is a simplified implementation - in reality, you'd use ffmpeg/libx264
        frameSize = config_.width * config_.height * 3 / 2; // Approximate size
    } else if (config_.format == "YUV420") {
        // YUV420 planar format
        frameSize = config_.width * config_.height * 3 / 2;
    } else if (config_.format == "MJPEG") {
        // MJPEG frame - generate a simple JPEG-like header
        frameSize = config_.width * config_.height * 2; // Approximate size
    } else {
        // Default to YUV420
        frameSize = config_.width * config_.height * 3 / 2;
    }

    // Create buffer with test pattern
    auto buffer = std::make_shared<DataBuffer>(frameSize);
    if (!buffer->Data()) {
        DHLOGE("Failed to allocate frame buffer");
        return nullptr;
    }

    // Fill with test pattern (simple gradient)
    uint8_t* data = buffer->Data();
    for (size_t i = 0; i < frameSize; i++) {
        data[i] = static_cast<uint8_t>((i + frameCounter_ * 10) % 256);
    }

    return buffer;
}

std::shared_ptr<DataBuffer> MockVideoSource::LoadFrameFromFile() {
    // This is a placeholder implementation
    // In a real implementation, you'd use ffmpeg to decode video files

    // Check if we have a video file configured
    if (config_.videoFile.empty()) {
        DHLOGW("No video file configured, using test pattern");
        return GenerateTestFrame();
    }

    // TODO: Integrate ffmpeg for actual video file decoding
    // Example integration points:
    // - Use libavformat to open and read video file
    // - Use libavcodec to decode frames
    // - Use libswscale to convert formats if needed
    // - Use libyuv for additional format conversions

    DHLOGI("Loading frames from file: %s", config_.videoFile.c_str());
    DHLOGW("Video file loading not implemented yet - using test pattern");
    return GenerateTestFrame();
}

} // namespace DistributedHardware
} // namespace OHOS