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

#ifndef OHOS_DISTRIBUTED_HARDWARE_LOG_H
#define OHOS_DISTRIBUTED_HARDWARE_LOG_H

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include "dh_log_callback.h"

#ifdef _WIN32
#define vsnprintf vsnprintf_s
#endif

// 简化的日志宏定义，用于Windows测试环境
// 使用回调机制将日志发送到测试程序

#define DHLOGI(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        DH_SendLog(DHLogLevel::DH_INFO, "DHLOG", buffer); \
    } while(0)

#define DHLOGE(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        DH_SendLog(DHLogLevel::DH_ERROR, "DHLOG", buffer); \
    } while(0)

#define DHLOGW(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        DH_SendLog(DHLogLevel::DH_WARN, "DHLOG", buffer); \
    } while(0)

#define DHLOGD(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        DH_SendLog(DHLogLevel::DH_DEBUG, "DHLOG", buffer); \
    } while(0)

// 匿名处理函数（简化版）
inline std::string GetAnonyString(const std::string& str) {
    if (str.length() <= 8) {
        return str;
    }
    return str.substr(0, 4) + "****" + str.substr(str.length() - 4);
}

#endif // OHOS_DISTRIBUTED_HARDWARE_LOG_H
