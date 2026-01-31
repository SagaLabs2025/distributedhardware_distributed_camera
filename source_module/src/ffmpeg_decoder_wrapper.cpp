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

#include "ffmpeg_decoder_wrapper.h"
#include "distributed_hardware_log.h"

#include <cstring>

FFmpegDecoderWrapper::FFmpegDecoderWrapper()
    : codecContext_(nullptr), codec_(nullptr),
      frame_(nullptr), packet_(nullptr),
      initialized_(false), frameCount_(0) {
}

FFmpegDecoderWrapper::~FFmpegDecoderWrapper() {
    if (packet_) {
        av_packet_free(&packet_);
    }
    if (frame_) {
        av_frame_free(&frame_);
    }
    if (codecContext_) {
        avcodec_free_context(&codecContext_);
    }
}

int32_t FFmpegDecoderWrapper::Initialize() {
    DHLOGI("[FFMPEG_DEC] Initialize");

    // 查找 H.265 解码器
    codec_ = avcodec_find_decoder(AV_CODEC_ID_HEVC);
    if (!codec_) {
        DHLOGE("[FFMPEG_DEC] Could not find HEVC decoder");
        return -1;
    }

    // 创建解码器上下文
    codecContext_ = avcodec_alloc_context3(codec_);
    if (!codecContext_) {
        DHLOGE("[FFMPEG_DEC] Could not allocate codec context");
        return -1;
    }

    // 打开解码器
    if (avcodec_open2(codecContext_, codec_, nullptr) < 0) {
        DHLOGE("[FFMPEG_DEC] Could not open codec");
        return -1;
    }

    // 创建帧
    frame_ = av_frame_alloc();
    if (!frame_) {
        DHLOGE("[FFMPEG_DEC] Could not allocate frame");
        return -1;
    }

    // 创建数据包
    packet_ = av_packet_alloc();
    if (!packet_) {
        DHLOGE("[FFMPEG_DEC] Could not allocate packet");
        return -1;
    }

    initialized_.store(true);
    DHLOGI("[FFMPEG_DEC] Initialize success");
    return 0;
}

int32_t FFmpegDecoderWrapper::Decode(const uint8_t* encodedData, int encodedSize,
                                      std::vector<uint8_t>& yuvData,
                                      int& width, int& height) {
    if (!initialized_.load()) {
        DHLOGE("[FFMPEG_DEC] Not initialized");
        return -1;
    }

    if (!encodedData || encodedSize <= 0) {
        DHLOGE("[FFMPEG_DEC] Invalid encoded data");
        return -1;
    }

    // 发送数据包到解码器
    int32_t ret = SendPacket(encodedData, encodedSize);
    if (ret < 0) {
        DHLOGE("[FFMPEG_DEC] Error sending packet to decoder");
        return -1;
    }

    // 接收解码后的帧
    ret = ReceiveFrame(yuvData, width, height);
    if (ret < 0) {
        DHLOGE("[FFMPEG_DEC] Error receiving frame from decoder");
        return -1;
    }

    return 0;
}

int32_t FFmpegDecoderWrapper::Flush() {
    DHLOGI("[FFMPEG_DEC] Flushing decoder");

    // 发送空数据包以刷新解码器
    int32_t ret = SendPacket(nullptr, 0);
    if (ret < 0) {
        DHLOGE("[FFMPEG_DEC] Error flushing decoder");
        return -1;
    }

    return 0;
}

int32_t FFmpegDecoderWrapper::SendPacket(const uint8_t* data, int size) {
    if (data && size > 0) {
        packet_->data = const_cast<uint8_t*>(data);
        packet_->size = size;
    } else {
        packet_->data = nullptr;
        packet_->size = 0;
    }

    int32_t ret = avcodec_send_packet(codecContext_, packet_);
    if (ret < 0) {
        DHLOGE("[FFMPEG_DEC] Error sending packet: %{public}d", ret);
        return ret;
    }

    return 0;
}

int32_t FFmpegDecoderWrapper::ReceiveFrame(std::vector<uint8_t>& yuvData, int& width, int& height) {
    int32_t ret = avcodec_receive_frame(codecContext_, frame_);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        // 需要更多输入或结束
        return 0;
    } else if (ret < 0) {
        DHLOGE("[FFMPEG_DEC] Error receiving frame: %{public}d", ret);
        return ret;
    }

    // 获取解码后的 YUV 数据
    width = frame_->width;
    height = frame_->height;

    // NV12 格式: Y + UV/2
    int ySize = width * height;
    int uvSize = ySize / 2;
    int totalSize = ySize + uvSize;

    yuvData.resize(totalSize);

    // Y 平面
    if (frame_->linesize[0] == width) {
        memcpy(yuvData.data(), frame_->data[0], ySize);
    } else {
        // 有步长，需要逐行复制
        for (int i = 0; i < height; ++i) {
            memcpy(yuvData.data() + i * width,
                   frame_->data[0] + i * frame_->linesize[0],
                   width);
        }
    }

    // UV 平面
    if (frame_->linesize[1] == width) {
        memcpy(yuvData.data() + ySize, frame_->data[1], uvSize);
    } else {
        // 有步长，需要逐行复制
        for (int i = 0; i < height / 2; ++i) {
            memcpy(yuvData.data() + ySize + i * width,
                   frame_->data[1] + i * frame_->linesize[1],
                   width);
        }
    }

    av_frame_unref(frame_);
    frameCount_++;

    DHLOGD("[FFMPEG_DEC] Decoded frame %{public}d: %{public}dx%{public}d",
            frameCount_, width, height);

    return 0;
}
