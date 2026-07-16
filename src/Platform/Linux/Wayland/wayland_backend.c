#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"
#include "Platform/Linux/Wayland/xdg_shell_client_protocol.h"

#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>

typedef struct WaylandBackend
{
    // Wayland connection
    struct wl_display* display;

    // Wayland registry
    struct wl_registry* registry;

    // Wayland objects
    struct wl_compositor* compositor;
    struct xdg_wm_base*   wm_base;

    // Event callback
    EventCallbackFn event_callback;
} WaylandBackend;

static void* wayland_backend_init();
static void  wayland_backend_shutdown(void* backend);
static void  wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  wayland_window_show(void* backend);
static void  wayland_window_destroy(void* backend);
static void  wayland_window_set_event_callback(void* backend, EventCallbackFn event_callback);
static void  wayland_events_poll_and_dispatch(void* backend);

static void _on_wl_registry_global_notify(
    void* data, struct wl_registry* registry, uint32 id, const char* interface, uint32 version);

static const WindowBackend WAYLAND_BACKEND = {
    .backend_init                    = wayland_backend_init,
    .backend_shutdown                = wayland_backend_shutdown,
    .window_create                   = wayland_window_create,
    .window_show                     = wayland_window_show,
    .window_destroy                  = wayland_window_destroy,
    .window_set_event_callback       = wayland_window_set_event_callback,
    .window_events_poll_and_dispatch = wayland_events_poll_and_dispatch,
};

static const struct wl_registry_listener registry_listener = {
    .global        = _on_wl_registry_global_notify,
    .global_remove = NULL,
};

// clang-format off
const WindowBackend* wayland_backend()
{
    return &WAYLAND_BACKEND;
}
// clang-format on

void* wayland_backend_init()
{
    WaylandBackend* backend_data = malloc(sizeof(WaylandBackend));

    backend_data->display = wl_display_connect(NULL);
    if (!backend_data->display)
    {
        free(backend_data);
        return NULL;
    }

    backend_data->registry = wl_display_get_registry(backend_data->display);
    if (!backend_data->registry)
    {
        free(backend_data->display);
        free(backend_data);
        return NULL;
    }
    wl_registry_add_listener(backend_data->registry, &registry_listener, (void*)backend_data);
    wl_display_roundtrip(backend_data->display);

    if (!backend_data->compositor || !backend_data->wm_base)
    {
        wl_registry_destroy(backend_data->registry);
        wl_display_disconnect(backend_data->display);
        free(backend_data);
        return NULL;
    }

    return backend_data;
}

void wayland_backend_shutdown(void* backend)
{
    if (!backend) { return; }

    WaylandBackend* b_end = (WaylandBackend*)backend;

    wl_compositor_destroy(b_end->compositor);
    xdg_wm_base_destroy(b_end->wm_base);

    wl_registry_destroy(b_end->registry);
    wl_display_disconnect(b_end->display);

    free(b_end);
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

static void _on_wl_registry_global_notify(
    void* data, struct wl_registry* registry, uint32 id, const char* interface, uint32 version)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        b_end->compositor = wl_registry_bind(b_end->registry, id, &wl_compositor_interface, version);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        b_end->wm_base = wl_registry_bind(b_end->registry, id, &xdg_wm_base_interface, version);
    }
}
