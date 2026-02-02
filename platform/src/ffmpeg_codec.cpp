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

#include "ffmpeg_codec.h"
#include <cstring>
#include <iostream>

namespace OHOS {
namespace DistributedHardware {

// Helper function to convert VideoCodecType to AVCodecID
static AVCodecID GetCodecID(VideoCodecType codecType) {
    switch (codecType) {
        case VideoCodecType::H264:
            return AV_CODEC_ID_H264;
        case VideoCodecType::H265:
            return AV_CODEC_ID_H265;
        case VideoCodecType::VP8:
            return AV_CODEC_ID_VP8;
        case VideoCodecType::VP9:
            return AV_CODEC_ID_VP9;
        default:
            return AV_CODEC_ID_H264;
    }
}

// Helper function to convert VideoPixelFormat to AVPixelFormat
static AVPixelFormat GetPixelFormat(VideoPixelFormat pixelFormat) {
    switch (pixelFormat) {
        case VideoPixelFormat::NV12:
            return AV_PIX_FMT_NV12;
        case VideoPixelFormat::NV21:
            return AV_PIX_FMT_NV21;
        case VideoPixelFormat::YUV420P:
            return AV_PIX_FMT_YUV420P;
        case VideoPixelFormat::RGB32:
            return AV_PIX_FMT_RGB32;
        case VideoPixelFormat::RGBA:
            return AV_PIX_FMT_RGBA;
        default:
            return AV_PIX_FMT_YUV420P;
    }
}

FFmpegVideoEncoder::FFmpegVideoEncoder() : encoderCtx_(nullptr), codec_(nullptr),
    inputFrame_(nullptr), outputPacket_(nullptr), swsCtx_(nullptr),
    isInitialized_(false), isStarted_(false), stopEncoding_(false) {
    // Initialize FFmpeg
    av_register_all();
    avcodec_register_all();
}

FFmpegVideoEncoder::~FFmpegVideoEncoder() {
    Release();
}

int32_t FFmpegVideoEncoder::Init(const VideoConfig& config) {
    if (isInitialized_) {
        return -1; // Already initialized
    }

    config_ = config;
    int32_t result = InitializeEncoder();
    if (result == 0) {
        isInitialized_ = true;
    }
    return result;
}

int32_t FFmpegVideoEncoder::Configure(const VideoConfig& config) {
    if (!isInitialized_) {
        return -1; // Not initialized
    }

    // For simplicity, we'll just update the config
    // In a real implementation, you might need to reconfigure the encoder
    config_ = config;
    return 0;
}

int32_t FFmpegVideoEncoder::Start() {
    if (!isInitialized_) {
        return -1; // Not initialized
    }

    if (isStarted_) {
        return 0; // Already started
    }

    stopEncoding_ = false;
    encodingThread_ = std::thread(&FFmpegVideoEncoder::EncodingThread, this);
    isStarted_ = true;
    return 0;
}

int32_t FFmpegVideoEncoder::Stop() {
    if (!isStarted_) {
        return 0; // Already stopped
    }

    stopEncoding_ = true;
    cv_.notify_all();

    if (encodingThread_.joinable()) {
        encodingThread_.join();
    }

    isStarted_ = false;
    return 0;
}

int32_t FFmpegVideoEncoder::Release() {
    Stop();

    if (encoderCtx_) {
        avcodec_free_context(&encoderCtx_);
        encoderCtx_ = nullptr;
    }

    if (inputFrame_) {
        av_frame_free(&inputFrame_);
        inputFrame_ = nullptr;
    }

    if (outputPacket_) {
        av_packet_free(&outputPacket_);
        outputPacket_ = nullptr;
    }

    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }

    isInitialized_ = false;
    return 0;
}

int32_t FFmpegVideoEncoder::FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                                          int64_t timestampUs) {
    if (!isStarted_) {
        return -1; // Not started
    }

    if (!inputBuffer || !inputBuffer->IsValid()) {
        return -1; // Invalid buffer
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        inputQueue_.push(inputBuffer);
    }
    cv_.notify_all();
    return 0;
}

