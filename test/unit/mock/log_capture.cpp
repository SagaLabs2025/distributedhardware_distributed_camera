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

#include "log_capture.h"
#include <iostream>
#include <algorithm>
#include <sstream>

namespace OHOS {
namespace DistributedHardware {

// ============================================================================
// LogCapture::CaptureStreamBuf 实现
// ============================================================================

LogCapture::CaptureStreamBuf::CaptureStreamBuf(LogCapture* capture)
    : capture_(capture), buffer_() {
    buffer_.reserve(BUFFER_SIZE);
}

LogCapture::CaptureStreamBuf::~CaptureStreamBuf() {
    FlushToCapture();
}

std::streambuf::int_type LogCapture::CaptureStreamBuf::overflow(int_type ch) {
    if (ch == traits_type::eof()) {
        return traits_type::eof();
    }

    // 将字符添加到缓冲区
    buffer_ += static_cast<char>(ch);

    // 如果遇到换行符，刷新到捕获器
    if (ch == '\n') {
        FlushToCapture();
    }

    return ch;
}

std::streamsize LogCapture::CaptureStreamBuf::xsputn(const char* s, std::streamsize count) {
    if (count <= 0) {
        return 0;
    }

    // 处理字符串中的每个换行符
    for (std::streamsize i = 0; i < count; ++i) {
        buffer_ += s[i];
        if (s[i] == '\n') {
            FlushToCapture();
        }
    }

    return count;
}

int LogCapture::CaptureStreamBuf::sync() {
    FlushToCapture();
    return 0;
}

void LogCapture::CaptureStreamBuf::FlushToCapture() {
    if (buffer_.empty()) {
        return;
    }

    if (capture_ != nullptr) {
        capture_->CaptureLog(buffer_);
    }

    buffer_.clear();
}

// ============================================================================
// LogCapture 实现
// ============================================================================

LogCapture::LogCapture()
    : originalCoutBuf_(nullptr),
      captureStreamBuf_(nullptr),
      capturedLogs_(),
      currentLine_(),
      mutex_(),
      isCapturing_(false) {
}

LogCapture::~LogCapture() {
    if (isCapturing_) {
        StopCapture();
    }
}

LogCapture& LogCapture::GetInstance() {
    static LogCapture instance;
    return instance;
}

void LogCapture::StartCapture() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (isCapturing_) {
        return;  // 已经在捕获中，无需重复开始
    }

    // 保存原始的std::cout streambuf
    originalCoutBuf_ = std::cout.rdbuf();

    // 创建自定义streambuf并替换
    captureStreamBuf_ = std::make_unique<CaptureStreamBuf>(this);
    std::cout.rdbuf(captureStreamBuf_.get());

    isCapturing_ = true;
}

void LogCapture::StopCapture() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isCapturing_) {
        return;  // 未在捕获中，无需停止
    }

    // 刷新任何剩余的缓冲内容
    if (captureStreamBuf_) {
        captureStreamBuf_->pubsync();
    }

    // 恢复原始的std::cout streambuf
    if (originalCoutBuf_ != nullptr) {
        std::cout.rdbuf(originalCoutBuf_);
        originalCoutBuf_ = nullptr;
    }

    // 释放自定义streambuf
    captureStreamBuf_.reset();

    isCapturing_ = false;
}

void LogCapture::CaptureLog(const std::string& log) {
    std::lock_guard<std::mutex> lock(mutex_);

    // 将日志内容添加到当前行
    currentLine_ += log;

    // 检查是否包含完整的行（以换行符结尾）
    size_t pos = 0;
    while ((pos = currentLine_.find('\n')) != std::string::npos) {
        std::string line = currentLine_.substr(0, pos);

        // 移除行尾的回车符（Windows平台）
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 只保存非空行
        if (!line.empty()) {
            capturedLogs_.push_back(line);
        }

        // 移除已处理的行
        currentLine_ = currentLine_.substr(pos + 1);
    }

    // 如果剩余内容过长（可能没有换行符），也保存
    if (currentLine_.length() > BUFFER_SIZE) {
        capturedLogs_.push_back(currentLine_);
        currentLine_.clear();
    }
}

void LogCapture::FlushBuffer() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!currentLine_.empty()) {
        capturedLogs_.push_back(currentLine_);
        currentLine_.clear();
    }
}

std::vector<std::string> LogCapture::GetLogs() const {
    std::lock_guard<std::mutex> lock(mutex_);

    // 如果有未刷新的缓冲区内容，先刷新
    if (!currentLine_.empty()) {
        const_cast<LogCapture*>(this)->FlushBuffer();
    }

    return capturedLogs_;
}

bool LogCapture::Contains(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // 检查已捕获的日志
    for (const auto& log : capturedLogs_) {
        if (log.find(pattern) != std::string::npos) {
            return true;
        }
    }

    // 检查当前正在构建的行
    if (!currentLine_.empty() && currentLine_.find(pattern) != std::string::npos) {
        return true;
    }

    return false;
}

int LogCapture::CountContains(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(mutex_);

    int count = 0;

    // 统计已捕获日志中的匹配数
    for (const auto& log : capturedLogs_) {
        if (log.find(pattern) != std::string::npos) {
            ++count;
        }
    }

    // 检查当前正在构建的行
    if (!currentLine_.empty() && currentLine_.find(pattern) != std::string::npos) {
        ++count;
    }

    return count;
}

void LogCapture::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);

    capturedLogs_.clear();
    currentLine_.clear();
}

std::string LogCapture::GetLastMatch(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(mutex_);

    // 从后往前查找已捕获的日志
    for (auto it = capturedLogs_.rbegin(); it != capturedLogs_.rend(); ++it) {
        if (it->find(pattern) != std::string::npos) {
            return *it;
        }
    }

    // 检查当前正在构建的行
    if (!currentLine_.empty() && currentLine_.find(pattern) != std::string::npos) {
        return currentLine_;
    }

    return "";
}

std::vector<std::string> LogCapture::GetMatches(const std::string& pattern) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> matches;
    matches.reserve(capturedLogs_.size());

    // 查找所有匹配的日志
    for (const auto& log : capturedLogs_) {
        if (log.find(pattern) != std::string::npos) {
            matches.push_back(log);
        }
    }

    // 检查当前正在构建的行
    if (!currentLine_.empty() && currentLine_.find(pattern) != std::string::npos) {
        matches.push_back(currentLine_);
    }

    return matches;
}

size_t LogCapture::GetLogCount() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t count = capturedLogs_.size();

    // 如果有未刷新的内容，也计入总数
    if (!currentLine_.empty()) {
        ++count;
    }

    return count;
}

bool LogCapture::IsCapturing() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return isCapturing_;
}

std::string LogCapture::GetJoinedLogs() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::ostringstream oss;

    bool first = true;
    for (const auto& log : capturedLogs_) {
        if (!first) {
            oss << "\n";
        }
        oss << log;
        first = false;
    }

    if (!currentLine_.empty()) {
        if (!first) {
            oss << "\n";
        }
        oss << currentLine_;
    }

    return oss.str();
}

} // namespace DistributedHardware
} // namespace OHOS
