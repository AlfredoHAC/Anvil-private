#ifndef BASE_HEADER
#define BASE_HEADER

// Compiler/Platform detection
// Compiler macros (MSVC, GCC, etc.)
#if defined(_MSC_VER)
#	define ANVIL_COMPILER_MSVC 1

// Target OS (Windows, Linux, macOS)
#	if defined(_WIN32) || defined(_WIN64)
#		define ANVIL_PLATFORM_WINDOWS 1
#	else
#		error OS platform not supported!
#	endif

#elif defined(__GNUC__)
#	define ANVIL_COMPILER_GCC 1

#	if defined(_WIN32) || defined(_WIN64)
#		define ANVIL_PLATFORM_WINDOWS 1
#	elif defined(__gnu_linux__) || defined(__linux__)
#		define ANVIL_PLATFORM_LINUX 1
#	elif defined(__APPLE__) && defined(__MACH__)
#		define ANVIL_PLATFORM_MACOS 1
#	else
#		error OS platform not supported!
#	endif

#endif

// Build configuration flags
#if defined(ANVIL_CONFIG_DEBUG)
#	if defined(ANVIL_PLATFORM_WINDOWS)
#		define ANVIL_BREAK() __debugbreak()
#	elif defined(ANVIL_PLATFORM_LINUX)
#		define ANVIL_BREAK() raise(SIGTRAP)
#	else
#		error OS platform not supported!
#	endif
#else
#	define ANVIL_CONFIG_DEBUG 0
#endif

#if !defined(ANVIL_COMPILER_MSVC)
#	define ANVIL_COMPILER_MSVC 0
#endif
#if !defined(ANVIL_COMPILER_GCC)
#	define ANVIL_COMPILER_GCC 0
#endif
#if !defined(ANVIL_PLATFORM_WINDOWS)
#	define ANVIL_PLATFORM_WINDOWS 0
#endif
#if !defined(ANVIL_PLATFORM_LINUX)
#	define ANVIL_PLATFORM_LINUX 0
#endif
#if !defined(ANVIL_PLATFORM_MACOS)
#	define ANVIL_PLATFORM_MACOS 0
#endif



#endif // !BASE_HEADER
