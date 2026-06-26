#include "anvlpch.h"

#include "logger.h"
#include "Platform/platform_detection.h"

#include <time.h>

#define MAX_LOG_MSG_LENGTH 256
static LogLevel current_level;

void anvl_logger_set_level(LogLevel level)
{
    current_level = level;
}

static void _print_timestamp_label()
{
    time_t current_time_raw;
    struct tm current_localtime;

    time(&current_time_raw);
    #if defined(ANVIL_PLATFORM_WINDOWS)
    localtime_s(&current_localtime, &current_time_raw);
    #elif defined(ANVIL_PLATFORM_LINUX)
    localtime_r(&current_localtime, &current_time_raw);
    #endif

    const char* timestamp_format = "[%02d:%02d:%02d] ";

    fprintf(stderr, timestamp_format, current_localtime.tm_hour, current_localtime.tm_min, current_localtime.tm_sec);
}

static void _print_level_label(LogLevel level)
{
    char* level_label_format = "%s(%s)%s ";
    const char* level_str = "";
    const char* color = "";

    switch (level)
    {
    case Fatal:
        color = "\033[31m";
        level_str = "FATAL";
        break;
    case Error:
        color = "\033[38;5;202m";
        level_str = "ERROR";
        break;
    case Warning:
        color = "\033[93m";
        level_str = "WARNING";
        break;
    case Info:
        color = "\033[32m";
        level_str = "INFO";
        break;
    case Debug:
        color = "\033[36m";
        level_str = "DEBUG";
        break;
    case Trace:
        color = "\033[34m";
        level_str = "TRACE";
        break;
    default:
        color = "\033[0m";
        level_str = "UNKNOWN";
    }

    fprintf(stderr, level_label_format, color, level_str, "\033[0m");
}

static void _log_message(LogLevel level, const char* call_module, const char* msg_format, va_list args)
{
    if ((uint16)level > (uint16)current_level)
    {
        return;
    }

    _print_timestamp_label();
    _print_level_label(level);
    fprintf(stderr, "%s: ", call_module);

    vfprintf(stderr, msg_format, args);
    fprintf(stderr, "\n");
}

void anvl_logger_fatal(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Fatal, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_error(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Error, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_warn(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Warning, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_info(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Info, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_debug(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Debug, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_trace(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(Trace, call_module, msg_format, args);

    va_end(args);
}
