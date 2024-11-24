/**
 * Simple thread-safe logger for debugging purposes. No-op in release builds.
 */

#include <iostream>
#include <chrono>
#include <cstdarg>
#include "Logger.h"

namespace RetroRenderer
{

LogLevel Logger::_minLevel = LogLevel::LOG_DEBUG;

constexpr int MAX_LOG_LENGTH = 1024;

// ANSI color codes
const char* color_reset = "\u001b[0m";
const char* color_info = "\u001b[37m";     // White
const char* color_debug = "\u001b[36m";    // Cyan
const char* color_warn = "\u001b[33m";     // Yellow
const char* color_error = "\u001b[31m";    // Red

#ifndef NDEBUG
void Logger::Log(LogLevel level, const char *file, int line, const char *format, ...)
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm local_time;
    localtime_s(&local_time, &now_time);
    if (level < _minLevel)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    char formatted_message[MAX_LOG_LENGTH];
    vsnprintf(formatted_message, MAX_LOG_LENGTH - 1, format, args);
    va_end(args);
    formatted_message[MAX_LOG_LENGTH - 1] = '\0';  // Ensure null termination

    char time_buffer[20];
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", &local_time);
    std::cout << "[" << time_buffer << "] ";
    switch (level)
    {
    case LogLevel::LOG_INFO:
        std::cout << color_info << "[INFO] ";
        break;
    case LogLevel::LOG_DEBUG:
        std::cout << color_debug << "[DEBUG] ";
        break;
    case LogLevel::LOG_WARN:
        std::cerr << color_warn << "[WARNING] ";
        break;
    case LogLevel::LOG_ERROR:
        std::cerr << color_error << "[ERROR] ";
        break;
    }

    std::cout << formatted_message << color_reset << std::endl;

    if (level > LogLevel::LOG_INFO)
    {
        std::cout << file << ":" << line << std::endl;
    }
}
#else
void Logger::Log(LogLevel level, const char *file, int line, const char *message) {}
#endif

}