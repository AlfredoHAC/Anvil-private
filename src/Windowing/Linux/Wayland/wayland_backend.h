#ifndef ANVIL_WINDOW_BACKEND_WAYLAND_HEADER
#define ANVIL_WINDOW_BACKEND_WAYLAND_HEADER

#include "Windowing/window_backend.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-egl-core.h>
#include <wayland-egl.h>

typedef struct AnvlWaylandBackend AnvlWaylandBackend;

typedef struct AnvlGraphicsContext
{
    struct wl_egl_window* egl_window;
    EGLDisplay            display;
    EGLSurface            surface;
    EGLContext            handle;
} AnvlGraphicsContext;

const AnvlWindowBackend* wayland_backend();

#endif // !ANVL_WINDOW_BACKEND_WAYLAND_HEADER
