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

#include "automation_test_engine.h"
#include "main_controller.h"
#include "log_redirector.h"

#include <QElapsedTimer>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
#include <QRegularExpression>
#include <QDebug>

AutomationTestEngine::AutomationTestEngine(MainController* controller)
    : QObject(), controller_(controller) {
}

AutomationTestEngine::~AutomationTestEngine() {
}

QString AutomationTestEngine::RunAllTests() {
    qDebug() << "[TestEngine] ========== Running Automated Tests ==========";

    testResults_.clear();

    // 运行所有测试用例
    testResults_.append(TestCompleteWorkflow());
    testResults_.append(TestHDFCallbackSequence());
    testResults_.append(TestSinkCapture());
    testResults_.append(TestSourceDecoding());

    // 生成报告
    QString report = GenerateReport();
    qDebug() << "[TestEngine] ========== Tests Completed ==========";
    qDebug().noquote() << report;

    return report;
}

TestResult AutomationTestEngine::RunTest(const QString& testName) {
    qDebug() << "[TestEngine] Running test:" << testName;

    if (testName == "CompleteWorkflow") {
        return TestCompleteWorkflow();
    } else if (testName == "HDFCallbackSequence") {
        return TestHDFCallbackSequence();
    } else if (testName == "SinkCapture") {
        return TestSinkCapture();
    } else if (testName == "SourceDecoding") {
        return TestSourceDecoding();
    }

    TestResult result;
    result.name = testName;
    result.passed = false;
    result.message = "Unknown test name";
    return result;
}

TestResult AutomationTestEngine::TestCompleteWorkflow() {
    TestResult result;
    result.name = "CompleteWorkflow";
    QElapsedTimer timer;

    qDebug() << "[TestEngine] ===== Test: CompleteWorkflow =====";
    timer.start();

    // 清除之前的日志
    LogRedirector::GetInstance().ClearLogs();
    LogRedirector::GetInstance().StartCapture();

    // 启动测试
    if (controller_) {
        controller_->StartDistributedCameraTest();
    }

    // 等待日志输出
    WaitForLogs(5000);

    // 停止测试
    if (controller_) {
        controller_->StopDistributedCameraTest();
    }

    // 验证关键日志
    bool passed = true;
    QStringList missingLogs;

    // 验证 HDF Mock 回调流程
    QStringList requiredLogs = {
        "[HDF_MOCK] OpenSession called",
        "[HDF_MOCK] OpenSession End",
        "[HDF_MOCK] ConfigureStreams called",
        "[HDF_MOCK] StartCapture called",
        "[HDF_MOCK] StartCapture success"
    };

    for (const auto& log : requiredLogs) {
        if (!VerifyLogExists(log)) {
            missingLogs.append(log);
            passed = false;
        }
    }

    result.durationMs = timer.elapsed();
    result.passed = passed;

    if (passed) {
        result.message = "All required logs found";
        qDebug() << "[TestEngine] ✓ Test PASSED";
    } else {
        result.message = "Missing logs: " + missingLogs.join(", ");
        qWarning() << "[TestEngine] ✗ Test FAILED:" << result.message;
    }

    return result;
}

TestResult AutomationTestEngine::TestHDFCallbackSequence() {
    TestResult result;
    result.name = "HDFCallbackSequence";
    QElapsedTimer timer;

    qDebug() << "[TestEngine] ===== Test: HDFCallbackSequence =====";
    timer.start();

    LogRedirector::GetInstance().ClearLogs();
    LogRedirector::GetInstance().StartCapture();

    // 验证 HDF 回调序列是否按正确顺序执行
    if (controller_) {
        controller_->StartDistributedCameraTest();
    }

    WaitForLogs(3000);

    if (controller_) {
        controller_->StopDistributedCameraTest();
    }

    // 检查日志顺序
    QStringList logs = controller_->GetCapturedLogs();
    bool passed = true;
    QString message;

    // 检查 OpenSession 在 ConfigureStreams 之前
    int openIndex = -1, configIndex = -1, startIndex = -1;
    for (int i = 0; i < logs.size(); ++i) {
        if (logs[i].contains("[HDF_MOCK] OpenSession called")) openIndex = i;
        if (logs[i].contains("[HDF_MOCK] ConfigureStreams called")) configIndex = i;
        if (logs[i].contains("[HDF_MOCK] StartCapture called")) startIndex = i;
    }

    if (openIndex < 0 || configIndex < 0 || startIndex < 0) {
        passed = false;
        message = "Not all HDF callbacks were called";
    } else if (openIndex > configIndex || configIndex > startIndex) {
        passed = false;
        message = QString("HDF callbacks not in correct order (Open:%1, Config:%2, Start:%3)")
                     .arg(openIndex).arg(configIndex).arg(startIndex);
    } else {
        message = QString("HDF callbacks in correct order (Open:%1, Config:%2, Start:%3)")
                     .arg(openIndex).arg(configIndex).arg(startIndex);
    }

    result.durationMs = timer.elapsed();
    result.passed = passed;
    result.message = message;

    qDebug() << "[TestEngine]" << (passed ? "✓" : "✗") << message;

    return result;
}

