#include "anvlpch.h"

#include "Platform/platform.h"
#include "Platform/Linux/Wayland/wayland_backend.h"

#include <wayland-client-core.h>

typedef struct WaylandBackend
{
    NativeWindow* window;

    struct wl_display* display;
} WaylandBackend;
