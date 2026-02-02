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

// ANSI color codes for terminal output
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_WHITE   "\033[37m"
#define ANSI_COLOR_RESET   "\033[0m"

// Log tag - can be overridden by defining DH_LOG_TAG before including this header
#ifndef DH_LOG_TAG
#define DH_LOG_TAG "DCAMERA"
#endif

// Extract filename from full path
#define DCAMERA_FILENAME (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : \
                          __builtin_strrchr(__FILE__, '\\') ? __builtin_strrchr(__FILE__, '\\') + 1 : __FILE__)

namespace OHOS {
namespace DistributedHardware {

// Helper function for colored console logging with formatted output
namespace detail {
    inline void LogMessage(const char* level, const char* color, const char* file, int line,
                           const char* fmt, ...) {
        char buffer[2048];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buffer, sizeof(buffer), fmt, args);
        va_end(args);

        // Output format: [TAG] [LEVEL] [FILE:LINE] message
        printf("%s[%s] [%s] [%s:%d] %s%s\n",
               color, DH_LOG_TAG, level, file, line, buffer, ANSI_COLOR_RESET);
    }
} // namespace detail

// Log macros with color output
// DHLOGI - INFO level (Green)
#define DHLOGI(fmt, ...) \
    do { \
        OHOS::DistributedHardware::detail::LogMessage("INFO", ANSI_COLOR_GREEN, \
                                                      DCAMERA_FILENAME, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

// DHLOGE - ERROR level (Red)
#define DHLOGE(fmt, ...) \
    do { \
        OHOS::DistributedHardware::detail::LogMessage("ERROR", ANSI_COLOR_RED, \
                                                      DCAMERA_FILENAME, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

// DHLOGW - WARN level (Yellow)
#define DHLOGW(fmt, ...) \
    do { \
        OHOS::DistributedHardware::detail::LogMessage("WARN", ANSI_COLOR_YELLOW, \
                                                      DCAMERA_FILENAME, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

// DHLOGD - DEBUG level (White)
#define DHLOGD(fmt, ...) \
    do { \
        OHOS::DistributedHardware::detail::LogMessage("DEBUG", ANSI_COLOR_WHITE, \
                                                      DCAMERA_FILENAME, __LINE__, fmt, ##__VA_ARGS__); \
    } while(0)

// Helper macros for checking conditions and logging
#define CHECK_AND_RETURN_RET_LOG(cond, ret, fmt, ...)   \
    do {                                                \
        if ((cond)) {                                   \
            DHLOGE(fmt, ##__VA_ARGS__);                 \
            return (ret);                               \
        }                                               \
    } while (0)

#define CHECK_AND_RETURN_LOG(cond, fmt, ...)   \
    do {                                       \
        if ((cond)) {                          \
            DHLOGE(fmt, ##__VA_ARGS__);        \
            return;                            \
        }                                      \
    } while (0)

#define CHECK_AND_LOG(cond, fmt, ...)          \
    do {                                       \
        if ((cond)) {                          \
            DHLOGE(fmt, ##__VA_ARGS__);        \
        }                                      \
    } while (0)

#define CHECK_NULL_RETURN(cond, ret, ...)       \
    do {                                        \
        if ((cond)) {                           \
            return (ret);                       \
        }                                       \
    } while (0)

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_HARDWARE_LOG_H
