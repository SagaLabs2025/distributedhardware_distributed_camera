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

#ifndef DH_LOG_CALLBACK_H
#define DH_LOG_CALLBACK_H

#include <functional>
#include <string>

#ifdef _WIN32
#define DH_LOG_CALLBACK_EXPORT __declspec(dllexport)
#else
#define DH_LOG_CALLBACK_EXPORT
#endif

// 日志级别（使用与Windows宏不同的名称）
enum class DHLogLevel {
    DH_INFO,
    DH_WARN,
    DH_ERROR,  // 使用DH_前缀避免与Windows ERROR宏冲突
    DH_DEBUG
};

// 日志回调函数类型
using DHLogCallbackFunc = std::function<void(DHLogLevel level, const char* tag, const char* message)>;

// 设置日志回调（由测试程序调用）
extern "C" {
    // 由测试程序实现 - 设置全局回调指针
    extern void (*g_DH_LogCallback)(DHLogLevel, const char*, const char*);

    // 由测试程序实现 - 发送日志
    DH_LOG_CALLBACK_EXPORT void DH_SetLogCallback(DHLogCallbackFunc callback);

    // 设置全局回调指针（供MainController使用）
    DH_LOG_CALLBACK_EXPORT void DH_SetGlobalCallbackPtr(void (*callback)(DHLogLevel, const char*, const char*));

    // 由DLL使用 - 发送日志（会调用测试程序的回调）
    DH_LOG_CALLBACK_EXPORT void DH_SendLog(DHLogLevel level, const char* tag, const char* message);
}

#endif // DH_LOG_CALLBACK_H
