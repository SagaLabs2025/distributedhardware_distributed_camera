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

#ifndef OHOS_FFMPEG_CODEC_H
#define OHOS_FFMPEG_CODEC_H

#include "platform_interface.h"
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <mutex>
#include <thread>
#include <queue>
#include <condition_variable>

namespace OHOS {
namespace DistributedHardware {

class FFmpegVideoEncoder : public IVideoEncoder {
public:
    FFmpegVideoEncoder();
    ~FFmpegVideoEncoder() override;

    int32_t Init(const VideoConfig& config) override;
    int32_t Configure(const VideoConfig& config) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;

    int32_t FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                           int64_t timestampUs) override;
    int32_t GetOutputBuffer(CodecBufferInfo& bufferInfo,
                           std::shared_ptr<IDataBuffer>& outputBuffer) override;

    void SetErrorCallback(std::function<void()> onError) override;
    void SetInputBufferAvailableCallback(
        std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) override;
    void SetOutputFormatChangedCallback(
        std::function<void(const VideoConfig&)> onFormatChanged) override;
    void SetOutputBufferAvailableCallback(
        std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) override;

private:
    int32_t InitializeEncoder();
    int32_t SetupPixelFormat(AVCodecContext* codecCtx, const VideoConfig& config);
    int32_t EncodeFrame(AVFrame* frame, AVPacket* packet);
    void EncodingThread();

    AVCodecContext* encoderCtx_ = nullptr;
    AVCodec* codec_ = nullptr;
    AVFrame* inputFrame_ = nullptr;
    AVPacket* outputPacket_ = nullptr;
    SwsContext* swsCtx_ = nullptr;

    VideoConfig config_;
    bool isInitialized_ = false;
    bool isStarted_ = false;
    bool stopEncoding_ = false;

    std::thread encodingThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<IDataBuffer>> inputQueue_;
    std::queue<std::pair<CodecBufferInfo, std::shared_ptr<IDataBuffer>>> outputQueue_;

    std::function<void()> onErrorCallback_;
    std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailableCallback_;
    std::function<void(const VideoConfig&)> onFormatChangedCallback_;
    std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailableCallback_;

    uint32_t frameCount_ = 0;
};

class FFmpegVideoDecoder : public IVideoDecoder {
public:
    FFmpegVideoDecoder();
    ~FFmpegVideoDecoder() override;

    int32_t Init(const VideoConfig& config) override;
    int32_t Configure(const VideoConfig& config) override;
    int32_t Start() override;
    int32_t Stop() override;
    int32_t Release() override;

    int32_t FeedInputBuffer(std::shared_ptr<IDataBuffer> inputBuffer,
                           int64_t timestampUs) override;
    int32_t GetOutputBuffer(CodecBufferInfo& bufferInfo,
                           std::shared_ptr<IDataBuffer>& outputBuffer) override;

    void SetErrorCallback(std::function<void()> onError) override;
    void SetInputBufferAvailableCallback(
        std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailable) override;
    void SetOutputFormatChangedCallback(
        std::function<void(const VideoConfig&)> onFormatChanged) override;
    void SetOutputBufferAvailableCallback(
        std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailable) override;

private:
    int32_t InitializeDecoder();
    int32_t DecodePacket(AVPacket* packet, AVFrame* frame);
    void DecodingThread();

    AVCodecContext* decoderCtx_ = nullptr;
    AVCodec* codec_ = nullptr;
    AVFrame* outputFrame_ = nullptr;
    AVPacket* inputPacket_ = nullptr;
    SwsContext* swsCtx_ = nullptr;

    VideoConfig config_;
    bool isInitialized_ = false;
    bool isStarted_ = false;
    bool stopDecoding_ = false;

    std::thread decodingThread_;
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::shared_ptr<IDataBuffer>> inputQueue_;
    std::queue<std::pair<CodecBufferInfo, std::shared_ptr<IDataBuffer>>> outputQueue_;

    std::function<void()> onErrorCallback_;
    std::function<void(uint32_t, std::shared_ptr<IDataBuffer>)> onInputAvailableCallback_;
    std::function<void(const VideoConfig&)> onFormatChangedCallback_;
    std::function<void(const CodecBufferInfo&, std::shared_ptr<IDataBuffer>)> onOutputAvailableCallback_;

    int64_t lastTimestampUs_ = 0;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_FFMPEG_CODEC_H