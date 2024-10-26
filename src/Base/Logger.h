#pragma once

namespace RetroRenderer
{

#define LOGI(...) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOGD(...) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOGW(...) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOGE(...) RetroRenderer::Logger::Log(RetroRenderer::LogLevel::LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)

enum class LogLevel
{
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

class Logger
{
public:
    static void Log(LogLevel level, const char *file, int line, const char *format, ...);
    static void SetLogLevel(LogLevel level);
private:
    static LogLevel _minLevel;
};

}