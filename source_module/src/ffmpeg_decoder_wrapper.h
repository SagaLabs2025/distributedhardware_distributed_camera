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

#ifndef FFMPEG_DECODER_WRAPPER_H
#define FFMPEG_DECODER_WRAPPER_H

#include <string>
#include <vector>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

/**
 * FFmpeg解码器封装
 * 用于解码 H.265 数据为 YUV
 *
 * 对应原始代码中的 AVCodec VideoDecoder
 */
class FFmpegDecoderWrapper {
public:
    FFmpegDecoderWrapper();
    ~FFmpegDecoderWrapper();

    // 初始化解码器
    int32_t Initialize();

    // 解码一帧编码数据
    int32_t Decode(const uint8_t* encodedData, int encodedSize,
                   std::vector<uint8_t>& yuvData,
                   int& width, int& height);

    // 刷新解码器
    int32_t Flush();

    // 获取解码器信息
    bool IsInitialized() const { return initialized_.load(); }

private:
    int32_t SendPacket(const uint8_t* data, int size);
    int32_t ReceiveFrame(std::vector<uint8_t>& yuvData, int& width, int& height);

    AVCodecContext* codecContext_;
    const AVCodec* codec_;
    AVFrame* frame_;
    AVPacket* packet_;

    bool initialized_;
    int frameCount_;
};

#endif // FFMPEG_DECODER_WRAPPER_H
