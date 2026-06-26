#ifndef ANVL_PLATFORM_HEADER
#define ANVL_PLATFORM_HEADER

#include "Platform/event.h"
#include "Platform/typedefs.h"

// Platform native window
typedef struct NativeWindow NativeWindow;

// Event callback function pointer type
typedef void (*EventCallbackFn)(Event event);

NativeWindow* anvl_platform_window_create(const char* window_title, uint16 window_width, uint16 window_height);
void anvl_platform_window_show(NativeWindow* window);
void anvl_platform_window_update(NativeWindow* window);
void anvl_platform_window_destroy(NativeWindow* window);
void anvl_platform_set_window_event_callback(NativeWindow* window, EventCallbackFn event_callback);

#endif // !ANVL_PLATFORM_HEADER
