/*
 * DLL 日志诊断工具 v2
 * 使用 DLL 导出的函数来设置回调
 */

#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

// 日志级别
enum class DHLogLevel {
    DH_INFO,
    DH_WARN,
    DH_ERROR,
    DH_DEBUG
};

// 日志接收器
class LogReceiver {
private:
    static int logCount_;
    static std::vector<std::string> capturedLogs_;

public:
    static void LogCallback(DHLogLevel level, const char* tag, const char* message) {
        logCount_++;
        std::string levelStr;
        switch (level) {
            case DHLogLevel::DH_INFO:  levelStr = "INFO"; break;
            case DHLogLevel::DH_WARN:  levelStr = "WARN"; break;
            case DHLogLevel::DH_ERROR: levelStr = "ERROR"; break;
            case DHLogLevel::DH_DEBUG: levelStr = "DEBUG"; break;
        }

        std::string log = std::string("[") + levelStr + "] " + tag + ": " + message;
        capturedLogs_.push_back(log);

        // 同时输出到控制台
        std::cout << log << std::endl;
    }

    static int GetLogCount() { return logCount_; }
    static const std::vector<std::string>& GetCapturedLogs() { return capturedLogs_; }
    static void Reset() {
        logCount_ = 0;
        capturedLogs_.clear();
    }
};

int LogReceiver::logCount_ = 0;
std::vector<std::string> LogReceiver::capturedLogs_;

// DLL 函数类型
typedef void (*SetGlobalCallbackPtrType)(void (*)(DHLogLevel, const char*, const char*));
typedef void* (*CreateSinkFunc)();
typedef void* (*CreateSourceFunc)();

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  DLL 日志诊断工具 v2" << std::endl;
    std::cout << "========================================" << std::endl;

    // 加载 Sink.dll (它会链接 dh_log_callback.cpp)
    std::cout << "\n[1] 加载 Sink.dll..." << std::endl;
    HMODULE sinkLib = LoadLibraryA("Sink.dll");
    if (!sinkLib) {
        std::cerr << "  [X] 无法加载 Sink.dll" << std::endl;
        return 1;
    }
    std::cout << "  [√] Sink.dll 加载成功" << std::endl;

    // 从 Sink.dll 获取设置回调的函数
    auto SetCallback = (SetGlobalCallbackPtrType)GetProcAddress(sinkLib, "DH_SetGlobalCallbackPtr");
    if (SetCallback) {
        std::cout << "  [√] 找到 DH_SetGlobalCallbackPtr 函数" << std::endl;
        // 设置回调
        SetCallback(LogReceiver::LogCallback);
        std::cout << "  [√] 已通过 DLL 函数设置全局回调" << std::endl;
    } else {
        std::cout << "  [!] 未找到 DH_SetGlobalCallbackPtr 函数" << std::endl;
    }

    // 加载 Source.dll
    std::cout << "\n[2] 加载 Source.dll..." << std::endl;
    HMODULE sourceLib = LoadLibraryA("Source.dll");
    if (!sourceLib) {
        std::cerr << "  [X] 无法加载 Source.dll" << std::endl;
        FreeLibrary(sinkLib);
        return 1;
    }
    std::cout << "  [√] Source.dll 加载成功" << std::endl;

    // Source.dll 也导出了这个函数，再次设置
    SetCallback = (SetGlobalCallbackPtrType)GetProcAddress(sourceLib, "DH_SetGlobalCallbackPtr");
    if (SetCallback) {
        SetCallback(LogReceiver::LogCallback);
        std::cout << "  [√] 已通过 Source.dll 设置全局回调" << std::endl;
    }

    // 获取工厂函数
    std::cout << "\n[3] 获取工厂函数..." << std::endl;
    auto CreateSink = (CreateSinkFunc)GetProcAddress(sinkLib, "CreateSinkService");
    auto CreateSource = (CreateSourceFunc)GetProcAddress(sourceLib, "CreateSourceService");

    if (!CreateSink || !CreateSource) {
        std::cerr << "  [X] 无法获取工厂函数" << std::endl;
        FreeLibrary(sinkLib);
        FreeLibrary(sourceLib);
        return 1;
    }
    std::cout << "  [√] 工厂函数获取成功" << std::endl;

    // 创建服务实例（这会触发 DHLOG）
    std::cout << "\n[4] 创建服务实例..." << std::endl;
    LogReceiver::Reset();

    void* sink = CreateSink();
    void* source = CreateSource();
    std::cout << "  [√] 服务实例创建成功" << std::endl;

    std::cout << "\n[5] 检查日志捕获..." << std::endl;
    std::cout << "  控制台输出的日志数: 需要观察上述输出" << std::endl;
    std::cout << "  回调捕获的日志数: " << LogReceiver::GetLogCount() << std::endl;

    // 显示捕获的日志
    if (!LogReceiver::GetCapturedLogs().empty()) {
        std::cout << "\n[6] 捕获的日志:" << std::endl;
        for (const auto& log : LogReceiver::GetCapturedLogs()) {
            std::cout << "    " << log << std::endl;
        }
    }

    // 清理
    FreeLibrary(sinkLib);
    FreeLibrary(sourceLib);

    std::cout << "\n========================================" << std::endl;
    std::cout << "  诊断完成" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "结果:" << std::endl;

    if (LogReceiver::GetLogCount() > 0) {
        std::cout << "  状态: ✓ 回调被正确调用，日志被捕获" << std::endl;
    } else {
        std::cout << "  状态: ✗ 回调未被调用" << std::endl;
        std::cout << "\n说明: " << std::endl;
        std::cout << "  - 日志输出到控制台是正常的" << std::endl;
        std::cout << "  - 但回调捕获机制需要进一步修复" << std::endl;
    }

    return 0;
}
