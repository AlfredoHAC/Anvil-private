#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"
#include "Platform/Linux/X11/x11_backend.h"
#include "Platform/Linux/window_backend.h"
#include "Platform/platform.h"

typedef struct NativeWindow
{
    const WindowBackend* backend;
    void*                backend_data;

    EventCallbackFn event_callback;
} NativeWindow;

typedef enum
{
    wbNone = 0,
    wbX11,
    wbWayland,
} WindowBackendType;

static const WindowBackend* _window_backend_create(NativeWindow* window);
static uint32               _window_backend_detect();

NativeWindow* anvl_platform_window_create(const char* window_title, uint16 window_width, uint16 window_height)
{
    NativeWindow* window = malloc(sizeof(NativeWindow));

    window->backend = _window_backend_create(window);
    if (!window->backend) { return NULL; }

    window->backend_data = window->backend->backend_init();

    window->backend->window_create(window->backend_data, window_title, window_width, window_height);
    // ...
    return window;
}

void anvl_platform_window_show(NativeWindow* window)
{ window->backend->window_show(window->backend_data); }

void anvl_platform_window_update(NativeWindow* window)
{
}

void anvl_platform_window_destroy(NativeWindow* window)
{

    window->backend->window_destroy(window->backend_data);
    window->backend->backend_shutdown(window->backend_data);
}

void anvl_platform_set_window_event_callback(NativeWindow* window, EventCallbackFn event_callback)
{
    if (!event_callback) { return; }

    window->event_callback = event_callback;
}

static const WindowBackend* _window_backend_create(NativeWindow* window)
{
    WindowBackendType window_backend_type = _window_backend_detect();
    if (window_backend_type == wbX11) { return x11_backend(); }
    else if (window_backend_type == wbWayland) { return wayland_backend(); }
    else
    {
        return NULL;
    }
}

static uint32 _window_backend_detect()
{
    char* xdg_session_type = getenv("XDG_SESSION_TYPE");
    char* x11_display      = getenv("DISPLAY");
    char* wayland_display  = getenv("WAYLAND_DISPLAY");

    if (xdg_session_type && strcmp(xdg_session_type, "x11") == 0 && x11_display != NULL) { return wbX11; }
    else if (xdg_session_type && strcmp(xdg_session_type, "wayland") == 0 && wayland_display != NULL)
    {
        return wbWayland;
    }

    return wbNone;
}
