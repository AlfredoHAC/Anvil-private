#include "anvlpch.h"

#include "logger.h"

#include <time.h>

#define MAX_LOG_MSG_LENGTH 256
static LogLevel current_level;

void anvlLoggerSetLevel(LogLevel level)
{
	current_level = level;
}

static void _PrintTimestampLabel()
{
	time_t current_time_raw;
	struct tm* current_localtime;

	time(&current_time_raw);
	current_localtime = localtime(&current_time_raw);

	const char* timestamp_format = "[%02d:%02d:%02d] ";

	fprintf(stderr, timestamp_format, current_localtime->tm_hour, current_localtime->tm_min, current_localtime->tm_sec);
}

static void _PrintLevelLabel(LogLevel level)
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
	}

	fprintf(stderr, level_label_format, color, level_str, "\033[0m");
}

static void _LogMessage(LogLevel level, const char* call_module, const char* msg_format, va_list args)
{
	if (level > current_level)
	{
		return;
	}

	_PrintTimestampLabel();
	_PrintLevelLabel(level);
	fprintf(stderr, "%s: ", call_module);

	vfprintf(stderr, msg_format, args);
	fprintf(stderr, "\n");
}

void anvlLogFatal(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Fatal, "ANVIL", msg_format, args);

	va_end(args);
}

void anvlLogError(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Error, "ANVIL", msg_format, args);

	va_end(args);
}

void anvlLogWarn(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Warning, "ANVIL", msg_format, args);

	va_end(args);
}

void anvlLogInfo(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Info, "ANVIL", msg_format, args);

	va_end(args);
}

void anvlLogDebug(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Debug, "ANVIL", msg_format, args);

	va_end(args);
}

void anvlLogTrace(const char* call_module, const char* msg_format, ...)
{
	va_list args;
	va_start(args, msg_format);

	_LogMessage(Trace, "ANVIL", msg_format, args);

	va_end(args);
}
