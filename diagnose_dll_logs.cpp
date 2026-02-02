/*
 * DLL 日志诊断工具
 * 用于验证 DHLOG 日志是否正确输出
 */

#include <iostream>
#include <windows.h>
#include <string>
#include <vector>

// DLL 日志回调类型定义
enum class DHLogLevel {
    DH_INFO,
    DH_WARN,
    DH_ERROR,
    DH_DEBUG
};

// 全局回调函数指针
extern "C" {
    void (*g_DH_LogCallback)(DHLogLevel, const char*, const char*) = nullptr;
}

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

// 设置回调的函数类型
typedef void (*LogCallbackType)(DHLogLevel, const char*, const char*);
typedef void (*SetGlobalCallbackPtrType)(LogCallbackType);

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  DLL 日志诊断工具" << std::endl;
    std::cout << "========================================" << std::endl;

    // 设置全局回调
    std::cout << "\n[1] 设置全局日志回调..." << std::endl;
    g_DH_LogCallback = LogReceiver::LogCallback;
    std::cout << "  [√] 全局回调已设置" << std::endl;
    std::cout << "  回调地址: " << (void*)g_DH_LogCallback << std::endl;

    // 加载 Sink.dll
    std::cout << "\n[2] 加载 Sink.dll..." << std::endl;
    HMODULE sinkLib = LoadLibraryA("Sink.dll");
    if (!sinkLib) {
        std::cerr << "  [X] 无法加载 Sink.dll" << std::endl;
        return 1;
    }
    std::cout << "  [√] Sink.dll 加载成功" << std::endl;

    // 加载 Source.dll
    std::cout << "\n[3] 加载 Source.dll..." << std::endl;
    HMODULE sourceLib = LoadLibraryA("Source.dll");
    if (!sourceLib) {
        std::cerr << "  [X] 无法加载 Source.dll" << std::endl;
        FreeLibrary(sinkLib);
        return 1;
    }
    std::cout << "  [√] Source.dll 加载成功" << std::endl;

    // 获取工厂函数
    std::cout << "\n[4] 获取工厂函数..." << std::endl;
    auto CreateSink = (void*(*)())GetProcAddress(sinkLib, "CreateSinkService");
    auto CreateSource = (void*(*)())GetProcAddress(sourceLib, "CreateSourceService");

    if (!CreateSink || !CreateSource) {
        std::cerr << "  [X] 无法获取工厂函数" << std::endl;
        FreeLibrary(sinkLib);
        FreeLibrary(sourceLib);
        return 1;
    }
    std::cout << "  [√] 工厂函数获取成功" << std::endl;

    // 创建服务实例
    std::cout << "\n[5] 创建服务实例..." << std::endl;
    LogReceiver::Reset();

    void* sink = CreateSink();
    void* source = CreateSource();
    std::cout << "  [√] 服务实例创建成功" << std::endl;
    std::cout << "  捕获日志数: " << LogReceiver::GetLogCount() << std::endl;

    // 显示捕获的日志
    if (!LogReceiver::GetCapturedLogs().empty()) {
        std::cout << "\n[6] 捕获的日志:" << std::endl;
        for (size_t i = 0; i < LogReceiver::GetCapturedLogs().size() && i < 10; ++i) {
            std::cout << "    " << LogReceiver::GetCapturedLogs()[i] << std::endl;
        }
        if (LogReceiver::GetCapturedLogs().size() > 10) {
            std::cout << "    ... (共 " << LogReceiver::GetCapturedLogs().size() << " 条)" << std::endl;
        }
    }

    // 清理
    FreeLibrary(sinkLib);
    FreeLibrary(sourceLib);

    std::cout << "\n========================================" << std::endl;
    std::cout << "  诊断完成" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "总结:" << std::endl;
    std::cout << "  全局回调地址: " << (void*)g_DH_LogCallback << std::endl;
    std::cout << "  捕获日志总数: " << LogReceiver::GetLogCount() << std::endl;

    if (LogReceiver::GetLogCount() > 0) {
        std::cout << "  状态: ✓ DLL 日志回调正常工作" << std::endl;
    } else {
        std::cout << "  状态: ✗ 未捕获到 DLL 日志" << std::endl;
        std::cout << "\n可能的原因:" << std::endl;
        std::cout << "  1. DLL 编译时没有链接 dh_log_callback.cpp" << std::endl;
        std::cout << "  2. DLL 使用了不同的 DHLOG 定义" << std::endl;
    }

    return 0;
}
