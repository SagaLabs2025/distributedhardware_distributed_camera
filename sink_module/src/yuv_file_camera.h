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

#ifndef YUV_FILE_CAMERA_H
#define YUV_FILE_CAMERA_H

#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

/**
 * YUV文件模拟相机
 * 用于替代 Camera Framework，从YUV文件读取帧数据
 *
 * 对应原始代码中的 Camera Framework 获取 YUV 数据的部分
 */
class YUVFileCamera {
public:
    YUVFileCamera();
    ~YUVFileCamera();

    // 初始化相机
    int32_t Initialize();

    // 打开YUV文件
    int32_t OpenFile(const std::string& filePath, int width, int height);

    // 读取一帧
    int32_t ReadFrame(std::vector<uint8_t>& frameData);

    // 关闭文件
    void Close();

    // 获取当前帧大小
    int32_t GetFrameSize() const { return frameSize_; }

    // 获取分辨率
    void GetResolution(int& width, int& height) const {
        width = width_;
        height = height_;
    }

private:
    std::ifstream file_;
    std::string filePath_;
    int width_;
    int height_;
    int frameSize_;
    bool initialized_;
    bool fileOpened_;

    // 生成默认测试YUV数据（如果没有文件）
    void GenerateTestYUV(std::vector<uint8_t>& yuvData);
};

#endif // YUV_FILE_CAMERA_H
