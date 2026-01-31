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

#include "ffmpeg_dynamic_loader.h"
#include "distributed_hardware_log.h"
#include <sstream>

FFmpegDynamicLoader::FFmpegDynamicLoader()
    : avcodecHandle_(nullptr), avutilHandle_(nullptr),
      initialized_(false) {
    // 初始化所有函数指针为nullptr
    avcodec_send_frame = nullptr;
    avcodec_receive_packet = nullptr;
    avcodec_find_encoder_by_name = nullptr;
    avcodec_find_encoder = nullptr;
    avcodec_alloc_context3 = nullptr;
    avcodec_open2 = nullptr;
    avcodec_free_context = nullptr;
    av_frame_alloc = nullptr;
    av_frame_free = nullptr;
    av_frame_get_buffer = nullptr;
    av_frame_make_writable = nullptr;
    av_frame_unref = nullptr;
    av_packet_alloc = nullptr;
    av_packet_free = nullptr;
    av_packet_unref = nullptr;
}

FFmpegDynamicLoader::~FFmpegDynamicLoader() {
#ifdef _WIN32
    if (avcodecHandle_) {
        FreeLibrary(avcodecHandle_);
    }
    if (avutilHandle_) {
        FreeLibrary(avutilHandle_);
    }
#else
    if (avcodecHandle_) {
        dlclose(avcodecHandle_);
    }
    if (avutilHandle_) {
        dlclose(avutilHandle_);
    }
#endif
}

FFmpegDynamicLoader& FFmpegDynamicLoader::GetInstance() {
    static FFmpegDynamicLoader instance;
    return instance;
}

bool FFmpegDynamicLoader::Initialize(const std::string& dllPath) {
    if (initialized_) {
        DHLOGI("[FFMPEG_LOADER] Already initialized");
        return true;
    }

    DHLOGI("[FFMPEG_LOADER] Initializing FFmpeg dynamic loader...");

    // 加载 avutil DLL (avcodec 依赖它)
    std::string utilPath = dllPath.empty() ? FFMPEG_UTIL_LIB : dllPath + "/" FFMPEG_UTIL_LIB;
    if (!LoadLibrary(utilPath)) {
        lastError_ = "Failed to load avutil library";
        DHLOGE("[FFMPEG_LOADER] %{public}s", lastError_.c_str());
        return false;
    }

    // 加载 avcodec DLL
    std::string codecPath = dllPath.empty() ? FFMPEG_LIB : dllPath + "/" FFMPEG_LIB;
    if (!LoadLibrary(codecPath)) {
        lastError_ = "Failed to load avcodec library";
        DHLOGE("[FFMPEG_LOADER] %{public}s", lastError_.c_str());
        return false;
    }

    // 加载所有需要的函数
    #define LOAD_FUNCTION(dest, name) \
        do { \
            (dest) = (decltype(dest))GetFunction(#name); \
            if (!(dest)) { \
                lastError_ = "Failed to load function: " #name; \
                DHLOGE("[FFMPEG_LOADER] %{public}s", lastError_.c_str()); \
                return false; \
            } \
        } while(0)

    // avcodec 函数
    LOAD_FUNCTION(avcodec_send_frame, avcodec_send_frame);
    LOAD_FUNCTION(avcodec_receive_packet, avcodec_receive_packet);
    LOAD_FUNCTION(avcodec_find_encoder_by_name, avcodec_find_encoder_by_name);
    LOAD_FUNCTION(avcodec_find_encoder, avcodec_find_encoder);
    LOAD_FUNCTION(avcodec_alloc_context3, avcodec_alloc_context3);
    LOAD_FUNCTION(avcodec_open2, avcodec_open2);
    LOAD_FUNCTION(avcodec_free_context, avcodec_free_context);

    // avutil 函数
    LOAD_FUNCTION(av_frame_alloc, av_frame_alloc);
    LOAD_FUNCTION(av_frame_free, av_frame_free);
    LOAD_FUNCTION(av_frame_get_buffer, av_frame_get_buffer);
    LOAD_FUNCTION(av_frame_make_writable, av_frame_make_writable);
    LOAD_FUNCTION(av_frame_unref, av_frame_unref);
    LOAD_FUNCTION(av_packet_alloc, av_packet_alloc);
    LOAD_FUNCTION(av_packet_free, av_packet_free);
    LOAD_FUNCTION(av_packet_unref, av_packet_unref);

    #undef LOAD_FUNCTION

    initialized_ = true;
    DHLOGI("[FFMPEG_LOADER] FFmpeg dynamic loader initialized successfully");
    return true;
}

bool FFmpegDynamicLoader::LoadLibrary(const std::string& libName) {
#ifdef _WIN32
    HMODULE handle = LoadLibraryA(libName.c_str());
    if (!handle) {
        DWORD error = GetLastError();
        std::ostringstream oss;
        oss << "Failed to load " << libName << ", error code: " << error;
        lastError_ = oss.str();
        DHLOGE("[FFMPEG_LOADER] %{public}s", lastError_.c_str());
        return false;
    }

    // 根据库名称分配到正确的句柄
    if (libName.find("avcodec") != std::string::npos) {
        avcodecHandle_ = handle;
    } else if (libName.find("avutil") != std::string::npos) {
        avutilHandle_ = handle;
    }

    DHLOGI("[FFMPEG_LOADER] Loaded library: %{public}s", libName.c_str());
    return true;
#else
    void* handle = dlopen(libName.c_str(), RTLD_LAZY);
    if (!handle) {
        lastError_ = dlerror();
        DHLOGE("[FFMPEG_LOADER] Failed to load %{public}s: %{public}s",
                libName.c_str(), lastError_.c_str());
        return false;
    }

    if (libName.find("avcodec") != std::string::npos) {
        avcodecHandle_ = handle;
    } else if (libName.find("avutil") != std::string::npos) {
        avutilHandle_ = handle;
    }

    DHLOGI("[FFMPEG_LOADER] Loaded library: %{public}s", libName.c_str());
    return true;
#endif
}

void* FFmpegDynamicLoader::GetFunction(const char* name) {
#ifdef _WIN32
    // 先从 avcodec 查找
    if (avcodecHandle_) {
        void* func = (void*)GetProcAddress(avcodecHandle_, name);
        if (func) {
            DHLOGI("[FFMPEG_LOADER] Found function %{public}s in avcodec", name);
            return func;
        }
    }

    // 再从 avutil 查找
    if (avutilHandle_) {
        void* func = (void*)GetProcAddress(avutilHandle_, name);
        if (func) {
            DHLOGI("[FFMPEG_LOADER] Found function %{public}s in avutil", name);
            return func;
        }
    }

    DHLOGE("[FFMPEG_LOADER] Function %{public}s not found", name);
    return nullptr;
#else
    // 先从 avcodec 查找
    if (avcodecHandle_) {
        void* func = dlsym(avcodecHandle_, name);
        if (func) {
            DHLOGI("[FFMPEG_LOADER] Found function %{public}s in avcodec", name);
            return func;
        }
    }

    // 再从 avutil 查找
    if (avutilHandle_) {
        void* func = dlsym(avutilHandle_, name);
        if (func) {
            DHLOGI("[FFMPEG_LOADER] Found function %{public}s in avutil", name);
            return func;
        }
    }

    DHLOGE("[FFMPEG_LOADER] Function %{public}s not found", name);
    return nullptr;
#endif
}
