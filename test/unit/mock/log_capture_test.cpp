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
#include "include/distributed_hardware_log.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

using namespace OHOS::DistributedHardware;

/**
 * @brief LogCapture功能测试示例
 *
 * 该文件展示了如何使用LogCapture来捕获和验证日志输出。
 */

// 测试辅助宏
#define TEST_ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            std::cerr << "ASSERT FAILED: " << #condition << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            std::cerr << "ASSERT FAILED: " << #actual << " == " << #expected \
                      << " (got " << (actual) << ", expected " << (expected) \
                      << ") at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

// 模拟测试函数
void SimulateTestOperation() {
    DHLOGI("InitSource SUCCESS");
    DHLOGI("OpenCamera cameraId=0");
    DHLOGW("OpenCamera warning: low memory");
    DHLOGE("OpenCamera failed: timeout");
    DHLOGD("Camera state changed: OPEN -> IDLE");
}

bool TestBasicCapture() {
    std::cout << "=== Test: Basic Capture ===" << std::endl;

    auto& capture = LogCapture::GetInstance();

    // 清空之前的日志
    capture.Clear();

    // 开始捕获
    capture.StartCapture();

    // 输出一些日志
    DHLOGI("Test log message 1");
    DHLOGE("Error message");
    DHLOGW("Warning message");

    // 停止捕获
    capture.StopCapture();

    // 验证日志内容
    TEST_ASSERT_TRUE(capture.Contains("Test log message 1"));
    TEST_ASSERT_TRUE(capture.Contains("Error message"));
    TEST_ASSERT_TRUE(capture.Contains("Warning message"));

    std::cout << "Basic capture test PASSED" << std::endl;
    return true;
}

bool TestContains() {
    std::cout << "=== Test: Contains ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    SimulateTestOperation();

    capture.StopCapture();

    // 测试Contains方法
    TEST_ASSERT_TRUE(capture.Contains("InitSource SUCCESS"));
    TEST_ASSERT_TRUE(capture.Contains("OpenCamera"));
    TEST_ASSERT_TRUE(capture.Contains("timeout"));
    TEST_ASSERT_FALSE(capture.Contains("NonExistentMessage"));

    std::cout << "Contains test PASSED" << std::endl;
    return true;
}

bool TestCountContains() {
    std::cout << "=== Test: CountContains ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    // 输出多条包含"OpenCamera"的日志
    DHLOGI("OpenCamera cameraId=0");
    DHLOGW("OpenCamera warning: low memory");
    DHLOGE("OpenCamera failed: timeout");
    DHLOGI("OpenCamera cameraId=1");

    capture.StopCapture();

    // 测试CountContains方法
    TEST_ASSERT_EQ(capture.CountContains("OpenCamera"), 4);
    TEST_ASSERT_EQ(capture.CountContains("InitSource"), 0);
    TEST_ASSERT_EQ(capture.CountContains("ERROR"), 2);  // DHLOGE和一条ERROR标签

    std::cout << "CountContains test PASSED" << std::endl;
    return true;
}

bool TestGetLastMatch() {
    std::cout << "=== Test: GetLastMatch ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    DHLOGI("First message with keyword");
    DHLOGI("Second message with keyword");
    DHLOGI("Third message with keyword");

    capture.StopCapture();

    // 测试GetLastMatch方法
    std::string lastMatch = capture.GetLastMatch("keyword");
    TEST_ASSERT_TRUE(lastMatch.find("Third") != std::string::npos);

    std::cout << "GetLastMatch test PASSED" << std::endl;
    return true;
}

bool TestGetMatches() {
    std::cout << "=== Test: GetMatches ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    DHLOGI("Error: code 100");
    DHLOGI("Warning: low memory");
    DHLOGE("Error: code 200");
    DHLOGW("Error: code 300");

    capture.StopCapture();

    // 测试GetMatches方法
    auto matches = capture.GetMatches("Error:");
    TEST_ASSERT_EQ(matches.size(), 3);

    std::cout << "GetMatches test PASSED" << std::endl;
    return true;
}

bool TestClear() {
    std::cout << "=== Test: Clear ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    DHLOGI("Message before clear");
    TEST_ASSERT_EQ(capture.GetLogCount(), 1);

    capture.Clear();
    TEST_ASSERT_EQ(capture.GetLogCount(), 0);

    DHLOGI("Message after clear");
    capture.StopCapture();

    TEST_ASSERT_EQ(capture.GetLogCount(), 1);
    TEST_ASSERT_TRUE(capture.Contains("Message after clear"));
    TEST_ASSERT_FALSE(capture.Contains("Message before clear"));

    std::cout << "Clear test PASSED" << std::endl;
    return true;
}

