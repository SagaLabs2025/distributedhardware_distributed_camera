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

#ifndef LOG_REDIRECTOR_H
#define LOG_REDIRECTOR_H

#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <functional>

/**
 * DHLOG日志重定向器
 * 将DHLOG输出重定向到UI和内存，用于验收原始代码的DHLOG输出
 */
class LogRedirector {
public:
    // 日志级别
    enum LogLevel {
        LOG_DEBUG = 0,
        LOG_INFO = 1,
        LOG_WARN = 2,
        LOG_ERROR = 3,
        LOG_FATAL = 4
    };

    // 日志回调函数类型
    using LogCallback = std::function<void(LogLevel level, const std::string& tag, const std::string& message)>;

    static LogRedirector& GetInstance();

    // 初始化重定向器
    void Initialize();

    // 设置日志回调
    void SetLogCallback(LogCallback callback);

    // 获取所有捕获的日志
    std::vector<std::string> GetCapturedLogs() const;

    // 清除捕获的日志
    void ClearLogs();

    // 检查是否包含指定日志
    bool Contains(const std::string& pattern) const;

    // 检查是否匹配正则表达式
    bool ContainsRegex(const std::string& pattern) const;

    // 获取日志数量
    size_t GetLogCount() const;

    // 获取所有日志连接成的字符串
    std::string GetJoinedLogs(const std::string& separator = "\n") const;

    // 开始捕获日志
    void StartCapture();

    // 停止捕获日志
    void StopCapture();

    // 重定向DHLOG输出
    void RedirectDHLOG(LogLevel level, const std::string& tag, const std::string& file,
                       int line, const std::string& message);

private:
    LogRedirector();
    ~LogRedirector();

    void AddLog(LogLevel level, const std::string& tag, const std::string& message);

    std::vector<std::string> capturedLogs_;
    std::mutex mutex_;
    LogCallback logCallback_;
    bool capturing_;
};

// ===== 全局日志重定向函数 =====

// 获取原始的DHLOG输出函数指针
extern "C" {
    typedef void (*OriginalDHLOGI)(const char* fmt, ...);
    typedef void (*OriginalDHLOGE)(const char* fmt, ...);
    typedef void (*OriginalDHLOGW)(const char* fmt, ...);
    typedef void (*OriginalDHLOGD)(const char* fmt, ...);
}

// 设置重定向
void InstallLogRedirector();

// 卸载重定向
void UninstallLogRedirector();

#endif // LOG_REDIRECTOR_H