int32_t FFmpegVideoEncoder::GetOutputBuffer(CodecBufferInfo& bufferInfo,
                                          std::shared_ptr<IDataBuffer>& outputBuffer) {
    if (!isStarted_) {
        return -1; // Not started
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (outputQueue_.empty()) {
        return -1; // No output available
    }

    auto& outputPair = outputQueue_.front();
    bufferInfo = outputPair.first;
    outputBuffer = outputPair.second;
    outputQueue_.pop();
    return 0;
}

void FFmpegVideoEncoder::SetErrorCallback(std::function<void()> onError) {
    onErrorCallback_ = onError;
}

void FFmpegVideoEncoder::SetInputBufferAvailableCallback(
    std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) {
    onInputAvailableCallback_ = onInputAvailable;
}

void FFmpegVideoEncoder::SetOutputFormatChangedCallback(
    std::function<void(const VideoConfig&)> onFormatChanged) {
    onFormatChangedCallback_ = onFormatChanged;
}

void FFmpegVideoEncoder::SetOutputBufferAvailableCallback(
    std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) {
    onOutputAvailableCallback_ = onOutputAvailable;
}

int32_t FFmpegVideoEncoder::InitializeEncoder() {
    // Find encoder
    AVCodecID codecId = GetCodecID(config_.codecType);
    codec_ = avcodec_find_encoder(codecId);
    if (!codec_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate codec context
    encoderCtx_ = avcodec_alloc_context3(codec_);
    if (!encoderCtx_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Configure codec context
    encoderCtx_->width = config_.width;
    encoderCtx_->height = config_.height;
    encoderCtx_->time_base = {1, static_cast<int>(config_.fps)};
    encoderCtx_->framerate = {static_cast<int>(config_.fps), 1};
    encoderCtx_->bit_rate = config_.bitrate;
    encoderCtx_->gop_size = config_.keyFrameInterval;
    encoderCtx_->max_b_frames = 1;
    encoderCtx_->pix_fmt = GetPixelFormat(config_.pixelFormat);

    // Open codec
    if (avcodec_open2(encoderCtx_, codec_, nullptr) < 0) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate frame
    inputFrame_ = av_frame_alloc();
    if (!inputFrame_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    inputFrame_->format = encoderCtx_->pix_fmt;
    inputFrame_->width = encoderCtx_->width;
    inputFrame_->height = encoderCtx_->height;

    if (av_frame_get_buffer(inputFrame_, 32) < 0) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate packet
    outputPacket_ = av_packet_alloc();
    if (!outputPacket_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    return 0;
}

int32_t FFmpegVideoEncoder::EncodeFrame(AVFrame* frame, AVPacket* packet) {
    int ret = avcodec_send_frame(encoderCtx_, frame);
    if (ret < 0) {
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_packet(encoderCtx_, packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            return ret;
        }

        // Packet is ready, add to output queue
        if (packet->size > 0) {
            auto outputBuffer = std::make_shared<MockDataBuffer>(
                reinterpret_cast<const uint8_t*>(packet->data), packet->size);

            CodecBufferInfo bufferInfo;
            bufferInfo.index = frameCount_;
            bufferInfo.offset = 0;
            bufferInfo.size = packet->size;
            bufferInfo.presentationTimestamp = packet->pts;
            bufferInfo.isKeyFrame = (packet->flags & AV_PKT_FLAG_KEY) != 0;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                outputQueue_.emplace(bufferInfo, outputBuffer);
            }

            if (onOutputAvailableCallback_) {
                onOutputAvailableCallback_(bufferInfo, outputBuffer);
            }
        }

        av_packet_unref(packet);
    }

    frameCount_++;
    return 0;
}

void FFmpegVideoEncoder::EncodingThread() {
    while (!stopEncoding_) {
        std::shared_ptr<IDataBuffer> inputBuffer;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !inputQueue_.empty() || stopEncoding_; });

            if (stopEncoding_) {
                break;
            }

            if (!inputQueue_.empty()) {
                inputBuffer = inputQueue_.front();
                inputQueue_.pop();
            }
        }

        if (inputBuffer && inputBuffer->IsValid()) {
            // Copy input data to frame
            size_t expectedSize = av_image_get_buffer_size(
                static_cast<AVPixelFormat>(inputFrame_->format),
                inputFrame_->width, inputFrame_->height, 1);

            if (inputBuffer->Size() >= expectedSize) {
                av_image_fill_arrays(
                    inputFrame_->data, inputFrame_->linesize,
                    inputBuffer->ConstData(),
                    static_cast<AVPixelFormat>(inputFrame_->format),
                    inputFrame_->width, inputFrame_->height, 1);

                inputFrame_->pts = frameCount_;

                EncodeFrame(inputFrame_, outputPacket_);
            }
        }
    }
}

// FFmpegVideoDecoder implementation
FFmpegVideoDecoder::FFmpegVideoDecoder() : decoderCtx_(nullptr), codec_(nullptr),
    outputFrame_(nullptr), inputPacket_(nullptr), swsCtx_(nullptr),
    isInitialized_(false), isStarted_(false), stopDecoding_(false) {
    // Initialize FFmpeg
    av_register_all();
    avcodec_register_all();
}

FFmpegVideoDecoder::~FFmpegVideoDecoder() {
    Release();
}

int32_t FFmpegVideoDecoder::Init(const VideoConfig& config) {
    if (isInitialized_) {
        return -1; // Already initialized
    }

    config_ = config;
    int32_t result = InitializeDecoder();
    if (result == 0) {
        isInitialized_ = true;
    }
    return result;
}

int32_t FFmpegVideoDecoder::Configure(const VideoConfig& config) {
    if (!isInitialized_) {
        return -1; // Not initialized
    }

    config_ = config;
    return 0;
}

int32_t FFmpegVideoDecoder::Start() {
    if (!isInitialized_) {
        return -1; // Not initialized
    }

    if (isStarted_) {
        return 0; // Already started
    }

    stopDecoding_ = false;
    decodingThread_ = std::thread(&FFmpegVideoDecoder::DecodingThread, this);
    isStarted_ = true;
    return 0;
}

int32_t FFmpegVideoDecoder::Stop() {
    if (!isStarted_) {
        return 0; // Already stopped
    }

    stopDecoding_ = true;
    cv_.notify_all();

    if (decodingThread_.joinable()) {
        decodingThread_.join();
    }

    isStarted_ = false;
    return 0;
}

int32_t FFmpegVideoDecoder::Release() {
    Stop();

    if (decoderCtx_) {
        avcodec_free_context(&decoderCtx_);
        decoderCtx_ = nullptr;
    }

    if (outputFrame_) {
        av_frame_free(&outputFrame_);
        outputFrame_ = nullptr;
    }

    if (inputPacket_) {
        av_packet_free(&inputPacket_);
        inputPacket_ = nullptr;
    }

    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }

    isInitialized_ = false;
    return 0;
}

