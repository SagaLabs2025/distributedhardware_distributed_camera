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

#ifndef FFMPEG_DYNAMIC_LOADER_H
#define FFMPEG_DYNAMIC_LOADER_H

#include <string>
#include <memory>

#ifdef _WIN32
#include <windows.h>
#define FFMPEG_LIB "avcodec-60.dll"
#define FFMPEG_UTIL_LIB "avutil-58.dll"
#else
#include <dlfcn.h>
#define FFMPEG_LIB "libavcodec.so.60"
#define FFMPEG_UTIL_LIB "libavutil.so.58"
#endif

/**
 * FFmpeg动态加载器
 * 在运行时加载FFmpeg DLL，避免编译时依赖
 */
class FFmpegDynamicLoader {
public:
    static FFmpegDynamicLoader& GetInstance();

    // 初始化 - 加载FFmpeg DLL
    bool Initialize(const std::string& dllPath = "");

    // 检查是否已初始化
    bool IsInitialized() const { return initialized_; }

    // 获取错误信息
    std::string GetLastError() const { return lastError_; }

    // ===== FFmpeg函数指针类型定义 =====
    typedef int (*avcodec_send_frame_func)(void* c, void* frame);
    typedef int (*avcodec_receive_packet_func)(void* c, void* pkt);
    typedef void* (*avcodec_find_encoder_by_name_func)(const char* name);
    typedef void* (*avcodec_find_encoder_func)(int id);
    typedef void* (*avcodec_alloc_context3_func)(const void* codec);
    typedef int (*avcodec_open2_func)(void* avctx, const void* codec, void* options);
    typedef void (*avcodec_free_context_func)(void** avctx);
    typedef void* (*av_frame_alloc_func)();
    typedef void (*av_frame_free_func)(void** frame);
    typedef int (*av_frame_get_buffer_func)(void* frame, int align);
    typedef int (*av_frame_make_writable_func)(void* frame);
    typedef void (*av_frame_unref_func)(void* frame);
    typedef void* (*av_packet_alloc_func)();
    typedef void (*av_packet_free_func)(void** pkt);
    typedef void (*av_packet_unref_func)(void* pkt);
    typedef void* (*av_image_fill_arrays_func)(...);

    // ===== FFmpeg函数指针 =====
    avcodec_send_frame_func avcodec_send_frame;
    avcodec_receive_packet_func avcodec_receive_packet;
    avcodec_find_encoder_by_name_func avcodec_find_encoder_by_name;
    avcodec_find_encoder_func avcodec_find_encoder;
    avcodec_alloc_context3_func avcodec_alloc_context3;
    avcodec_open2_func avcodec_open2;
    avcodec_free_context_func avcodec_free_context;
    av_frame_alloc_func av_frame_alloc;
    av_frame_free_func av_frame_free;
    av_frame_get_buffer_func av_frame_get_buffer;
    av_frame_make_writable_func av_frame_make_writable;
    av_frame_unref_func av_frame_unref;
    av_packet_alloc_func av_packet_alloc;
    av_packet_free_func av_packet_free;
    av_packet_unref_func av_packet_unref;

private:
    FFmpegDynamicLoader();
    ~FFmpegDynamicLoader();

    bool LoadLibrary(const std::string& libName);
    void* GetFunction(const char* name);

#ifdef _WIN32
    HMODULE avcodecHandle_;
    HMODULE avutilHandle_;
#else
    void* avcodecHandle_;
    void* avutilHandle_;
#endif

    bool initialized_;
    std::string lastError_;
};

#endif // FFMPEG_DYNAMIC_LOADER_H
