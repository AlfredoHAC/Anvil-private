#ifndef ANVL_LOGGER_HEADER
#define ANVL_LOGGER_HEADER

typedef enum LogLevel
{
    NoLog = 0,
    Fatal,
    Error,
    Warning,
    Info,
    Debug,
    Trace,
} LogLevel;

void anvl_logger_set_level(LogLevel level);
void anvl_logger_fatal(const char* call_module, const char* msg_format, ...);
void anvl_logger_error(const char* call_module, const char* msg_format, ...);
void anvl_logger_warn(const char* call_module, const char* msg_format, ...);
void anvl_logger_info(const char* call_module, const char* msg_format, ...);
void anvl_logger_debug(const char* call_module, const char* msg_format, ...);
void anvl_logger_trace(const char* call_module, const char* msg_format, ...);

#define ANVIL_CORE_FATAL(...) anvl_logger_fatal("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_ERROR(...) anvl_logger_error("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_WARN(...) anvl_logger_warn("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_INFO(...) anvl_logger_info("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_DEBUG(...) anvl_logger_debug("ANVIL", __VA_ARGS__)
#define ANVIL_CORE_TRACE(...) anvl_logger_trace("ANVIL", __VA_ARGS__)

#define ANVIL_FATAL(ClientName, ...) anvl_logger_fatal(ClientName, __VA_ARGS__)
#define ANVIL_ERROR(ClientName, ...) anvl_logger_error(ClientName, __VA_ARGS__)
#define ANVIL_WARN(ClientName, ...) anvl_logger_warn(ClientName, __VA_ARGS__)
#define ANVIL_INFO(ClientName, ...) anvl_logger_info(ClientName, __VA_ARGS__)
#define ANVIL_DEBUG(ClientName, ...) anvl_logger_debug(ClientName, __VA_ARGS__)
#define ANVIL_TRACE(ClientName, ...) anvl_logger_trace(ClientName, __VA_ARGS__)

#endif // !ANVL_LOGGER_HEADER
