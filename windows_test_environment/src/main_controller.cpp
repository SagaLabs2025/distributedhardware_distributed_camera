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

#include "main_controller.h"
#include "log_redirector.h"
#include "automation_test_engine.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>

MainController::MainController(QObject* parent)
    : QObject(parent),
      sinkService_(nullptr),
      sourceService_(nullptr),
      initialized_(false) {

    InitializeLogRedirector();
}

MainController::~MainController() {
    StopDistributedCameraTest();
}

void MainController::InitializeLogRedirector() {
    auto& logRedirector = LogRedirector::GetInstance();
    logRedirector.Initialize();

    // 设置日志回调以发送信号
    logRedirector.SetLogCallback([this](LogRedirector::LogLevel level,
                                              const std::string& tag,
                                              const std::string& message) {
        QString log = QString("[%1] %2").arg(QString::fromStdString(tag))
                                            .arg(QString::fromStdString(message));
        emit LogUpdated(log);
    });
}

bool MainController::LoadLibraries(const QString& sinkPath, const QString& sourcePath) {
    qDebug() << "[MainController] Loading libraries...";
    qDebug() << "[MainController] Sink path:" << sinkPath;
    qDebug() << "[MainController] Source path:" << sourcePath;

    sinkPath_ = sinkPath;
    sourcePath_ = sourcePath;

    // 加载Sink.dll
    sinkLib_.setFileName(sinkPath);
    if (!sinkLib_.load()) {
        QString error = QString("Failed to load Sink.dll: %1").arg(sinkLib_.errorString());
        qCritical() << error;
        emit ErrorOccurred(error);
        return false;
    }

    // 加载Source.dll
    sourceLib_.setFileName(sourcePath);
    if (!sourceLib_.load()) {
        QString error = QString("Failed to load Source.dll: %1").arg(sourceLib_.errorString());
        qCritical() << error;
        emit ErrorOccurred(error);
        sinkLib_.unload();
        return false;
    }

    qDebug() << "[MainController] Libraries loaded successfully";
    return true;
}

bool MainController::CreateServices() {
    qDebug() << "[MainController] Creating service instances...";

    // 获取工厂函数
    typedef void* (*CreateSinkFunc)();
    typedef void* (*CreateSourceFunc)();
    typedef const char* (*GetVersionFunc)();

    auto createSink = (CreateSinkFunc)sinkLib_.resolve("CreateSinkService");
    auto createSource = (CreateSourceFunc)sourceLib_.resolve("CreateSourceService");
    auto getSinkVersion = (GetVersionFunc)sinkLib_.resolve("GetSinkVersion");
    auto getSourceVersion = (GetVersionFunc)sourceLib_.resolve("GetSourceVersion");

    if (!createSink || !createSource) {
        QString error = "Failed to resolve factory functions";
        qCritical() << error;
        emit ErrorOccurred(error);
        return false;
    }

    // 创建服务实例
    sinkService_ = (IDistributedCameraSink*)createSink();
    sourceService_ = (IDistributedCameraSource*)createSource();

    if (!sinkService_ || !sourceService_) {
        QString error = "Failed to create service instances";
        qCritical() << error;
        emit ErrorOccurred(error);
        return false;
    }

    qDebug() << "[MainController] Service instances created";
    initialized_ = true;
    return true;
}

void MainController::StartDistributedCameraTest() {
    qDebug() << "[MainController] ========== Starting Distributed Camera Test ==========";

    if (!initialized_) {
        if (!CreateServices()) {
            emit ErrorOccurred("Services not initialized");
            return;
        }
    }

    // 1. 初始化Sink
    qDebug() << "[MainController] Step 1: Initializing Sink service...";
    std::string params = "{}";
    if (sinkService_->InitSink(params, this) != 0) {
        emit ErrorOccurred("Failed to init Sink");
        return;
    }

    // 2. 初始化Source
    qDebug() << "[MainController] Step 2: Initializing Source service...";
    if (sourceService_->InitSource(params, this) != 0) {
        emit ErrorOccurred("Failed to init Source");
        return;
    }

    // 3. 注册分布式硬件
    qDebug() << "[MainController] Step 3: Registering distributed hardware...";
    if (sourceService_->RegisterDistributedHardware("LOCAL_SINK", "CAMERA_001") != 0) {
        emit ErrorOccurred("Failed to register hardware");
        return;
    }

    // 4. 【关键调用】启动捕获 - 这会触发完整的HDF回调流程
    qDebug() << "[MainController] Step 4: Starting capture (triggers HDF callbacks)...";
    if (sourceService_->StartCapture() != 0) {
        emit ErrorOccurred("Failed to start capture");
        return;
    }

    qDebug() << "[MainController] ========== Test Started Successfully ==========";
}

