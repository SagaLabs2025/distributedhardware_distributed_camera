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

#include "yuv_file_camera.h"
#include "distributed_hardware_log.h"

#include <cstring>

YUVFileCamera::YUVFileCamera()
    : width_(1920), height_(1080), frameSize_(0),
      initialized_(false), fileOpened_(false) {
    // NV12 格式: Y + UV/2
    frameSize_ = width_ * height_ * 3 / 2;
}

YUVFileCamera::~YUVFileCamera() {
    Close();
}

int32_t YUVFileCamera::Initialize() {
    DHLOGI("[YUV_CAMERA] Initialize, default resolution: %{public}dx%{public}d",
            width_, height_);

    if (initialized_) {
        DHLOGW("[YUV_CAMERA] Already initialized");
        return 0;
    }

    // 默认使用生成的测试数据
    initialized_ = true;
    DHLOGI("[YUV_CAMERA] Initialize success");
    return 0;
}

int32_t YUVFileCamera::OpenFile(const std::string& filePath, int width, int height) {
    DHLOGI("[YUV_CAMERA] Opening file: %{public}s, resolution: %{public}dx%{public}d",
            filePath.c_str(), width, height);

    width_ = width;
    height_ = height;
    frameSize_ = width * height * 3 / 2;

    file_.open(filePath, std::ios::binary);
    if (!file_.is_open()) {
        DHLOGW("[YUV_CAMERA] Failed to open file, will use generated test data");
        // 不返回错误，使用生成的测试数据
        fileOpened_ = false;
        return 0;
    }

    fileOpened_ = true;
    DHLOGI("[YUV_CAMERA] File opened successfully");
    return 0;
}

int32_t YUVFileCamera::ReadFrame(std::vector<uint8_t>& frameData) {
    if (!initialized_) {
        DHLOGE("[YUV_CAMERA] Not initialized");
        return -1;
    }

    frameData.resize(frameSize_);

    if (fileOpened_ && file_.is_open()) {
        // 从文件读取
        file_.read(reinterpret_cast<char*>(frameData.data()), frameSize_);
        if (file_.gcount() < static_cast<std::streamsize>(frameSize_)) {
            // 文件读取完毕，循环播放
            file_.clear();
            file_.seekg(0, std::ios::beg);
            file_.read(reinterpret_cast<char*>(frameData.data()), frameSize_);
        }
    } else {
        // 生成测试数据
        GenerateTestYUV(frameData);
    }

    return 0;
}

void YUVFileCamera::Close() {
    if (file_.is_open()) {
        file_.close();
        fileOpened_ = false;
    }
    initialized_ = false;
    DHLOGI("[YUV_CAMERA] Closed");
}

void YUVFileCamera::GenerateTestYUV(std::vector<uint8_t>& yuvData) {
    // NV12 格式生成测试图案
    int ySize = width_ * height_;
    int uvSize = ySize / 2;

    // Y 平面 - 生成渐变灰度
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            int value = (x + y) % 256;
            yuvData[y * width_ + x] = static_cast<uint8_t>(value);
        }
    }

    // UV 平面 - 生成彩色条纹
    for (int i = 0; i < uvSize; ++i) {
        // 交替生成 U 和 V
        if ((i / width_) % 2 == 0) {
            yuvData[ySize + i] = 128;  // U
        } else {
            yuvData[ySize + i] = 128;  // V
        }
    }
}
