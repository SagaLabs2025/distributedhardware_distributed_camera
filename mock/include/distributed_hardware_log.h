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

#ifndef OHOS_DISTRIBUTED_HARDWARE_LOG_H
#define OHOS_DISTRIBUTED_HARDWARE_LOG_H

#include <iostream>
#include <string>
#include <cstdio>
#include <cstdarg>

// Simple log macros for independent project with printf-style formatting
namespace {
    inline void LogMessage(const char* level, const char* fmt, ...) {
        char buffer[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);
        std::cout << "[" << level << "] " << buffer << std::endl;
    }
}

#define DHLOGI(fmt, ...) do { \
    LogMessage("INFO", fmt, ##__VA_ARGS__); \
} while(0)

#define DHLOGE(fmt, ...) do { \
    LogMessage("ERROR", fmt, ##__VA_ARGS__); \
} while(0)

#define DHLOGW(fmt, ...) do { \
    LogMessage("WARN", fmt, ##__VA_ARGS__); \
} while(0)

#define DHLOGD(fmt, ...) do { \
    LogMessage("DEBUG", fmt, ##__VA_ARGS__); \
} while(0)

#endif // OHOS_DISTRIBUTED_HARDWARE_LOG_H