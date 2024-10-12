/**
 * Simple thread-safe logger for debugging purposes. No-op in release builds.
 */

#include <iostream>
#include <chrono>
#include "Logger.h"

namespace RetroRenderer
{

LogLevel Logger::_minLevel = LogLevel::LOG_INFO;
std::mutex Logger::_mutex;

// ANSI color codes
const char* color_reset = "\u001b[0m";
const char* color_info = "\u001b[37m";     // White
const char* color_debug = "\u001b[36m";    // Cyan
const char* color_warn = "\u001b[33m";     // Yellow
const char* color_error = "\u001b[31m";    // Red

#ifndef NDEBUG
void Logger::Log(LogLevel level, const char *file, int line, const char *message)
{
    std::lock_guard<std::mutex> lock_guard(_mutex);
    const auto timestamp = std::chrono::system_clock::now();
    if (level < _minLevel)
    {
        return;
    }
    std::cout << "[" << timestamp << "] ";
    switch (level)
    {
    case LogLevel::LOG_INFO:
        std::cout << color_info << "[INFO] ";
        break;
    case LogLevel::LOG_DEBUG:
        std::cout << color_debug << "[DEBUG] ";
        break;
    case LogLevel::LOG_WARN:
        std::cout << color_warn << "[WARNING] ";
        break;
    case LogLevel::LOG_ERROR:
        std::cout << color_error << "[ERROR] ";
        break;
    }

    std::cout << message << color_reset << std::endl;

    if (level > LogLevel::LOG_INFO)
    {
        std::cout << file << ":" << line << std::endl;
    }
}
#else
void Logger::Log(LogLevel level, const char *file, int line, const char *message) {}
#endif

}