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

#include "ffmpeg_encoder_wrapper.h"
#include "distributed_hardware_log.h"

FFmpegEncoderWrapper::FFmpegEncoderWrapper()
    : codecContext_(nullptr), codec_(nullptr),
      frame_(nullptr), packet_(nullptr),
      width_(0), height_(0), frameSize_(0),
      frameCount_(0), initialized_(false) {
}

FFmpegEncoderWrapper::~FFmpegEncoderWrapper() {
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

int32_t FFmpegEncoderWrapper::Initialize(int width, int height) {
    DHLOGI("[FFMPEG_ENC] Initialize, resolution: %{public}dx%{public}d", width, height);

    width_ = width;
    height_ = height;
    frameSize_ = width * height * 3 / 2;  // NV12

    // 查找 H.265 编码器
    codec_ = avcodec_find_encoder_by_name("libx265");
    if (!codec_) {
        DHLOGE("[FFMPEG_ENC] Could not find libx265 encoder, trying hevc...");
        codec_ = avcodec_find_encoder(AV_CODEC_ID_HEVC);
    }

    if (!codec_) {
        DHLOGE("[FFMPEG_ENC] Could not find HEVC encoder");
        return -1;
    }

    // 创建编码器上下文
    codecContext_ = avcodec_alloc_context3(codec_);
    if (!codecContext_) {
        DHLOGE("[FFMPEG_ENC] Could not allocate codec context");
        return -1;
    }

    // 配置编码器参数
    codecContext_->width = width;
    codecContext_->height = height;
    codecContext_->time_base = AVRational{1, 30};
    codecContext_->framerate = AVRational{30, 1};
    codecContext_->gop_size = 30;
    codecContext_->max_b_frames = 0;
    codecContext_->pix_fmt = AV_PIX_FMT_NV12;

    // 设置比特率
    codecContext_->bit_rate = 5000000;  // 5 Mbps

    // 打开编码器
    if (avcodec_open2(codecContext_, codec_, nullptr) < 0) {
        DHLOGE("[FFMPEG_ENC] Could not open codec");
        return -1;
    }

    // 创建帧
    frame_ = av_frame_alloc();
    if (!frame_) {
        DHLOGE("[FFMPEG_ENC] Could not allocate frame");
        return -1;
    }

    frame_->format = codecContext_->pix_fmt;
    frame_->width = codecContext_->width;
    frame_->height = codecContext_->height;

    if (av_frame_get_buffer(frame_, 0) < 0) {
        DHLOGE("[FFMPEG_ENC] Could not allocate frame data");
        return -1;
    }

    // 创建数据包
    packet_ = av_packet_alloc();
    if (!packet_) {
        DHLOGE("[FFMPEG_ENC] Could not allocate packet");
        return -1;
    }

    initialized_ = true;
    DHLOGI("[FFMPEG_ENC] Initialize success");
    return 0;
}

int32_t FFmpegEncoderWrapper::Encode(const std::vector<uint8_t>& yuvData,
                                     std::vector<uint8_t>& encodedData) {
    if (!initialized_) {
        DHLOGE("[FFMPEG_ENC] Not initialized");
        return -1;
    }

    if (yuvData.size() < static_cast<size_t>(frameSize_)) {
        DHLOGE("[FFMPEG_ENC] Invalid YUV data size");
        return -1;
    }

    // 准备帧数据
    av_frame_make_writable(frame_);

    // Y 平面
    memcpy(frame_->data[0], yuvData.data(), width_ * height_);

    // UV 平面 (NV12 是交错存储的)
    memcpy(frame_->data[1], yuvData.data() + width_ * height_, width_ * height_ / 2);

    frame_->pts = frameCount_++;

    // 发送帧到编码器
    int32_t ret = SendFrame(frame_);
    if (ret < 0) {
        DHLOGE("[FFMPEG_ENC] Error sending frame to encoder");
        return -1;
    }

    // 接收编码后的数据
    ret = ReceivePacket(encodedData);
    if (ret < 0) {
        DHLOGE("[FFMPEG_ENC] Error receiving packet from encoder");
        return -1;
    }

    return 0;
}

int32_t FFmpegEncoderWrapper::Flush() {
    DHLOGI("[FFMPEG_ENC] Flushing encoder");

    // 发送空帧以刷新编码器
    int32_t ret = SendFrame(nullptr);
    if (ret < 0) {
        DHLOGE("[FFMPEG_ENC] Error flushing encoder");
        return -1;
    }

    return 0;
}

int32_t FFmpegEncoderWrapper::SendFrame(AVFrame* frame) {
    int32_t ret = avcodec_send_frame(codecContext_, frame);
    if (ret < 0) {
        DHLOGE("[FFMPEG_ENC] Error sending frame: %{public}d", ret);
        return ret;
    }
    return 0;
}

int32_t FFmpegEncoderWrapper::ReceivePacket(std::vector<uint8_t>& encodedData) {
    int32_t ret = avcodec_receive_packet(codecContext_, packet_);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        return 0;  // 需要更多输入或结束
    } else if (ret < 0) {
        DHLOGE("[FFMPEG_ENC] Error receiving packet: %{public}d", ret);
        return ret;
    }

    // 复制编码数据
    encodedData.assign(packet_->data, packet_->data + packet_->size);

    av_packet_unref(packet_);
    return 0;
}
