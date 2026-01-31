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

#include "log_redirector.h"

#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <regex>
#include <cstring>
#include <algorithm>

#ifdef _WIN32
#define vsnprintf vsnprintf_s
#endif

// ===== DHLOG宏定义 (必须在使用之前定义) =====

#define DHLOGI(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        LogRedirector::GetInstance().RedirectDHLOG(LogRedirector::LOG_INFO, "DHLOGI", __FILE__, __LINE__, buffer); \
    } while(0)

#define DHLOGE(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        LogRedirector::GetInstance().RedirectDHLOG(LogRedirector::LOG_ERROR, "DHLOGE", __FILE__, __LINE__, buffer); \
    } while(0)

#define DHLOGW(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        LogRedirector::GetInstance().RedirectDHLOG(LogRedirector::LOG_WARN, "DHLOGW", __FILE__, __LINE__, buffer); \
    } while(0)

#define DHLOGD(fmt, ...) \
    do { \
        char buffer[1024]; \
        snprintf(buffer, sizeof(buffer), fmt, ##__VA_ARGS__); \
        LogRedirector::GetInstance().RedirectDHLOG(LogRedirector::LOG_DEBUG, "DHLOGD", __FILE__, __LINE__, buffer); \
    } while(0)

// ===== LogRedirector实现 =====

LogRedirector& LogRedirector::GetInstance() {
    static LogRedirector instance;
    return instance;
}

LogRedirector::LogRedirector() : capturing_(false) {
}

LogRedirector::~LogRedirector() {
    StopCapture();
}

void LogRedirector::Initialize() {
    DHLOGI("[LOG_REDIRECTOR] Initialize");
    StartCapture();
}

void LogRedirector::SetLogCallback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    logCallback_ = callback;
}

std::vector<std::string> LogRedirector::GetCapturedLogs() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return capturedLogs_;
}

void LogRedirector::ClearLogs() {
    std::lock_guard<std::mutex> lock(mutex_);
    capturedLogs_.clear();
}

bool LogRedirector::Contains(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    for (const auto& log : capturedLogs_) {
        if (log.find(pattern) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool LogRedirector::ContainsRegex(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    try {
        std::regex re(pattern);
        for (const auto& log : capturedLogs_) {
            if (std::regex_search(log, re)) {
                return true;
            }
        }
    } catch (const std::regex_error&) {
        // 正则表达式无效，使用普通搜索
        return Contains(pattern);
    }
    return false;
}

size_t LogRedirector::GetLogCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return capturedLogs_.size();
}

std::string LogRedirector::GetJoinedLogs(const std::string& separator) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    std::ostringstream oss;
    for (size_t i = 0; i < capturedLogs_.size(); ++i) {
        if (i > 0) oss << separator;
        oss << capturedLogs_[i];
    }
    return oss.str();
}

void LogRedirector::StartCapture() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (capturing_) return;
    capturing_ = true;
    DHLOGI("[LOG_REDIRECTOR] Log capture started");
}

void LogRedirector::StopCapture() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!capturing_) return;
    capturing_ = false;
    DHLOGI("[LOG_REDIRECTOR] Log capture stopped");
}

void LogRedirector::RedirectDHLOG(LogLevel level, const std::string& tag,
                                  const std::string& file, int line,
                                  const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!capturing_) {
        // 未开始捕获，直接输出到控制台
        std::cout << "[" << tag << "] " << message << std::endl;
        return;
    }

    // 构造完整日志消息
    std::ostringstream oss;
    oss << "[" << tag << "] " << message;

    std::string fullLog = oss.str();

    // 添加到捕获列表
    capturedLogs_.push_back(fullLog);

    // 同时输出到控制台
    std::cout << fullLog << std::endl;

    // 触发回调
    if (logCallback_) {
        logCallback_(level, tag, message);
    }
}

void LogRedirector::AddLog(LogLevel level, const std::string& tag, const std::string& message) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "[" << tag << "] " << message;
    capturedLogs_.push_back(oss.str());
}

// ===== 全局日志捕获辅助类 =====

class LogCaptureHelper {
public:
    static LogCaptureHelper& GetInstance() {
        static LogCaptureHelper instance;
        return instance;
    }

    void StartCapture() { LogRedirector::GetInstance().StartCapture(); }
    void StopCapture() { LogRedirector::GetInstance().StopCapture(); }
    void Clear() { LogRedirector::GetInstance().ClearLogs(); }
    bool Contains(const std::string& pattern) const {
        return LogRedirector::GetInstance().Contains(pattern);
    }
    bool ContainsRegex(const std::string& pattern) const {
        return LogRedirector::GetInstance().ContainsRegex(pattern);
    }
    size_t GetLogCount() const { return LogRedirector::GetInstance().GetLogCount(); }
    std::string GetJoinedLogs(const std::string& separator = "\n") const {
        return LogRedirector::GetInstance().GetJoinedLogs(separator);
    }
};

// ===== 别名兼容 =====

namespace LogCaptureAlias {
    inline LogCaptureHelper& GetInstance() {
        return LogCaptureHelper::GetInstance();
    }
}
