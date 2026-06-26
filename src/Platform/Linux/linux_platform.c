#include "anvlpch.h"

#include "Platform/platform.h"

typedef struct NativeWindow
{
    EventCallbackFn event_callback;
} NativeWindow;

NativeWindow* anvl_platform_window_create(const char* window_title, uint16 window_width, uint16 window_height)
{
    // ...
    return NULL;
}

void anvl_platform_window_show(NativeWindow* window)
{
}

void anvl_platform_window_update(NativeWindow* window)
{
}

void anvl_platform_window_destroy(NativeWindow* window)
{
}

void anvl_platform_set_window_event_callback(NativeWindow* window, EventCallbackFn event_callback)
{
    if (!event_callback) { return; }

    window->event_callback = event_callback;
}