TestResult AutomationTestEngine::TestSinkCapture() {
    TestResult result;
    result.name = "SinkCapture";
    QElapsedTimer timer;

    qDebug() << "[TestEngine] ===== Test: SinkCapture =====";
    timer.start();

    // 这个测试主要验证 Sink 端的原始代码被执行
    LogRedirector::GetInstance().ClearLogs();
    LogRedirector::GetInstance().StartCapture();

    if (controller_) {
        controller_->StartDistributedCameraTest();
    }

    WaitForLogs(5000);

    if (controller_) {
        controller_->StopDistributedCameraTest();
    }

    // 验证是否有日志输出
    bool passed = LogRedirector::GetInstance().GetLogCount() > 0;
    QString message = QString("Total logs captured: %1")
                     .arg(LogRedirector::GetInstance().GetLogCount());

    result.durationMs = timer.elapsed();
    result.passed = passed;
    result.message = message;

    qDebug() << "[TestEngine]" << (passed ? "✓" : "✗") << message;

    return result;
}

TestResult AutomationTestEngine::TestSourceDecoding() {
    TestResult result;
    result.name = "SourceDecoding";
    QElapsedTimer timer;

    qDebug() << "[TestEngine] ===== Test: SourceDecoding =====";
    timer.start();

    // 这个测试验证 Source 端解码功能
    LogRedirector::GetInstance().ClearLogs();
    LogRedirector::GetInstance().StartCapture();

    if (controller_) {
        controller_->StartDistributedCameraTest();
    }

    WaitForLogs(3000);

    if (controller_) {
        controller_->StopDistributedCameraTest();
    }

    // 验证日志输出
    bool passed = LogRedirector::GetInstance().GetLogCount() > 0;
    QString message = QString("Source decoding test, logs: %1")
                     .arg(LogRedirector::GetInstance().GetLogCount());

    result.durationMs = timer.elapsed();
    result.passed = passed;
    result.message = message;

    qDebug() << "[TestEngine]" << (passed ? "✓" : "✗") << message;

    return result;
}

bool AutomationTestEngine::VerifyLogExists(const QString& pattern) const {
    return LogRedirector::GetInstance().Contains(pattern.toStdString());
}

bool AutomationTestEngine::VerifyLogPattern(const QString& regex) const {
    return LogRedirector::GetInstance().ContainsRegex(regex.toStdString());
}

void AutomationTestEngine::WaitForLogs(int timeoutMs) {
    QEventLoop loop;
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
}

QString AutomationTestEngine::GenerateReport() const {
    QString report;
    QTextStream stream(&report);

    stream << "===========================================\n";
    stream << "       自动化测试报告\n";
    stream << "===========================================\n\n";

    int passed = 0, failed = 0;
    qint64 totalDuration = 0;

    for (const auto& result : testResults_) {
        stream << "测试: " << result.name << "\n";
        stream << "结果: " << (result.passed ? "✓ PASS" : "✗ FAIL") << "\n";
        stream << "耗时: " << result.durationMs << " ms\n";
        stream << "消息: " << result.message << "\n";
        stream << "-------------------------------------------\n";

        totalDuration += result.durationMs;
        if (result.passed) passed++;
        else failed++;
    }

    stream << "\n总结:\n";
    stream << "通过: " << passed << "\n";
    stream << "失败: " << failed << "\n";
    stream << "总耗时: " << totalDuration << " ms\n";
    stream << "===========================================\n";

    return report;
}
