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

#ifndef AUTOMATION_TEST_ENGINE_H
#define AUTOMATION_TEST_ENGINE_H

#include <QObject>
#include <QString>
#include <QVector>
#include <memory>

// 前向声明
class MainController;

/**
 * 测试结果
 */
struct TestResult {
    QString name;
    bool passed;
    QString message;
    qint64 durationMs;
};

/**
 * 自动化测试引擎
 * 负责运行自动化测试用例并验证原始代码的DHLOG输出
 */
class AutomationTestEngine : public QObject {
    Q_OBJECT

public:
    explicit AutomationTestEngine(MainController* controller);
    ~AutomationTestEngine();

    // 运行所有测试
    QString RunAllTests();

    // 运行单个测试
    TestResult RunTest(const QString& testName);

    // 获取测试结果
    QVector<TestResult> GetTestResults() const { return testResults_; }

    // 生成测试报告
    QString GenerateReport() const;

private:
    // 测试用例
    TestResult TestCompleteWorkflow();
    TestResult TestHDFCallbackSequence();
    TestResult TestSinkCapture();
    TestResult TestSourceDecoding();

    // 辅助方法
    bool VerifyLogExists(const QString& pattern) const;
    bool VerifyLogPattern(const QString& regex) const;
    void WaitForLogs(int timeoutMs = 5000);

    MainController* controller_;
    QVector<TestResult> testResults_;
};

#endif // AUTOMATION_TEST_ENGINE_H
