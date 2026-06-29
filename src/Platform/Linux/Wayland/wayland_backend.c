#include "Platform/Linux/window_backend.h"
#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"

#include <wayland-client-core.h>

typedef struct WaylandBackend
{
    struct wl_display* display;
} WaylandBackend;

static void* wayland_backend_init();
static void  wayland_backend_shutdown(void* backend);
static void  wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  wayland_window_show(void* backend);
static void  wayland_window_destroy(void* backend);

static const WindowBackend WAYLAND_BACKEND = {
    .backend_init   = wayland_backend_init,
    .window_create  = wayland_window_create,
    .window_show    = wayland_window_show,
    .window_destroy = wayland_window_destroy,
};

const WindowBackend* wayland_backend()
{
    //
    return &WAYLAND_BACKEND;
}

void* wayland_backend_init()
{
    WaylandBackend* backend_data = malloc(sizeof(WaylandBackend));

    ANVIL_CORE_TRACE("Wayland Backend initialized.");

    return backend_data;
}

void wayland_backend_shutdown(void* backend)
{
    if(!backend) { return; };

    ANVIL_CORE_TRACE("Wayland Backend initialized.");

    free((WaylandBackend*)backend);
}

void wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{ ANVIL_CORE_TRACE("Wayland Window Created."); }

void wayland_window_show(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window Showed."); }

void wayland_window_destroy(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window Destroyed."); }
