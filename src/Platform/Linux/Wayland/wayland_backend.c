#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"

#include <wayland-client-core.h>

typedef struct WaylandBackend
{
    // Wayland connection
    struct wl_display* display;

    // Event callback
    EventCallbackFn event_callback;
} WaylandBackend;

static void* wayland_backend_init();
static void  wayland_backend_shutdown(void* backend);
static void  wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  wayland_window_show(void* backend);
static void  wayland_window_destroy(void* backend);
static void  wayland_window_set_event_callback(void* backend, EventCallbackFn event_callback);

static void wayland_events_poll_and_dispatch(void* backend);

static const WindowBackend WAYLAND_BACKEND = {
    .backend_init                    = wayland_backend_init,
    .backend_shutdown                = wayland_backend_shutdown,
    .window_create                   = wayland_window_create,
    .window_show                     = wayland_window_show,
    .window_destroy                  = wayland_window_destroy,
    .window_set_event_callback       = wayland_window_set_event_callback,
    .window_events_poll_and_dispatch = wayland_events_poll_and_dispatch,
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
    if (!backend) { return; };

    ANVIL_CORE_TRACE("Wayland Backend initialized.");

    free((WaylandBackend*)backend);
}

void wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{ ANVIL_CORE_TRACE("Wayland Window Created."); }

void wayland_window_show(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window Showed."); }

void wayland_window_destroy(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window Destroyed."); }

void wayland_window_set_event_callback(void* backend, EventCallbackFn event_callback)
{ ANVIL_CORE_TRACE("Wayland Window EventCallback set."); }

static void wayland_events_poll_and_dispatch(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window polling events."); }
