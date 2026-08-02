#ifndef ANVIL_ASSERT_HEADER
#define ANVIL_ASSERT_HEADER

#include "Platform/platform_detection.h"
#include "Tools/logger.h"

#ifdef ANVIL_CONFIG_DEBUG
#    define ANVIL_ASSERT(expression)                                                               \
        do                                                                                         \
        {                                                                                          \
            if (!(expression))                                                                       \
            {                                                                                      \
                ANVIL_CORE_FATAL("Assertion failed: %s\n at %s:%d in %s",                          \
                                 #expression,                                                      \
                                 __FILE__,                                                         \
                                 __LINE__,                                                         \
                                 __func__);                                                        \
                ANVIL_BREAK();                                                                     \
            }                                                                                      \
        } while (0)

#    define ANVIL_ASSERT_MSG(expression, msg, ...)                                                 \
        do                                                                                         \
        {                                                                                          \
            if (!expression)                                                                       \
            {                                                                                      \
                ANVIL_CORE_FATAL("Assertion failed: %s\n %s\n at %s:%d in %s",                     \
                                 #expression,                                                      \
                                 msg,                                                              \
                                 __FILE__,                                                         \
                                 __LINE__,                                                         \
                                 __func__,                                                         \
                                 ##__VA_ARGS__);                                                   \
                ANVIL_BREAK();                                                                     \
            }                                                                                      \
        } while (0)
#else
#    define ANVIL_ASSERT(expression)               ((void)0)
#    define ANVIL_ASSERT_MSG(expression, msg, ...) ((void)0)
#endif

#endif // ANVIL_ASSERT_HEADER
