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

#ifndef FFMPEG_ENCODER_WRAPPER_H
#define FFMPEG_ENCODER_WRAPPER_H

#include <string>
#include <vector>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

/**
 * FFmpeg编码器封装
 * 用于编码 YUV 数据为 H.265
 *
 * 对应原始代码中的 AVCodec VideoEncoder
 */
class FFmpegEncoderWrapper {
public:
    FFmpegEncoderWrapper();
    ~FFmpegEncoderWrapper();

    // 初始化编码器
    int32_t Initialize(int width, int height);

    // 编码一帧 YUV 数据
    int32_t Encode(const std::vector<uint8_t>& yuvData,
                   std::vector<uint8_t>& encodedData);

    // 刷新编码器
    int32_t Flush();

    // 获取编码器信息
    void GetResolution(int& width, int& height) const {
        width = width_;
        height = height_;
    }

private:
    int32_t SendFrame(AVFrame* frame);
    int32_t ReceivePacket(std::vector<uint8_t>& encodedData);

    AVCodecContext* codecContext_;
    const AVCodec* codec_;
    AVFrame* frame_;
    AVPacket* packet_;

    int width_;
    int height_;
    int frameSize_;
    int frameCount_;
    bool initialized_;
};

#endif // FFMPEG_ENCODER_WRAPPER_H
