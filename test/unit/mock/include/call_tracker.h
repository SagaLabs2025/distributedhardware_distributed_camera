/*
 * Call Tracker - 调用跟踪工具
 * 用于跟踪测试用例是否真正执行了原有代码
 */
#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <algorithm>

namespace OHOS {
namespace DistributedHardware {

// 调用记录
struct CallRecord {
    std::string functionName;   // 函数名
    std::string className;      // 类名
    std::string details;        // 详细信息
    std::chrono::steady_clock::time_point timestamp;

    CallRecord(const std::string& func, const std::string& cls, const std::string& det = "")
        : functionName(func), className(cls), details(det)
        , timestamp(std::chrono::steady_clock::now()) {}
};

// 调用跟踪器（单例）
class CallTracker {
public:
    static CallTracker& GetInstance() {
        static CallTracker instance;
        return instance;
    }

    // 记录函数调用
    void RecordCall(const std::string& className, const std::string& functionName,
                    const std::string& details = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.emplace_back(functionName, className, details);

        // 实时打印到控制台
        PrintCallRecord(records_.back());
    }

    // 打印调用记录
    void PrintCallRecord(const CallRecord& record) {
        auto time = std::chrono::steady_clock::now();
        auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(time);
        auto duration = ms.time_since_epoch();
        auto timestamp = duration.count();

        std::cout << "[CALL_TRACE] " << timestamp << " | "
                  << std::setw(30) << std::left << record.className
                  << " :: "
                  << std::setw(25) << std::left << record.functionName;
        if (!record.details.empty()) {
            std::cout << " | " << record.details;
        }
        std::cout << std::endl;
    }

    // 打印所有调用记录（用于测试报告）
    void PrintAllRecords() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << "\n========================================" << std::endl;
        std::cout << "  Call Trace Report (Total: " << records_.size() << " calls)" << std::endl;
        std::cout << "========================================" << std::endl;

        for (const auto& record : records_) {
            std::cout << "  " << record.className << "::" << record.functionName;
            if (!record.details.empty()) {
                std::cout << " [" << record.details << "]";
            }
            std::cout << std::endl;
        }
        std::cout << "========================================\n" << std::endl;
    }

    // 清除所有记录
    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        records_.clear();
    }

    // 检查某个函数是否被调用过
    bool WasCalled(const std::string& className, const std::string& functionName) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& record : records_) {
            if (record.className == className && record.functionName == functionName) {
                return true;
            }
        }
        return false;
    }

    // 获取某个函数的调用次数
    int GetCallCount(const std::string& className, const std::string& functionName) {
        std::lock_guard<std::mutex> lock(mutex_);
        int count = 0;
        for (const auto& record : records_) {
            if (record.className == className && record.functionName == functionName) {
                count++;
            }
        }
        return count;
    }

    // 获取所有记录（用于测试验证）
    std::vector<CallRecord> GetRecords() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return records_;
    }

private:
    CallTracker() = default;
    ~CallTracker() = default;
    CallTracker(const CallTracker&) = delete;
    CallTracker& operator=(const CallTracker&) = delete;

    mutable std::mutex mutex_;
    std::vector<CallRecord> records_;
};

// 便捷宏定义
#define TRACK_CALL() \
    OHOS::DistributedHardware::CallTracker::GetInstance().RecordCall(__FUNCTION__, __CLASS_NAME__)

#define TRACK_CALL_WITH_DETAILS(details) \
    OHOS::DistributedHardware::CallTracker::GetInstance().RecordCall(__FUNCTION__, __CLASS_NAME__, details)

// 类名宏（在每个类的开头定义）
#define DEFINE_CLASS_NAME(name) \
    static constexpr const char* __CLASS_NAME__ = #name

#define GET_CLASS_NAME() __CLASS_NAME__

} // namespace DistributedHardware
} // namespace OHOS
