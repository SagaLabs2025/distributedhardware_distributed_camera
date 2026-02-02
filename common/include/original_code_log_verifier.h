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

#ifndef ORIGINAL_CODE_LOG_VERIFIER_H
#define ORIGINAL_CODE_LOG_VERIFIER_H

#include <string>
#include <vector>
#include <sstream>
#include <regex>

/**
 * 原始代码日志验证工具
 * 用于验证是否执行到了原始代码流程中的DHLOG输出
 *
 * 核心原则：验收的核心是验证是否执行到了原有代码流程中的DHLOG输出
 * 例如原始代码 dcamera_sink_data_process.cpp:108:
 * DHLOGI("StartCapture %{public}s success", GetAnonyString(dhId_).c_str());
 */
class OriginalCodeLogVerifier {
public:
    /**
     * 验证完整流程是否执行到了原始代码
     * @return true 如果所有关键日志都被验证到
     */
    static bool VerifyCompleteWorkflow();

    /**
     * 验证 HDF Mock 回调流程
     * @return true 如果所有 HDF Mock 日志都被验证到
     */
    static bool VerifyHDFCallbackWorkflow();

    /**
     * 验证 Source 端原始代码 DHLOG
     * @return true 如果 Source 端原始代码日志都被验证到
     */
    static bool VerifySourceOriginalCode();

    /**
     * 验证 Sink 端原始代码 DHLOG
     * @return true 如果 Sink 端原始代码日志都被验证到
     */
    static bool VerifySinkOriginalCode();

    /**
     * 生成验证报告
     * @return 包含验证结果的详细报告字符串
     */
    static std::string GenerateVerificationReport();

    /**
     * 统计关键日志点数量
     * @return 关键日志在所有日志中出现的次数
     */
    static int CountKeyLogs(const std::string& allLogs);

private:
    // 关键日志模式列表
    static const std::vector<std::string> HDF_MOCK_LOGS;
    static const std::vector<std::string> SOURCE_ORIGINAL_LOGS;
    static const std::vector<std::string> SINK_ORIGINAL_LOGS;
    static const std::vector<std::string> KEY_LOG_PATTERNS;
};

// ===== 内联实现 =====

inline bool OriginalCodeLogVerifier::VerifyHDFCallbackWorkflow() {
    for (const auto& log : HDF_MOCK_LOGS) {
        if (!LogCapture::GetInstance().Contains(log)) {
            printf("[VERIFY] Missing HDF Mock log: %s\n", log.c_str());
            return false;
        }
    }
    return true;
}

inline bool OriginalCodeLogVerifier::VerifySourceOriginalCode() {
    for (const auto& log : SOURCE_ORIGINAL_LOGS) {
        if (!LogCapture::GetInstance().Contains(log)) {
            printf("[VERIFY] Missing Source original log: %s\n", log.c_str());
            return false;
        }
    }
    return true;
}

inline bool OriginalCodeLogVerifier::VerifySinkOriginalCode() {
    for (const auto& log : SINK_ORIGINAL_LOGS) {
        if (!LogCapture::GetInstance().ContainsRegex(log)) {
            printf("[VERIFY] Missing Sink original log pattern: %s\n", log.c_str());
            return false;
        }
    }
    return true;
}

inline bool OriginalCodeLogVerifier::VerifyCompleteWorkflow() {
    std::ostringstream report;
    bool allPassed = true;

    report << "=== 原始代码DHLOG验证报告 ===\n\n";

    // 1. 验证 HDF Mock 回调流程
    report << "## HDF Mock 回调验证\n";
    for (const auto& log : HDF_MOCK_LOGS) {
        bool found = LogCapture::GetInstance().Contains(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    // 2. 验证 Source 端原始代码
    report << "## Source端原始代码验证\n";
    for (const auto& log : SOURCE_ORIGINAL_LOGS) {
        bool found = LogCapture::GetInstance().Contains(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    // 3. 验证 Sink 端原始代码
    report << "## Sink端原始代码验证\n";
    for (const auto& log : SINK_ORIGINAL_LOGS) {
        bool found = LogCapture::GetInstance().ContainsRegex(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    // 4. 统计信息
    report << "## 统计信息\n";
    std::string allLogs = LogCapture::GetInstance().GetJoinedLogs();
    report << "  总DHLOG数: " << LogCapture::GetInstance().GetLogCount() << "\n";
    report << "  关键日志点: " << CountKeyLogs(allLogs) << "\n";
    report << "  验证结果: " << (allPassed ? "✓ PASS" : "✗ FAIL") << "\n";

    printf("%s\n", report.str().c_str());
    return allPassed;
}

inline std::string OriginalCodeLogVerifier::GenerateVerificationReport() {
    std::ostringstream report;
    bool allPassed = true;

    report << "=== 原始代码DHLOG验证报告 ===\n\n";

    report << "## HDF Mock 回调验证\n";
    for (const auto& log : HDF_MOCK_LOGS) {
        bool found = LogCapture::GetInstance().Contains(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    report << "## Source端原始代码验证\n";
    for (const auto& log : SOURCE_ORIGINAL_LOGS) {
        bool found = LogCapture::GetInstance().Contains(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    report << "## Sink端原始代码验证\n";
    for (const auto& log : SINK_ORIGINAL_LOGS) {
        bool found = LogCapture::GetInstance().ContainsRegex(log);
        report << "  " << (found ? "✓" : "✗") << " " << log << "\n";
        if (!found) allPassed = false;
    }
    report << "\n";

    report << "## 统计信息\n";
    std::string allLogs = LogCapture::GetInstance().GetJoinedLogs();
    report << "  总DHLOG数: " << LogCapture::GetInstance().GetLogCount() << "\n";
    report << "  关键日志点: " << CountKeyLogs(allLogs) << "\n";
    report << "  验证结果: " << (allPassed ? "✓ PASS" : "✗ FAIL") << "\n";

    return report.str();
}

inline int OriginalCodeLogVerifier::CountKeyLogs(const std::string& allLogs) {
    int count = 0;
    for (const auto& key : KEY_LOG_PATTERNS) {
        size_t pos = 0;
        while ((pos = allLogs.find(key, pos)) != std::string::npos) {
            count++;
            pos += key.length();
        }
    }
    return count;
}

// ===== 静态常量定义 =====

inline const std::vector<std::string> OriginalCodeLogVerifier::HDF_MOCK_LOGS = {
    "[HDF_MOCK] OpenSession called",
    "[HDF_MOCK] OpenSession End",
    "[HDF_MOCK] ConfigureStreams called",
    "[HDF_MOCK] StartCapture called",
    "[HDF_MOCK] StartCapture success"
};

inline const std::vector<std::string> OriginalCodeLogVerifier::SOURCE_ORIGINAL_LOGS = {
    "DCameraSourceDataProcess StartCapture",  // 对应 dcamera_source_data_process.cpp:150
    "DCameraSourceDataProcess FeedStream"     // 对应 dcamera_source_data_process.cpp:41
};

inline const std::vector<std::string> OriginalCodeLogVerifier::SINK_ORIGINAL_LOGS = {
    "StartCapture dhId:",        // 对应 dcamera_sink_data_process.cpp:72
    "StartCapture.*success"      // 对应 dcamera_sink_data_process.cpp:108
};

inline const std::vector<std::string> OriginalCodeLogVerifier::KEY_LOG_PATTERNS = {
    "StartCapture",
    "OpenSession",
    "ConfigureStreams",
    "[HDF_MOCK]",
    "DCameraSourceDataProcess"
};

#endif // ORIGINAL_CODE_LOG_VERIFIER_H
