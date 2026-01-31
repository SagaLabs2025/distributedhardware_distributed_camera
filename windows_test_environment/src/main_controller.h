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

#ifndef MAIN_CONTROLLER_H
#define MAIN_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QLibrary>
#include <memory>
#include "distributed_camera_source.h"
#include "distributed_camera_sink.h"

// 前向声明
class AutomationTestEngine;
class LogRedirector;

/**
 * 主控制器
 * 负责协调Source和Sink的工作流程
 */
class MainController : public QObject, public ISourceCallback, public ISinkCallback {
    Q_OBJECT

public:
    explicit MainController(QObject* parent = nullptr);
    ~MainController();

    // 加载Sink和Source动态库
    bool LoadLibraries(const QString& sinkPath, const QString& sourcePath);

    // 创建服务实例
    bool CreateServices();

    // 【对外接口】启动测试 - UI按钮点击触发
    Q_INVOKABLE void StartDistributedCameraTest();

    // 【对外接口】停止测试
    Q_INVOKABLE void StopDistributedCameraTest();

    // 运行自动化测试
    Q_INVOKABLE QString RunAutomatedTests();

    // ISourceCallback 实现
    void OnSourceError(int32_t errorCode, const std::string& errorMsg) override;
    void OnSourceStateChanged(const std::string& state) override;
    void OnDecodedFrameAvailable(const uint8_t* yuvData, int width, int height) override;

    // ISinkCallback 实现
    void OnSinkError(int32_t errorCode, const std::string& errorMsg) override;

    // 获取库版本信息
    Q_INVOKABLE QString GetSinkVersion() const;
    Q_INVOKABLE QString GetSourceVersion() const;

    // 获取日志
    Q_INVOKABLE QStringList GetCapturedLogs() const;
    Q_INVOKABLE void ClearLogs();

signals:
    // 源状态变化信号
    void SourceStateChanged(const QString& state);

    // 视频帧就绪信号
    void VideoFrameReady(const QByteArray& yuvData, int width, int height);

    // 错误信号
    void ErrorOccurred(const QString& error);

    // 测试完成信号
    void TestCompleted(const QString& result);

    // 日志更新信号
    void LogUpdated(const QString& log);

private:
    void InitializeLogRedirector();
    void ProcessDecodedFrame(const uint8_t* yuvData, int width, int height);

    QLibrary sinkLib_;
    QLibrary sourceLib_;

    IDistributedCameraSink* sinkService_;
    IDistributedCameraSource* sourceService_;

    std::unique_ptr<AutomationTestEngine> testEngine_;

    QString sinkPath_;
    QString sourcePath_;
    bool initialized_;
};

#endif // MAIN_CONTROLLER_H
