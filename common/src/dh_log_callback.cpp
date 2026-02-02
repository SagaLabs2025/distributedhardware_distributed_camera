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

#include "dh_log_callback.h"
#include <iostream>
#include <mutex>

// 全局日志回调指针（由测试程序设置）
// 使用 __declspec(dllexport) 确保可以被 DLL 和 EXE 共享
#ifdef _WIN32
#define DLL_CALLBACK_EXPORT __declspec(dllexport)
#else
#define DLL_CALLBACK_EXPORT
#endif

extern "C" {
    // 全局回调函数指针（导出以便 EXE 可以直接设置）
    DLL_CALLBACK_EXPORT void (*g_DH_LogCallback)(DHLogLevel, const char*, const char*) = nullptr;

    // 静态存储的std::function（避免被析构）
    static DHLogCallbackFunc* g_callbackHolder = nullptr;

    DH_LOG_CALLBACK_EXPORT void DH_SetLogCallback(DHLogCallbackFunc callback) {
        if (g_callbackHolder) {
            delete g_callbackHolder;
        }
        g_callbackHolder = new DHLogCallbackFunc(callback);
    }

    // 内部函数：获取回调
    DHLogCallbackFunc* DH_GetCallbackHolder() {
        return g_callbackHolder;
    }
}

// 在头文件中定义的DH_SendLog
extern "C" DH_LOG_CALLBACK_EXPORT void DH_SendLog(DHLogLevel level, const char* tag, const char* message);

// 实现DH_SendLog
extern "C" void DH_SendLog(DHLogLevel level, const char* tag, const char* message) {
    // 调用全局回调指针
    if (g_DH_LogCallback) {
        g_DH_LogCallback(level, tag, message);
        return;  // 如果回调存在，不输出到控制台
    }

    // 回调未设置，输出到控制台
    const char* levelStr = "";
    switch (level) {
        case DHLogLevel::DH_INFO:  levelStr = "INFO"; break;
        case DHLogLevel::DH_WARN:  levelStr = "WARN"; break;
        case DHLogLevel::DH_ERROR: levelStr = "ERROR"; break;
        case DHLogLevel::DH_DEBUG: levelStr = "DEBUG"; break;
    }

    if (level == DHLogLevel::DH_ERROR) {
        fprintf(stderr, "[%s] %s: %s\n", levelStr, tag, message);
    } else {
        fprintf(stdout, "[%s] %s: %s\n", levelStr, tag, message);
    }
    fflush(level == DHLogLevel::DH_ERROR ? stderr : stdout);
}

// 设置全局回调指针（供MainController使用）
extern "C" DH_LOG_CALLBACK_EXPORT void DH_SetGlobalCallbackPtr(void (*callback)(DHLogLevel, const char*, const char*)) {
    g_DH_LogCallback = callback;
}