void MainController::StopDistributedCameraTest() {
    qDebug() << "[MainController] Stopping test...";

    if (sourceService_) {
        sourceService_->StopCapture();
        sourceService_->UnregisterDistributedHardware("LOCAL_SINK", "CAMERA_001");
        sourceService_->ReleaseSource();
    }

    if (sinkService_) {
        sinkService_->ReleaseSink();
    }

    qDebug() << "[MainController] Test stopped";
}

QString MainController::RunAutomatedTests() {
    qDebug() << "[MainController] Running automated tests...";

    if (!testEngine_) {
        testEngine_ = std::make_unique<AutomationTestEngine>(this);
    }

    QString result = testEngine_->RunAllTests();
    emit TestCompleted(result);

    return result;
}

// ===== ISourceCallback 实现 =====

void MainController::OnSourceError(int32_t errorCode, const std::string& errorMsg) {
    QString error = QString("Source error [%1]: %2")
                        .arg(errorCode)
                        .arg(QString::fromStdString(errorMsg));
    qWarning() << error;
    emit ErrorOccurred(error);
}

void MainController::OnSourceStateChanged(const std::string& state) {
    QString stateStr = QString::fromStdString(state);
    qDebug() << "[MainController] Source state changed:" << stateStr;
    emit SourceStateChanged(stateStr);
}

void MainController::OnDecodedFrameAvailable(const uint8_t* yuvData, int width, int height) {
    qDebug() << "[MainController] Decoded frame available:" << width << "x" << height;

    // 转换为QByteArray
    int yuvSize = width * height * 3 / 2;
    QByteArray byteArray(reinterpret_cast<const char*>(yuvData), yuvSize);

    emit VideoFrameReady(byteArray, width, height);
}

// ===== ISinkCallback 实现 =====

void MainController::OnSinkError(int32_t errorCode, const std::string& errorMsg) {
    QString error = QString("Sink error [%1]: %2")
                        .arg(errorCode)
                        .arg(QString::fromStdString(errorMsg));
    qWarning() << error;
    emit ErrorOccurred(error);
}

// ===== 其他方法 =====

QString MainController::GetSinkVersion() const {
    if (sinkLib_.isLoaded()) {
        typedef const char* (*GetVersionFunc)();
        auto getVersion = (GetVersionFunc)const_cast<QLibrary*>(&sinkLib_)->resolve("GetSinkVersion");
        if (getVersion) {
            return QString(getVersion());
        }
    }
    return "Unknown";
}

QString MainController::GetSourceVersion() const {
    if (sourceLib_.isLoaded()) {
        typedef const char* (*GetVersionFunc)();
        auto getVersion = (GetVersionFunc)const_cast<QLibrary*>(&sourceLib_)->resolve("GetSourceVersion");
        if (getVersion) {
            return QString(getVersion());
        }
    }
    return "Unknown";
}

QStringList MainController::GetCapturedLogs() const {
    QStringList logs;
    auto& logRedirector = LogRedirector::GetInstance();
    auto capturedLogs = logRedirector.GetCapturedLogs();
    for (const auto& log : capturedLogs) {
        logs.append(QString::fromStdString(log));
    }
    return logs;
}

void MainController::ClearLogs() {
    auto& logRedirector = LogRedirector::GetInstance();
    logRedirector.ClearLogs();
}
