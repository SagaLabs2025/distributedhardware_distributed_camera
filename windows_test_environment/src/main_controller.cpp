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
#include "dh_log_callback.h"

#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <QThread>

MainController::MainController(QObject* parent)
    : QObject(parent),
      sinkService_(nullptr),
      sourceService_(nullptr),
      initialized_(false),
      frameCount_(0),
      targetFrameCount_(10) {  // 接收10帧后自动完成测试

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

    // 设置DLL日志回调 (将DLL日志重定向到UI)
    DH_SetGlobalCallbackPtr([](DHLogLevel level, const char* tag, const char* message) {
        auto& redirector = LogRedirector::GetInstance();
        // 转换日志级别
        LogRedirector::LogLevel logLevel;
        switch (level) {
            case DHLogLevel::DH_INFO:  logLevel = LogRedirector::LOG_INFO; break;
            case DHLogLevel::DH_WARN:  logLevel = LogRedirector::LOG_WARN; break;
            case DHLogLevel::DH_ERROR: logLevel = LogRedirector::LOG_ERROR; break;
            case DHLogLevel::DH_DEBUG: logLevel = LogRedirector::LOG_DEBUG; break;
            default: logLevel = LogRedirector::LOG_INFO; break;
        }
        redirector.RedirectDHLOG(logLevel, tag, "", 0, message);
    });

    qDebug() << "[MainController] Log redirector initialized (DLL logs enabled)";
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

    // 重置帧计数
    frameCount_ = 0;

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
    emit LogUpdated("[TEST] Step 3: Registering distributed hardware...");
    if (sourceService_->RegisterDistributedHardware("LOCAL_SINK", "CAMERA_001") != 0) {
        emit ErrorOccurred("Failed to register hardware");
        return;
    }

    // 4. 启动Source端接收服务器
    qDebug() << "[MainController] Step 4: Starting Source receiver...";
    emit LogUpdated("[TEST] Step 4: Starting Source receiver (Socket server on port 8888)...");
    if (sourceService_->StartCapture() != 0) {
        emit ErrorOccurred("Failed to start Source receiver");
        return;
    }

    // 等待服务器完全启动
    qDebug() << "[MainController] Waiting for server to be ready...";
    emit LogUpdated("[TEST] Waiting for server to be ready...");
    QThread::msleep(1000);  // 给服务器1秒时间完全启动

    // 5. 启动Sink端（模拟Source端发送SoftBus消息）
    qDebug() << "[MainController] Step 5: Starting Sink capture...";
    emit LogUpdated("[TEST] Step 5: Starting Sink capture (connecting to 127.0.0.1:8888)...");
    int sinkResult = sinkService_->StartCapture("CAMERA_001", 1920, 1080);
    if (sinkResult != 0) {
        emit ErrorOccurred(QString("Failed to start Sink capture (error code: %1)").arg(sinkResult));
        return;
    }

    qDebug() << "[MainController] ========== Test Started Successfully ==========";
    emit LogUpdated("[TEST] ========== Test Started Successfully ==========");
    emit LogUpdated("[TEST] Waiting for data frames...");
}

void MainController::StopDistributedCameraTest() {
    qDebug() << "[MainController] Stopping test...";

    if (sinkService_) {
        sinkService_->StopCapture("CAMERA_001");
        sinkService_->ReleaseSink();
    }

    if (sourceService_) {
        sourceService_->StopCapture();
        sourceService_->UnregisterDistributedHardware("LOCAL_SINK", "CAMERA_001");
        sourceService_->ReleaseSource();
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
    frameCount_++;
    qDebug() << "[MainController] Decoded frame available:" << width << "x" << height
             << "(Frame #" << frameCount_ << " of " << targetFrameCount_ << ")";

    // 转换为QByteArray
    int yuvSize = width * height * 3 / 2;
    QByteArray byteArray(reinterpret_cast<const char*>(yuvData), yuvSize);

    emit VideoFrameReady(byteArray, width, height);

    // 自动完成测试：接收到目标帧数后停止
    if (frameCount_ >= targetFrameCount_) {
        qDebug() << "[MainController] Target frame count reached, stopping test...";
        QTimer::singleShot(500, this, [this]() {
            StopDistributedCameraTest();
            QString result = QString("\n========== 测试完成 ==========\n"
                                    "接收帧数: %1\n"
                                    "测试状态: PASS\n"
                                    "================================\n")
                                .arg(frameCount_);
            emit TestCompleted(result);
        });
    }
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
