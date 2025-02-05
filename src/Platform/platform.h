#ifndef PLATFORM_HEADER
#define PLATFORM_HEADER

#include "Core/typedefs.h"

// Platform native window
typedef struct NativeWindow NativeWindow;

// Event callback function pointer type
typedef void(*PFEVENTCALLBACKFUNC)();

NativeWindow* anvlPlatformWindowCreate(const char* window_title, uint16 window_width, uint16 window_height);
void anvlPlatformWindowUpdate(NativeWindow* window);
void anvlPlatformWindowDestroy(NativeWindow* window);
void anvlPlatformSetWindowEventCallback(NativeWindow* window, PFEVENTCALLBACKFUNC event_callback);

#endif // !PLATFORM_HEADER
