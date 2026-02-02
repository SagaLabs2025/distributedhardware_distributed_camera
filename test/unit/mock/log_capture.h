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

#ifndef OHOS_DISTRIBUTED_CAMERA_LOG_CAPTURE_H
#define OHOS_DISTRIBUTED_CAMERA_LOG_CAPTURE_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <mutex>
#include <cstring>

namespace OHOS {
namespace DistributedHardware {

/**
 * @brief 日志捕获类，用于捕获和验证DHLOG输出
 *
 * 该类提供单例模式，能够捕获所有通过DHLOG宏输出的日志，
 * 并提供验证接口用于测试日志内容。
 *
 * 使用示例：
 * @code
 * LogCapture::GetInstance().StartCapture();
 * // ... 执行测试代码 ...
 * LogCapture::GetInstance().StopCapture();
 *
 * // 验证日志
 * EXPECT_TRUE(LogCapture::GetInstance().Contains("InitSource SUCCESS"));
 * EXPECT_EQ(LogCapture::GetInstance().CountContains("OpenCamera"), 1);
 *
 * LogCapture::GetInstance().Clear();
 * @endcode
 */
class LogCapture {
public:
    /**
     * @brief 获取LogCapture单例实例
     * @return LogCapture实例的引用
     */
    static LogCapture& GetInstance();

    /**
     * @brief 开始捕获日志（替代原有输出到std::cout）
     *
     * 该方法会重定向std::cout的输出到内部缓冲区，
     * 所有通过DHLOG宏输出的日志都会被捕获。
     */
    void StartCapture();

    /**
     * @brief 停止捕获并恢复原有输出
     *
     * 停止捕获后，std::cout会恢复到原始输出流。
     * 已捕获的日志内容会被保留。
     */
    void StopCapture();

    /**
     * @brief 获取所有捕获的日志
     * @return 包含所有捕获日志的字符串向量，按捕获顺序排列
     */
    std::vector<std::string> GetLogs() const;

    /**
     * @brief 验证日志是否包含指定内容
     * @param pattern 要搜索的模式字符串
     * @return 如果至少有一条日志包含该模式，返回true；否则返回false
     */
    bool Contains(const std::string& pattern) const;

    /**
     * @brief 验证日志包含指定内容的次数
     * @param pattern 要搜索的模式字符串
     * @return 包含该模式的日志条数
     */
    int CountContains(const std::string& pattern) const;

    /**
     * @brief 清空日志缓存
     *
     * 清除所有已捕获的日志内容。
     * 如果当前正在捕获，不会影响后续的日志捕获。
     */
    void Clear();

    /**
     * @brief 获取最后一次匹配的日志
     * @param pattern 要搜索的模式字符串
     * @return 最后一条包含该模式的日志，如果没有找到则返回空字符串
     */
    std::string GetLastMatch(const std::string& pattern) const;

    /**
     * @brief 获取所有匹配的日志
     * @param pattern 要搜索的模式字符串
     * @return 包含该模式的所有日志，按时间顺序排列
     */
    std::vector<std::string> GetMatches(const std::string& pattern) const;

    /**
     * @brief 获取捕获的日志数量
     * @return 当前已捕获的日志条数
     */
    size_t GetLogCount() const;

    /**
     * @brief 检查当前是否正在捕获
     * @return 如果正在捕获返回true，否则返回false
     */
    bool IsCapturing() const;

    /**
     * @brief 获取所有日志的合并字符串
     * @return 所有日志连接成一个字符串，每条日志用换行符分隔
     */
    std::string GetJoinedLogs() const;

private:
    LogCapture();
    ~LogCapture();

    // 禁止拷贝和赋值
    LogCapture(const LogCapture&) = delete;
    LogCapture& operator=(const LogCapture&) = delete;

    /**
     * @brief 内部方法：捕获日志行
     * @param log 捕获到的日志内容
     */
    void CaptureLog(const std::string& log);

    /**
     * @brief 刷新当前缓冲区内容到日志列表
     */
    void FlushBuffer();

    /**
     * @brief 自定义std::streambuf，用于捕获std::cout的输出
     */
    class CaptureStreamBuf : public std::streambuf {
    public:
        explicit CaptureStreamBuf(LogCapture* capture);
        ~CaptureStreamBuf() override;

    protected:
        int_type overflow(int_type ch) override;
        std::streamsize xsputn(const char* s, std::streamsize count) override;
        int sync() override;

    private:
        LogCapture* capture_;
        std::string buffer_;
        static const size_t BUFFER_SIZE = 4096;

        void FlushToCapture();
    };

    // 原始std::cout的streambuf指针
    std::streambuf* originalCoutBuf_;

    // 自定义捕获streambuf
    std::unique_ptr<CaptureStreamBuf> captureStreamBuf_;

    // 捕获的日志列表
    std::vector<std::string> capturedLogs_;

    // 当前正在构建的日志行
    std::string currentLine_;

    // 互斥锁，保证线程安全
    mutable std::mutex mutex_;

    // 是否正在捕获
    bool isCapturing_;

    // 友元类，允许CaptureStreamBuf访问私有成员
    friend class CaptureStreamBuf;
};

} // namespace DistributedHardware
} // namespace OHOS

#endif // OHOS_DISTRIBUTED_CAMERA_LOG_CAPTURE_H
