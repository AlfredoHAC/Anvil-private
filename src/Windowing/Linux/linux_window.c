#include "anvlpch.h"

#include "Window/window.h"
#include "Windowing/Linux/Wayland/wayland_backend.h"
#include "Windowing/Linux/X11/x11_backend.h"
#include "Windowing/window_backend.h"

typedef struct NativeWindow
{
    const WindowBackend* backend;
    void*                backend_data;
} NativeWindow;

typedef enum WindowBackendType
{
    ANVL_WINDOW_BACKEND_NONE = 0,
    ANVL_WINDOW_BACKEND_X11,
    ANVL_WINDOW_BACKEND_WAYLAND,
} WindowBackendType;

static const WindowBackend* _window_backend_create(NativeWindow* window);
static uint32               _window_backend_detect();

NativeWindow* anvl_platform_window_create(const char* window_title,
                                          uint16      window_width,
                                          uint16      window_height)
{
    NativeWindow* window = malloc(sizeof(NativeWindow));
    ANVIL_ASSERT(window != NULL);

    window->backend = _window_backend_create(window);
    ANVIL_ASSERT(window->backend != NULL);

    window->backend_data = window->backend->backend_init();
    ANVIL_ASSERT(window->backend_data != NULL);

    window->backend->window_create(window->backend_data, window_title, window_width, window_height);

    return window;
}

// clang-format off
void anvl_platform_window_show(NativeWindow* window)
{
    window->backend->window_show(window->backend_data);
}

void anvl_platform_window_update(NativeWindow* window)
{
    window->backend->window_events_poll_and_dispatch(window->backend_data);
}
// clang-format on

void anvl_platform_window_destroy(NativeWindow* window)
{
    ANVIL_ASSERT(window != NULL);

    window->backend->window_destroy(window->backend_data);
    window->backend->backend_shutdown(window->backend_data);

    free(window);
}

void anvl_platform_window_set_event_callback(NativeWindow* window, EventCallbackFn event_callback)
{
    ANVIL_ASSERT(event_callback != NULL);

    window->backend->window_set_event_callback(window->backend_data, event_callback);
}

void anvl_platform_window_unset_event_callback(NativeWindow* window)
{
    window->backend->window_set_event_callback(window->backend_data, NULL);
}

static const WindowBackend* _window_backend_create(NativeWindow* window)
{
    WindowBackendType window_backend_type = _window_backend_detect();
    if (window_backend_type == ANVL_WINDOW_BACKEND_WAYLAND) { return wayland_backend(); }
    else if (window_backend_type == ANVL_WINDOW_BACKEND_X11) { return x11_backend(); }
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

    if (xdg_session_type != NULL && wayland_display != NULL &&
        strcmp(xdg_session_type, "wayland") == 0)
    {
        return ANVL_WINDOW_BACKEND_WAYLAND;
    }
    else if (xdg_session_type != NULL && x11_display != NULL &&
             strcmp(xdg_session_type, "x11") == 0)
    {
        return ANVL_WINDOW_BACKEND_X11;
    }

    return ANVL_WINDOW_BACKEND_NONE;
}