int32_t FFmpegVideoDecoder::FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                                          int64_t timestampUs) {
    if (!isStarted_) {
        return -1; // Not started
    }

    if (!inputBuffer || !inputBuffer->IsValid()) {
        return -1; // Invalid buffer
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        inputQueue_.push(inputBuffer);
    }
    cv_.notify_all();
    return 0;
}

int32_t FFmpegVideoDecoder::GetOutputBuffer(CodecBufferInfo& bufferInfo,
                                          std::shared_ptr<IDataBuffer>& outputBuffer) {
    if (!isStarted_) {
        return -1; // Not started
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (outputQueue_.empty()) {
        return -1; // No output available
    }

    auto& outputPair = outputQueue_.front();
    bufferInfo = outputPair.first;
    outputBuffer = outputPair.second;
    outputQueue_.pop();
    return 0;
}

void FFmpegVideoDecoder::SetErrorCallback(std::function<void()> onError) {
    onErrorCallback_ = onError;
}

void FFmpegVideoDecoder::SetInputBufferAvailableCallback(
    std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) {
    onInputAvailableCallback_ = onInputAvailable;
}

void FFmpegVideoDecoder::SetOutputFormatChangedCallback(
    std::function<void(const VideoConfig&)> onFormatChanged) {
    onFormatChangedCallback_ = onFormatChanged;
}

void FFmpegVideoDecoder::SetOutputBufferAvailableCallback(
    std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) {
    onOutputAvailableCallback_ = onOutputAvailable;
}

int32_t FFmpegVideoDecoder::InitializeDecoder() {
    // Find decoder
    AVCodecID codecId = GetCodecID(config_.codecType);
    codec_ = avcodec_find_decoder(codecId);
    if (!codec_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate codec context
    decoderCtx_ = avcodec_alloc_context3(codec_);
    if (!decoderCtx_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Open codec
    if (avcodec_open2(decoderCtx_, codec_, nullptr) < 0) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate frame
    outputFrame_ = av_frame_alloc();
    if (!outputFrame_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    // Allocate packet
    inputPacket_ = av_packet_alloc();
    if (!inputPacket_) {
        if (onErrorCallback_) {
            onErrorCallback_();
        }
        return -1;
    }

    return 0;
}

int32_t FFmpegVideoDecoder::DecodePacket(AVPacket* packet, AVFrame* frame) {
    int ret = avcodec_send_packet(decoderCtx_, packet);
    if (ret < 0) {
        return ret;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(decoderCtx_, frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return 0;
        } else if (ret < 0) {
            return ret;
        }

        // Frame is ready, convert to desired format and add to output queue
        AVPixelFormat targetFormat = GetPixelFormat(config_.pixelFormat);

        if (frame->format != targetFormat) {
            // Need to convert format
            SwsContext* swsCtx = sws_getContext(
                frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
                frame->width, frame->height, targetFormat,
                SWS_BILINEAR, nullptr, nullptr, nullptr);

            if (swsCtx) {
                AVFrame* convertedFrame = av_frame_alloc();
                convertedFrame->format = targetFormat;
                convertedFrame->width = frame->width;
                convertedFrame->height = frame->height;

                av_frame_get_buffer(convertedFrame, 32);
                sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height,
                         convertedFrame->data, convertedFrame->linesize);

                // Create output buffer
                size_t bufferSize = av_image_get_buffer_size(
                    targetFormat, frame->width, frame->height, 1);
                auto outputBuffer = std::make_shared<MockDataBuffer>(bufferSize);

                av_image_copy_to_buffer(
                    outputBuffer->Data(), bufferSize,
                    convertedFrame->data, convertedFrame->linesize,
                    targetFormat, frame->width, frame->height, 1);

                CodecBufferInfo bufferInfo;
                bufferInfo.index = static_cast<uint32_t>(lastTimestampUs_);
                bufferInfo.offset = 0;
                bufferInfo.size = bufferSize;
                bufferInfo.presentationTimestamp = lastTimestampUs_;
                bufferInfo.isKeyFrame = (frame->key_frame != 0);

                {
                    std::lock_guard<std::mutex> lock(mutex_);
                    outputQueue_.emplace(bufferInfo, outputBuffer);
                }

                if (onOutputAvailableCallback_) {
                    onOutputAvailableCallback_(bufferInfo, outputBuffer);
                }

                av_frame_free(&convertedFrame);
                sws_freeContext(swsCtx);
            }
        } else {
            // No conversion needed
            size_t bufferSize = av_image_get_buffer_size(
                targetFormat, frame->width, frame->height, 1);
            auto outputBuffer = std::make_shared<MockDataBuffer>(bufferSize);

            av_image_copy_to_buffer(
                outputBuffer->Data(), bufferSize,
                frame->data, frame->linesize,
                targetFormat, frame->width, frame->height, 1);

            CodecBufferInfo bufferInfo;
            bufferInfo.index = static_cast<uint32_t>(lastTimestampUs_);
            bufferInfo.offset = 0;
            bufferInfo.size = bufferSize;
            bufferInfo.presentationTimestamp = lastTimestampUs_;
            bufferInfo.isKeyFrame = (frame->key_frame != 0);

            {
                std::lock_guard<std::mutex> lock(mutex_);
                outputQueue_.emplace(bufferInfo, outputBuffer);
            }

            if (onOutputAvailableCallback_) {
                onOutputAvailableCallback_(bufferInfo, outputBuffer);
            }
        }
    }

    return 0;
}

void FFmpegVideoDecoder::DecodingThread() {
    while (!stopDecoding_) {
        std::shared_ptr<IDataBuffer> inputBuffer;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return !inputQueue_.empty() || stopDecoding_; });

            if (stopDecoding_) {
                break;
            }

            if (!inputQueue_.empty()) {
                inputBuffer = inputQueue_.front();
                inputQueue_.pop();
            }
        }

        if (inputBuffer && inputBuffer->IsValid()) {
            // Copy input data to packet
            inputPacket_->data = const_cast<uint8_t*>(inputBuffer->ConstData());
            inputPacket_->size = static_cast<int>(inputBuffer->Size());
            inputPacket_->pts = lastTimestampUs_;

            DecodePacket(inputPacket_, outputFrame_);

            av_packet_unref(inputPacket_);
        }
    }
}

} // namespace DistributedHardware
} // namespace OHOS