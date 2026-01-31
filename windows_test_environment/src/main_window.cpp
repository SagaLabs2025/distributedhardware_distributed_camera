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

#include "main_window.h"
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), controller_(nullptr) {

    setWindowTitle("分布式相机测试环境");
    resize(1024, 768);

    controller_ = new MainController(this);

    SetupUI();
    ConnectSignals();

    // 加载动态库
    QString sinkPath = QCoreApplication::applicationDirPath() + "/Sink.dll";
    QString sourcePath = QCoreApplication::applicationDirPath() + "/Source.dll";

    if (controller_->LoadLibraries(sinkPath, sourcePath)) {
        if (controller_->CreateServices()) {
            versionLabel_->setText(QString("Sink: %1 | Source: %2")
                                     .arg(controller_->GetSinkVersion())
                                     .arg(controller_->GetSourceVersion()));
            statusLabel_->setText("就绪");
        } else {
            statusLabel_->setText("错误: 创建服务失败");
        }
    } else {
        statusLabel_->setText("错误: 加载库失败");
    }
}

MainWindow::~MainWindow() {
    delete controller_;
}

void MainWindow::SetupUI() {
    centralWidget_ = new QWidget(this);
    setCentralWidget(centralWidget_);

    auto* mainLayout = new QVBoxLayout(centralWidget_);

    // 顶部信息栏
    auto* topLayout = new QHBoxLayout();
    versionLabel_ = new QLabel("版本: 未知", this);
    statusLabel_ = new QLabel("状态: 初始化中...", this);
    topLayout->addWidget(versionLabel_);
    topLayout->addStretch();
    topLayout->addWidget(statusLabel_);
    mainLayout->addLayout(topLayout);

    // 进度条
    progressBar_ = new QProgressBar(this);
    progressBar_->setRange(0, 100);
    progressBar_->setValue(0);
    mainLayout->addWidget(progressBar_);

    // 日志输出区域
    logTextEdit_ = new QTextEdit(this);
    logTextEdit_->setReadOnly(true);
    logTextEdit_->setFont(QFont("Consolas", 9));
    mainLayout->addWidget(logTextEdit_, 1);

    // 控制按钮区域
    auto* buttonLayout = new QHBoxLayout();
    startButton_ = new QPushButton("启动测试", this);
    stopButton_ = new QPushButton("停止测试", this);
    testButton_ = new QPushButton("运行自动化测试", this);

    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    testButton_->setEnabled(true);

    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(stopButton_);
    buttonLayout->addWidget(testButton_);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // 初始日志
    logTextEdit_->append("===========================================\n");
    logTextEdit_->append("    分布式相机测试环境\n");
    logTextEdit_->append("===========================================\n");
    logTextEdit_->append("等待启动测试...\n");
}

void MainWindow::ConnectSignals() {
    connect(startButton_, &QPushButton::clicked, this, &MainWindow::OnStartTest);
    connect(stopButton_, &QPushButton::clicked, this, &MainWindow::OnStopTest);
    connect(testButton_, &QPushButton::clicked, this, &MainWindow::OnRunTests);

    connect(controller_, &MainController::SourceStateChanged,
            this, &MainWindow::OnSourceStateChanged);
    connect(controller_, &MainController::ErrorOccurred,
            this, &MainWindow::OnErrorOccurred);
    connect(controller_, &MainController::TestCompleted,
            this, &MainWindow::OnTestCompleted);
    connect(controller_, &MainController::LogUpdated,
            this, &MainWindow::OnLogUpdated);
}

void MainWindow::OnStartTest() {
    logTextEdit_->append("\n========== 启动测试 ==========\n");

    startButton_->setEnabled(false);
    stopButton_->setEnabled(true);
    testButton_->setEnabled(false);
    progressBar_->setValue(0);

    statusLabel_->setText("状态: 测试运行中...");
    controller_->StartDistributedCameraTest();
}

void MainWindow::OnStopTest() {
    logTextEdit_->append("\n========== 停止测试 ==========\n");

    controller_->StopDistributedCameraTest();

    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    testButton_->setEnabled(true);
    progressBar_->setValue(100);

    statusLabel_->setText("状态: 已停止");
}

void MainWindow::OnRunTests() {
    logTextEdit_->append("\n========== 运行自动化测试 ==========\n");

    startButton_->setEnabled(false);
    stopButton_->setEnabled(false);
    testButton_->setEnabled(false);
    progressBar_->setValue(0);

    statusLabel_->setText("状态: 自动化测试中...");

    QString result = controller_->RunAutomatedTests();
    logTextEdit_->append(result);
}

void MainWindow::OnSourceStateChanged(const QString& state) {
    logTextEdit_->append(QString("[STATE] %1").arg(state));

    if (state == "OPENED") {
        progressBar_->setValue(25);
    } else if (state == "CONFIGURED") {
        progressBar_->setValue(50);
    } else if (state == "CAPTURING") {
        progressBar_->setValue(75);
    } else if (state == "STOPPED") {
        progressBar_->setValue(100);
        startButton_->setEnabled(true);
        stopButton_->setEnabled(false);
        testButton_->setEnabled(true);
    }
}

void MainWindow::OnErrorOccurred(const QString& error) {
    logTextEdit_->append(QString("[ERROR] %1").arg(error));

    QMessageBox::critical(this, "错误", error);

    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    testButton_->setEnabled(true);
    statusLabel_->setText("状态: 错误");
}

void MainWindow::OnTestCompleted(const QString& result) {
    logTextEdit_->append("\n" + result);

    progressBar_->setValue(100);
    startButton_->setEnabled(true);
    stopButton_->setEnabled(false);
    testButton_->setEnabled(true);
    statusLabel_->setText("状态: 测试完成");
}

void MainWindow::OnLogUpdated(const QString& log) {
    logTextEdit_->append(log);
}

void MainWindow::OnUpdateStatus() {
    // 定时更新状态
}
