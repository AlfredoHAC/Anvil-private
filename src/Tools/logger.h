#ifndef ANVL_LOGGER_HEADER
#define ANVL_LOGGER_HEADER

#include "Core/typedefs.h"

typedef enum LogLevel
{
	NoLog = 0,
	Fatal,
	Error,
	Warning,
	Info,
	Trace,
	All
} LogLevel;

void anvlLoggerSetLevel(LogLevel level);
void anvlLogFatal(const char* call_module, const char* msg_format, ...);
void anvlLogError(const char* call_module, const char* msg_format, ...);
void anvlLogWarn(const char* call_module, const char* msg_format, ...);
void anvlLogInfo(const char* call_module, const char* msg_format, ...);
void anvlLogTrace(const char* call_module, const char* msg_format, ...);

#define ANVIL_CORE_FATAL(...)		anvlLogFatal("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_ERROR(...)       anvlLogError("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_WARN(...)		anvlLogWarn("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_INFO(...)        anvlLogInfo("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_TRACE(...)       anvlLogTrace("ANVIL", __VA_ARGS__)

#define ANVIL_FATAL(ClientName, ...)	anvlLogFatal(ClientName, __VA_ARGS__)
#define ANVIL_ERROR(ClientName, ...)    anvlLogError(ClientName, __VA_ARGS__)
#define ANVIL_WARN(ClientName, ...)		anvlLogWarn(ClientName, __VA_ARGS__)
#define ANVIL_INFO(ClientName, ...)     anvlLogInfo(ClientName, __VA_ARGS__)
#define ANVIL_TRACE(ClientName, ...)    anvlLogTrace(ClientName, __VA_ARGS__)

#endif // !ANVL_LOGGER_HEADER
