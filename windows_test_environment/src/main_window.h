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

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTimer>

#include "main_controller.h"

/**
 * 主窗口
 * 提供UI界面用于测试分布式相机
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void OnStartTest();
    void OnStopTest();
    void OnRunTests();
    void OnSourceStateChanged(const QString& state);
    void OnErrorOccurred(const QString& error);
    void OnTestCompleted(const QString& result);
    void OnLogUpdated(const QString& log);
    void OnUpdateStatus();

private:
    void SetupUI();
    void ConnectSignals();

    // UI组件
    QWidget* centralWidget_;
    QTextEdit* logTextEdit_;
    QPushButton* startButton_;
    QPushButton* stopButton_;
    QPushButton* testButton_;
    QLabel* statusLabel_;
    QLabel* versionLabel_;
    QProgressBar* progressBar_;

    // 控制器
    MainController* controller_;
};

#endif // MAIN_WINDOW_H
