#include "anvlpch.h"

#include "Platform/platform_detection.h"
#include "Tools/logger.h"

#include <time.h>

#define MAX_LOG_MSG_LENGTH 256

static void _print_timestamp_label();
static void _print_level_label(LogLevel level);
static void _log_message(LogLevel    level,
                         const char* call_module,
                         const char* msg_format,
                         va_list     args);

static LogLevel current_level;

// clang-format off
void anvl_logger_set_level(LogLevel level)
{
    ANVIL_ASSERT(level >= ANVL_LOG_LEVEL_NONE && level <= ANVL_LOG_LEVEL_TRACE);

    current_level = level;
}
// clang-format on

void anvl_logger_fatal(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_FATAL, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_error(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_ERROR, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_warn(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_WARNING, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_info(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_INFO, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_debug(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_DEBUG, call_module, msg_format, args);

    va_end(args);
}

void anvl_logger_trace(const char* call_module, const char* msg_format, ...)
{
    va_list args;
    va_start(args, msg_format);

    _log_message(ANVL_LOG_LEVEL_TRACE, call_module, msg_format, args);

    va_end(args);
}

static void _print_timestamp_label()
{
    time_t    current_time_raw;
    struct tm current_localtime;

    time(&current_time_raw);
#if ANVIL_PLATFORM_WINDOWS
    localtime_s(&current_localtime, &current_time_raw);
#elif ANVIL_PLATFORM_LINUX
    localtime_r(&current_time_raw, &current_localtime);
#endif

    const char* timestamp_format = "[%02d:%02d:%02d] ";

    fprintf(stderr,
            timestamp_format,
            current_localtime.tm_hour,
            current_localtime.tm_min,
            current_localtime.tm_sec);
}

static void _print_level_label(LogLevel level)
{
    char*       level_label_format = "%s(%s)%s ";
    const char* level_str          = "";
    const char* color              = "";

    switch (level)
    {
        case ANVL_LOG_LEVEL_FATAL:
            color     = "\033[31m";
            level_str = "FATAL";
            break;
        case ANVL_LOG_LEVEL_ERROR:
            color     = "\033[38;5;202m";
            level_str = "ERROR";
            break;
        case ANVL_LOG_LEVEL_WARNING:
            color     = "\033[93m";
            level_str = "WARNING";
            break;
        case ANVL_LOG_LEVEL_INFO:
            color     = "\033[32m";
            level_str = "INFO";
            break;
        case ANVL_LOG_LEVEL_DEBUG:
            color     = "\033[36m";
            level_str = "DEBUG";
            break;
        case ANVL_LOG_LEVEL_TRACE:
            color     = "\033[34m";
            level_str = "TRACE";
            break;
        default: color = "\033[0m"; level_str = "UNKNOWN";
    }

    fprintf(stderr, level_label_format, color, level_str, "\033[0m");
}

static void _log_message(LogLevel    level,
                         const char* call_module,
                         const char* msg_format,
                         va_list     args)
{
    ANVIL_ASSERT(level >= ANVL_LOG_LEVEL_NONE && level <= ANVL_LOG_LEVEL_TRACE);

    if (level > current_level) { return; }

    _print_timestamp_label();
    _print_level_label(level);

    ANVIL_ASSERT(call_module != NULL);

    fprintf(stderr, "%s: ", call_module);

    ANVIL_ASSERT(msg_format != NULL);

    vfprintf(stderr, msg_format, args);
    fprintf(stderr, "\n");
}
