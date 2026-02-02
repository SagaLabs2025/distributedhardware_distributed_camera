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

#ifndef LOGANALYZER_H
#define LOGANALYZER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QMap>
#include <QList>
#include <QDateTime>
#include <QTimer>
#include <memory>

struct LogEntry {
    QDateTime timestamp;
    QString level;
    QString source;
    QString message;
    bool isAlert;
};

struct LogAnalysisResult {
    int totalLines;
    int errorCount;
    int warningCount;
    int alertCount;
    QList<LogEntry> alerts;
    QMap<QString, int> sourceCounts;
    QMap<QString, int> levelCounts;
};

class LogAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit LogAnalyzer(QObject *parent = nullptr);
    ~LogAnalyzer() override;

    // 設置要監控的日誌文件路徑
    void setLogFilePath(const QString& filePath);

    // 添加關鍵詞告警規則
    void addAlertKeyword(const QString& keyword, const QString& description = "");

    // 移除關鍵詞告警規則
    void removeAlertKeyword(const QString& keyword);

    // 清除所有告警規則
    void clearAlertKeywords();

    // 開始實時監控
    void startMonitoring();

    // 停止監控
    void stopMonitoring();

    // 分析指定的日誌文件
    LogAnalysisResult analyzeLogFile(const QString& filePath);

    // 獲取最後分析結果
    const LogAnalysisResult& getLastAnalysisResult() const;

    // 檢查是否正在監控
    bool isMonitoring() const { return isMonitoring_; }

signals:
    void analysisCompleted(const LogAnalysisResult& result);
    void newAlertDetected(const LogEntry& alert);
    void monitoringStarted();
    void monitoringStopped();
    void errorOccurred(const QString& error);

private slots:
    void onTimerTimeout();

private:
    void parseLogLine(const QString& line, LogEntry& entry);
    bool shouldAlert(const LogEntry& entry);
    void emitAlertIfNecessary(const LogEntry& entry);

    QString logFilePath_;
    QFile logFile_;
    qint64 lastPosition_;
    QTimer* monitorTimer_;
    bool isMonitoring_;

    // 告警配置
    QMap<QString, QString> alertKeywords_; // keyword -> description

    // 分析結果
    LogAnalysisResult lastAnalysisResult_;

    // 日誌解析模式
    QRegularExpression logPattern_;
};

#endif // LOGANALYZER_H