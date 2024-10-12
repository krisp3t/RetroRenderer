#pragma once
#include <mutex>

namespace RetroRenderer
{

#define LOGI(msg) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_INFO, __FILE__, __LINE__, msg)
#define LOGD(msg) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_DEBUG, __FILE__, __LINE__, msg)
#define LOGW(msg) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_WARN, __FILE__, __LINE__, msg)
#define LOGE(msg) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_ERROR, __FILE__, __LINE__, msg)

enum class LogLevel
{
    LOG_INFO,
    LOG_DEBUG,
    LOG_WARN,
    LOG_ERROR
};

class Logger
{
public:
    static void Log(LogLevel level, const char *file, int line, const char *message);
    static void SetLogLevel(LogLevel level);
private:
    static LogLevel _minLevel;
    static std::mutex _mutex;
};

}