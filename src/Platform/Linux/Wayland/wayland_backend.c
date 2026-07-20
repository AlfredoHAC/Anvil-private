#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"
#include "Platform/Linux/Wayland/xdg_shell_client_protocol.h"
#include "Platform/Linux/Wayland/xdg_shell_decoration_protocol.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
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
    struct wl_compositor*              compositor;
    struct xdg_wm_base*                wm_base;
    struct wl_surface*                 surface;
    struct xdg_surface*                xdg_surface;
    struct xdg_toplevel*               top_level;
    struct wl_shm*                     shared_mem;
    struct zxdg_decoration_manager_v1* dc_manager;

    // Configure data
    const char* title;
    uint32      width;
    uint32      height;

    // Shared Memory (Pixel Buffer) data
    int32             shm_fd;
    void*             shm_data;
    struct wl_buffer* shm_buffer;

    // Decoration data
    struct zxdg_toplevel_decoration_v1* dc_object;

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

static void _create_shm_buffer(WaylandBackend* backend, int32 width, int32 height);

static void _on_wl_registry_global_notify(
    void* data, struct wl_registry* registry, uint32 id, const char* interface, uint32 version);
static void _on_xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32 serial);
static void _on_xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32 serial);

static const WindowBackend WAYLAND_BACKEND = {
    .backend_init                    = wayland_backend_init,
    .backend_shutdown                = wayland_backend_shutdown,
    .window_create                   = wayland_window_create,
    .window_show                     = wayland_window_show,
    .window_destroy                  = wayland_window_destroy,
    .window_set_event_callback       = wayland_window_set_event_callback,
    .window_events_poll_and_dispatch = wayland_events_poll_and_dispatch,
};

static const struct wl_registry_listener REGISTRY_LISTENER = {
    .global        = _on_wl_registry_global_notify,
    .global_remove = NULL,
};

static const struct xdg_wm_base_listener XDG_WM_BASE_LISTENER = {
    .ping = _on_xdg_wm_base_ping,
};

static const struct xdg_surface_listener XDG_SURFACE_LISTENER = {
    .configure = _on_xdg_surface_configure,
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
    wl_registry_add_listener(backend_data->registry, &REGISTRY_LISTENER, (void*)backend_data);
    wl_display_roundtrip(backend_data->display);

    if (!backend_data->compositor || !backend_data->wm_base)
    {
        wl_registry_destroy(backend_data->registry);
        wl_display_disconnect(backend_data->display);
        free(backend_data);
        return NULL;
    }
    xdg_wm_base_add_listener(backend_data->wm_base, &XDG_WM_BASE_LISTENER, (void*)backend_data);

    return backend_data;
}

void wayland_backend_shutdown(void* backend)
{
    if (!backend) { return; }

    WaylandBackend* b_end       = (WaylandBackend*)backend;
    uint64          buffer_size = b_end->width * b_end->height * sizeof(uint32);

    if (b_end->dc_object) { zxdg_toplevel_decoration_v1_destroy(b_end->dc_object); }

    xdg_toplevel_destroy(b_end->top_level);
    xdg_surface_destroy(b_end->xdg_surface);

    if (b_end->shm_buffer)
    {
        wl_buffer_destroy(b_end->shm_buffer);
        b_end->shm_buffer = NULL;
    }
    if (b_end->surface)
    {
        wl_surface_destroy(b_end->surface);
        b_end->surface = NULL;
    }

    if (b_end->dc_manager) { zxdg_decoration_manager_v1_destroy(b_end->dc_manager); }

    wl_shm_destroy(b_end->shared_mem);
    xdg_wm_base_destroy(b_end->wm_base);
    wl_compositor_destroy(b_end->compositor);

    if (b_end->registry) { wl_registry_destroy(b_end->registry); }
    if (b_end->display) { wl_display_disconnect(b_end->display); }

    if (b_end->shm_data != MAP_FAILED && b_end->shm_fd >= 0) { munmap(b_end->shm_data, buffer_size); }
    close(b_end->shm_fd);

    free(b_end);
}

void wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;
    b_end->title          = window_title;
    b_end->height         = height;
    b_end->width          = width;

    b_end->surface     = wl_compositor_create_surface(b_end->compositor);
    b_end->xdg_surface = xdg_wm_base_get_xdg_surface(b_end->wm_base, b_end->surface);
    xdg_surface_add_listener(b_end->xdg_surface, &XDG_SURFACE_LISTENER, (void*)b_end);

    b_end->top_level = xdg_surface_get_toplevel(b_end->xdg_surface);
    xdg_toplevel_set_app_id(b_end->top_level, "ANVIL_WINDOW");
    xdg_toplevel_set_title(b_end->top_level, window_title);
    xdg_toplevel_set_min_size(b_end->top_level, width, height);

    if (b_end->dc_manager)
    {
        b_end->dc_object = zxdg_decoration_manager_v1_get_toplevel_decoration(b_end->dc_manager, b_end->top_level);

        if (b_end->dc_object)
        {
            zxdg_toplevel_decoration_v1_set_mode(b_end->dc_object, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
        }
    }

    wl_surface_commit(b_end->surface);
    wl_display_roundtrip(b_end->display);
}

void wayland_window_show(void* backend)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;

    _create_shm_buffer(b_end, b_end->width, b_end->height);
    if (!b_end->shm_buffer) { return; }

    wl_surface_attach(b_end->surface, b_end->shm_buffer, 0, 0);

    wl_surface_damage_buffer(b_end->surface, 0, 0, b_end->width, b_end->height);

    wl_surface_commit(b_end->surface);
    wl_display_roundtrip(b_end->display);
}

void wayland_window_destroy(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window Destroyed."); }

void wayland_window_set_event_callback(void* backend, EventCallbackFn event_callback)
{ ANVIL_CORE_TRACE("Wayland Window EventCallback set."); }

static void wayland_events_poll_and_dispatch(void* backend)
{ ANVIL_CORE_TRACE("Wayland Window polling events."); }

static void _create_shm_buffer(WaylandBackend* backend, int32 width, int32 height)
{
    if (backend->shm_data != MAP_FAILED && backend->shm_fd >= 0) { return; }

    int32 buffer_size = width * height * sizeof(uint32);

    if (buffer_size <= 0) { return; }

    backend->shm_fd = memfd_create("FORGE_WM_BUFFER", MFD_CLOEXEC | MFD_ALLOW_SEALING);

    ftruncate(backend->shm_fd, buffer_size);

    backend->shm_data = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, backend->shm_fd, 0);
    if (backend->shm_data == MAP_FAILED)
    {
        close(backend->shm_fd);
        return;
    }

    uint32* pixels = (uint32*)backend->shm_data;
    for (uint32 i = 0; i < width * height; ++i)
    {
        pixels[i] = 0xFF000000u;
    }

    if (!backend->shared_mem) { return; }

    struct wl_shm_pool* shm_pool = wl_shm_create_pool(backend->shared_mem, backend->shm_fd, buffer_size);

    backend->shm_buffer = wl_shm_pool_create_buffer(shm_pool, 0, width, height, width * 4, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(shm_pool);
}

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
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        b_end->shared_mem = wl_registry_bind(b_end->registry, id, &wl_shm_interface, version);
    }
    else if (strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
    {
        b_end->dc_manager = wl_registry_bind(b_end->registry, id, &zxdg_decoration_manager_v1_interface, version);
    }
}

// clang-format off
static void _on_xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32 serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

static void _on_xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32 serial)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    xdg_surface_set_window_geometry(b_end->xdg_surface, 0, 0, b_end->width, b_end->height);

    xdg_surface_ack_configure(xdg_surface, serial);
}
// clang-format on
