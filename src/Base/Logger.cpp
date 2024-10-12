/**
 * Simple thread-safe logger for debugging purposes. Only in debug builds.
 */

#include <iostream>
#include <chrono>
#include "Logger.h"

namespace RetroRenderer
{

LogLevel Logger::_minLevel = LogLevel::LOG_INFO;
std::mutex Logger::_mutex;

#ifndef NDEBUG
void Logger::Log(LogLevel level, const char *file, int line, const char *message)
{
    std::lock_guard<std::mutex> lock_guard(_mutex);
    const auto timestamp = std::chrono::system_clock::now();
    if (level < _minLevel)
    {
        return;
    }
    std::cout << "[" << timestamp << "] " << message << std::endl;
}
#else
// no-op on release builds
void Logger::Log(LogLevel level, const char *file, int line, const char *message) {}
#endif

}