bool TestMultipleStartStop() {
    std::cout << "=== Test: Multiple Start/Stop ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();

    // 第一次捕获
    capture.StartCapture();
    DHLOGI("First capture");
    capture.StopCapture();

    // 第二次捕获
    capture.StartCapture();
    DHLOGI("Second capture");
    capture.StopCapture();

    TEST_ASSERT_TRUE(capture.Contains("First capture"));
    TEST_ASSERT_TRUE(capture.Contains("Second capture"));

    std::cout << "Multiple Start/Stop test PASSED" << std::endl;
    return true;
}

bool TestGetLogs() {
    std::cout << "=== Test: GetLogs ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    DHLOGI("Line 1");
    DHLOGI("Line 2");
    DHLOGI("Line 3");

    capture.StopCapture();

    auto logs = capture.GetLogs();
    TEST_ASSERT_EQ(logs.size(), 3);
    TEST_ASSERT_TRUE(logs[0].find("Line 1") != std::string::npos);
    TEST_ASSERT_TRUE(logs[1].find("Line 2") != std::string::npos);
    TEST_ASSERT_TRUE(logs[2].find("Line 3") != std::string::npos);

    std::cout << "GetLogs test PASSED" << std::endl;
    return true;
}

bool TestIsCapturing() {
    std::cout << "=== Test: IsCapturing ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();

    TEST_ASSERT_FALSE(capture.IsCapturing());

    capture.StartCapture();
    TEST_ASSERT_TRUE(capture.IsCapturing());

    capture.StopCapture();
    TEST_ASSERT_FALSE(capture.IsCapturing());

    std::cout << "IsCapturing test PASSED" << std::endl;
    return true;
}

bool TestGetJoinedLogs() {
    std::cout << "=== Test: GetJoinedLogs ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    DHLOGI("First line");
    DHLOGI("Second line");

    capture.StopCapture();

    std::string joined = capture.GetJoinedLogs();
    TEST_ASSERT_TRUE(joined.find("First line") != std::string::npos);
    TEST_ASSERT_TRUE(joined.find("Second line") != std::string::npos);

    std::cout << "GetJoinedLogs test PASSED" << std::endl;
    return true;
}

bool TestLongLogLines() {
    std::cout << "=== Test: Long Log Lines ===" << std::endl;

    auto& capture = LogCapture::GetInstance();
    capture.Clear();
    capture.StartCapture();

    // 生成一个很长的日志
    std::string longLog(5000, 'A');
    DHLOGI("Long log: %s", longLog.c_str());

    capture.StopCapture();

    TEST_ASSERT_TRUE(capture.GetLogCount() > 0);

    std::cout << "Long Log Lines test PASSED" << std::endl;
    return true;
}

void PrintUsage() {
    std::cout << "\n===== LogCapture Usage Example =====\n" << std::endl;
    std::cout << "1. Basic Usage:\n";
    std::cout << "   LogCapture::GetInstance().StartCapture();\n";
    std::cout << "   // ... 执行测试代码 ...\n";
    std::cout << "   LogCapture::GetInstance().StopCapture();\n\n";

    std::cout << "2. Verify Log Content:\n";
    std::cout << "   EXPECT_TRUE(LogCapture::GetInstance().Contains(\"InitSource SUCCESS\"));\n";
    std::cout << "   EXPECT_EQ(LogCapture::GetInstance().CountContains(\"OpenCamera\"), 1);\n\n";

    std::cout << "3. Get Matching Logs:\n";
    std::cout << "   std::string last = LogCapture::GetInstance().GetLastMatch(\"pattern\");\n";
    std::cout << "   auto matches = LogCapture::GetInstance().GetMatches(\"pattern\");\n\n";

    std::cout << "4. Clear Logs:\n";
    std::cout << "   LogCapture::GetInstance().Clear();\n\n";

    std::cout << "5. Get All Logs:\n";
    std::cout << "   auto logs = LogCapture::GetInstance().GetLogs();\n";
    std::cout << "   std::string all = LogCapture::GetInstance().GetJoinedLogs();\n\n";
}

int main() {
    std::cout << "\n========================================" << std::endl;
    std::cout << "   LogCapture Functionality Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    int passed = 0;
    int total = 0;

    // 运行所有测试
    total++;
    if (TestBasicCapture()) passed++;

    total++;
    if (TestContains()) passed++;

    total++;
    if (TestCountContains()) passed++;

    total++;
    if (TestGetLastMatch()) passed++;

    total++;
    if (TestGetMatches()) passed++;

    total++;
    if (TestClear()) passed++;

    total++;
    if (TestMultipleStartStop()) passed++;

    total++;
    if (TestGetLogs()) passed++;

    total++;
    if (TestIsCapturing()) passed++;

    total++;
    if (TestGetJoinedLogs()) passed++;

    total++;
    if (TestLongLogLines()) passed++;

    // 打印测试结果
    std::cout << "\n========================================" << std::endl;
    std::cout << "   Test Results: " << passed << "/" << total << " passed" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // 打印使用示例
    PrintUsage();

    return (passed == total) ? 0 : 1;
}
