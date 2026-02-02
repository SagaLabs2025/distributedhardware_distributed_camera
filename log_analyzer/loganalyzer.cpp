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

#include "loganalyzer.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>

LogAnalyzer::LogAnalyzer(QObject *parent)
    : QObject(parent)
    , monitorTimer_(nullptr)
    , isMonitoring_(false)
    , lastPosition_(0)
{
    monitorTimer_ = new QTimer(this);
    connect(monitorTimer_, &QTimer::timeout, this, &LogAnalyzer::onTimerTimeout);

    // 設置日誌解析正則表達式模式
    // 支持常見的日誌格式，如: [2026-01-29 10:30:45] ERROR source: message
    logPattern_.setPattern(R"(\[(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\]\s+(\w+)\s+([^:]+):\s+(.+))");
}

LogAnalyzer::~LogAnalyzer()
{
    if (monitorTimer_) {
        monitorTimer_->stop();
        delete monitorTimer_;
    }
}

void LogAnalyzer::setLogFilePath(const QString& filePath)
{
    logFilePath_ = filePath;
    lastPosition_ = 0;
}

void LogAnalyzer::addAlertKeyword(const QString& keyword, const QString& description)
{
    alertKeywords_[keyword.toLower()] = description;
}

void LogAnalyzer::removeAlertKeyword(const QString& keyword)
{
    alertKeywords_.remove(keyword.toLower());
}

void LogAnalyzer::clearAlertKeywords()
{
    alertKeywords_.clear();
}

void LogAnalyzer::startMonitoring()
{
    if (logFilePath_.isEmpty()) {
        emit errorOccurred("Log file path is not set");
        return;
    }

    if (!QFile::exists(logFilePath_)) {
        emit errorOccurred("Log file does not exist: " + logFilePath_);
        return;
    }

    // 打開文件並定位到末尾
    logFile_.setFileName(logFilePath_);
    if (!logFile_.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to open log file: " + logFilePath_);
        return;
    }

    // 定位到當前文件末尾
    lastPosition_ = logFile_.size();

    // 開始監控（每100ms檢查一次）
    monitorTimer_->start(100);
    isMonitoring_ = true;

    emit monitoringStarted();
}

void LogAnalyzer::stopMonitoring()
{
    if (monitorTimer_) {
        monitorTimer_->stop();
    }
    if (logFile_.isOpen()) {
        logFile_.close();
    }
    isMonitoring_ = false;
    emit monitoringStopped();
}

LogAnalysisResult LogAnalyzer::analyzeLogFile(const QString& filePath)
{
    LogAnalysisResult result;
    result.totalLines = 0;
    result.errorCount = 0;
    result.warningCount = 0;
    result.alertCount = 0;
    result.alerts.clear();
    result.sourceCounts.clear();
    result.levelCounts.clear();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit errorOccurred("Failed to open log file for analysis: " + filePath);
        return result;
    }

    QTextStream stream(&file);
    QString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        result.totalLines++;

        LogEntry entry;
        parseLogLine(line, entry);

        if (!entry.level.isEmpty()) {
            result.levelCounts[entry.level]++;
            if (entry.level == "ERROR") {
                result.errorCount++;
            } else if (entry.level == "WARNING" || entry.level == "WARN") {
                result.warningCount++;
            }

            if (!entry.source.isEmpty()) {
                result.sourceCounts[entry.source]++;
            }

            if (shouldAlert(entry)) {
                entry.isAlert = true;
                result.alerts.append(entry);
                result.alertCount++;
                emitAlertIfNecessary(entry);
            }
        }
    }

    file.close();
    lastAnalysisResult_ = result;
    emit analysisCompleted(result);
    return result;
}

const LogAnalysisResult& LogAnalyzer::getLastAnalysisResult() const
{
    return lastAnalysisResult_;
}

void LogAnalyzer::onTimerTimeout()
{
    if (!logFile_.isOpen() || logFilePath_.isEmpty()) {
        return;
    }

    // 檢查文件是否被截斷（例如logrotate）
    if (logFile_.size() < lastPosition_) {
        lastPosition_ = 0;
        logFile_.seek(0);
    }

    // 如果文件沒有增長，直接返回
    if (logFile_.size() <= lastPosition_) {
        return;
    }

    // 移動到上次讀取的位置
    if (!logFile_.seek(lastPosition_)) {
        emit errorOccurred("Failed to seek in log file");
        return;
    }

    QTextStream stream(&logFile_);
    QString line;
    while (!stream.atEnd()) {
        line = stream.readLine();
        lastPosition_ = logFile_.pos();

        LogEntry entry;
        parseLogLine(line, entry);

        if (!entry.level.isEmpty() && shouldAlert(entry)) {
            entry.isAlert = true;
            emit newAlertDetected(entry);
        }
    }
}

void LogAnalyzer::parseLogLine(const QString& line, LogEntry& entry)
{
    entry.timestamp = QDateTime();
    entry.level = "";
    entry.source = "";
    entry.message = "";
    entry.isAlert = false;

    QRegularExpressionMatch match = logPattern_.match(line);
    if (match.hasMatch()) {
        // 解析時間戳
        QString timestampStr = match.captured(1);
        entry.timestamp = QDateTime::fromString(timestampStr, "yyyy-MM-dd hh:mm:ss");

        // 解析日誌級別
        entry.level = match.captured(2).toUpper();

        // 解析來源
        entry.source = match.captured(3);

        // 解析消息
        entry.message = match.captured(4);
    } else {
        // 如果不匹配標準格式，嘗試簡單解析
        // 假設格式為: LEVEL source: message
        QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            QString potentialLevel = parts[0].toUpper();
            if (potentialLevel == "ERROR" || potentialLevel == "WARNING" ||
                potentialLevel == "WARN" || potentialLevel == "INFO" ||
                potentialLevel == "DEBUG") {
                entry.level = potentialLevel;
                entry.source = parts[1].remove(':');
                entry.message = line.mid(parts[0].length() + parts[1].length() + 2);
                entry.timestamp = QDateTime::currentDateTime();
            }
        }
    }
}

bool LogAnalyzer::shouldAlert(const LogEntry& entry)
{
    if (alertKeywords_.isEmpty()) {
        // 默認告警規則：錯誤和警告級別
        return (entry.level == "ERROR" || entry.level == "WARNING" || entry.level == "WARN");
    }

    QString lowerMessage = entry.message.toLower();
    for (const QString& keyword : alertKeywords_.keys()) {
        if (lowerMessage.contains(keyword)) {
            return true;
        }
    }

    return false;
}

void LogAnalyzer::emitAlertIfNecessary(const LogEntry& entry)
{
    // 在實時監控模式下，這個函數會被調用
    // 在批量分析模式下，告警已經在analyzeLogFile中處理
}