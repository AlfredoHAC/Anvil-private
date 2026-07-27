#include "anvlpch.h"

#include "Platform/Linux/Wayland/wayland_backend.h"
#include "Platform/Linux/Wayland/xdg_shell_client_protocol.h"
#include "Platform/Linux/Wayland/xdg_shell_decoration_protocol.h"
#include "Platform/event.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <wayland-client.h>
#include <wayland-util.h>

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
    struct wl_seat*                    seat;

    // Configure data
    const char* title;
    uint32      width;
    uint32      height;

    // Shared Memory (Pixel Buffer) data
    int32             shm_fd;
    void*             shm_data;
    struct wl_buffer* shm_buffer;
    uint32            buffer_width;
    uint32            buffer_height;

    // Decoration data
    struct zxdg_toplevel_decoration_v1* dc_object;

    // Event devices
    struct wl_keyboard* keyboard;
    struct wl_pointer*  pointer;

    // Event devices data
    uint32  modifier_state;
    float32 pointer_x;
    float32 pointer_y;

    // Event capturing data
    uint32 capabilities;

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

static void _shm_buffer_create(WaylandBackend* b_end, int32 width, int32 height);
static void _shm_buffer_destroy(WaylandBackend* b_end);

static void _on_wl_registry_global_notify(
    void* data, struct wl_registry* registry, uint32 id, const char* interface, uint32 version);
static void _on_xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32 serial);
static void _on_xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32 serial);

static void _on_wl_seat_capabilities(void* data, struct wl_seat* seat, uint32 capabilities);
static void _on_wl_seat_name_noop(void* data, struct wl_seat* seat, const char* name);
static void _on_xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel);
static void _on_xdg_toplevel_configure(
    void* data, struct xdg_toplevel* xdg_toplevel, int32 width, int32 height, struct wl_array* states);
static void _on_xdg_toplevel_configure_bounds_noop(void*                data,
                                                   struct xdg_toplevel* xdg_toplevel,
                                                   int32                width,
                                                   int32                height);
static void _on_xdg_toplevel_wm_capabilities_noop(void*                data,
                                                  struct xdg_toplevel* xdg_toplevel,
                                                  struct wl_array*     capabilities);
static void _on_wl_keyboard_keymap_noop(
    void* data, struct wl_keyboard* wl_keyboard, uint32 format, int32 fd, uint32 size);
static void _on_wl_keyboard_enter_noop(
    void* data, struct wl_keyboard* wl_keyboard, uint32 serial, struct wl_surface* surface, struct wl_array* keys);
static void _on_wl_keyboard_leave_noop(void*               data,
                                       struct wl_keyboard* wl_keyboard,
                                       uint32              serial,
                                       struct wl_surface*  surface);
static void _on_wl_keyboard_key(
    void* data, struct wl_keyboard* wl_keyboard, uint32 serial, uint32 time, uint32 key, uint32 state);
static void _on_wl_keyboard_modifier(void*               data,
                                     struct wl_keyboard* wl_keyboard,
                                     uint32              serial,
                                     uint32              mods_depressed,
                                     uint32              mods_latched,
                                     uint32              mods_locked,
                                     uint32              group);
static void _on_wl_keyboard_repeat_info_noop(void* data, struct wl_keyboard* wl_keyboard, int32 rate, int32 delay);
static void _on_wl_pointer_enter_noop(void*              data,
                                      struct wl_pointer* wl_pointer,
                                      uint32             serial,
                                      struct wl_surface* surface,
                                      wl_fixed_t         surface_x,
                                      wl_fixed_t         surface_y);
static void _on_wl_pointer_leave_noop(void*              data,
                                      struct wl_pointer* wl_pointer,
                                      uint32             serial,
                                      struct wl_surface* surface);
static void _on_wl_pointer_motion(
    void* data, struct wl_pointer* wl_pointer, uint32 time, wl_fixed_t surface_x, wl_fixed_t surface_y);
static void _on_wl_pointer_button(
    void* data, struct wl_pointer* wl_pointer, uint32 serial, uint32 time, uint32 button, uint32 state);
static void _on_wl_pointer_axis(void* data, struct wl_pointer* wl_pointer, uint32 time, uint32 axis, wl_fixed_t value);
static void _on_wl_pointer_frame_noop(void* data, struct wl_pointer* wl_pointer);
static void _on_wl_pointer_axis_source_noop(void* data, struct wl_pointer* wl_pointer, uint32 axis_source);
static void _on_wl_pointer_axis_stop_noop(void* data, struct wl_pointer* wl_pointer, uint32 time, uint32 axis);
static void _on_wl_pointer_axis_relative_direction_noop(void*              data,
                                                        struct wl_pointer* wl_pointer,
                                                        uint32             axis,
                                                        uint32             direction);

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

static const struct wl_seat_listener WL_SEAT_LISTENER = {
    .capabilities = _on_wl_seat_capabilities,
    .name         = _on_wl_seat_name_noop,
};

static const struct xdg_toplevel_listener XDG_TOPLEVEL_LISTENER = {
    .close            = _on_xdg_toplevel_close,
    .configure        = _on_xdg_toplevel_configure,
    .configure_bounds = _on_xdg_toplevel_configure_bounds_noop,
    .wm_capabilities  = _on_xdg_toplevel_wm_capabilities_noop,
};

static const struct wl_keyboard_listener WL_KEYBOARD_LISTENER = {
    .keymap      = _on_wl_keyboard_keymap_noop,
    .enter       = _on_wl_keyboard_enter_noop,
    .leave       = _on_wl_keyboard_leave_noop,
    .key         = _on_wl_keyboard_key,
    .modifiers   = _on_wl_keyboard_modifier,
    .repeat_info = _on_wl_keyboard_repeat_info_noop,
};

static const struct wl_pointer_listener WL_POINTER_LISTENER = {
    .enter                   = _on_wl_pointer_enter_noop,
    .leave                   = _on_wl_pointer_leave_noop,
    .motion                  = _on_wl_pointer_motion,
    .button                  = _on_wl_pointer_button,
    .axis                    = _on_wl_pointer_axis,
    .frame                   = _on_wl_pointer_frame_noop,
    .axis_source             = _on_wl_pointer_axis_source_noop,
    .axis_stop               = _on_wl_pointer_axis_stop_noop,
    .axis_relative_direction = _on_wl_pointer_axis_relative_direction_noop,
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
    memset(backend_data, 0, sizeof(WaylandBackend));
    backend_data->shm_fd = -1;

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

    WaylandBackend* b_end = (WaylandBackend*)backend;

    wl_keyboard_destroy(b_end->keyboard);
    wl_pointer_destroy(b_end->pointer);
    wl_seat_destroy(b_end->seat);
    zxdg_decoration_manager_v1_destroy(b_end->dc_manager);
    wl_shm_destroy(b_end->shared_mem);
    xdg_wm_base_destroy(b_end->wm_base);
    wl_compositor_destroy(b_end->compositor);

    if (b_end->registry) { wl_registry_destroy(b_end->registry); }
    if (b_end->display) { wl_display_disconnect(b_end->display); }

    free(b_end);
}

void wayland_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;
    b_end->height         = height;
    b_end->width          = width;

    b_end->surface     = wl_compositor_create_surface(b_end->compositor);
    b_end->xdg_surface = xdg_wm_base_get_xdg_surface(b_end->wm_base, b_end->surface);
    xdg_surface_add_listener(b_end->xdg_surface, &XDG_SURFACE_LISTENER, (void*)b_end);

    b_end->top_level = xdg_surface_get_toplevel(b_end->xdg_surface);
    xdg_toplevel_set_app_id(b_end->top_level, window_title);
    xdg_toplevel_set_title(b_end->top_level, window_title);
    xdg_toplevel_set_min_size(b_end->top_level, width, height);
    xdg_toplevel_add_listener(b_end->top_level, &XDG_TOPLEVEL_LISTENER, (void*)b_end);

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

    _shm_buffer_create(b_end, b_end->width, b_end->height);
    if (!b_end->shm_buffer) { return; }

    wl_surface_attach(b_end->surface, b_end->shm_buffer, 0, 0);

    wl_surface_damage_buffer(b_end->surface, 0, 0, b_end->width, b_end->height);

    wl_surface_commit(b_end->surface);
}

void wayland_window_destroy(void* backend)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;

    if (b_end->dc_object) { zxdg_toplevel_decoration_v1_destroy(b_end->dc_object); }

    _shm_buffer_destroy(b_end);

    xdg_toplevel_destroy(b_end->top_level);
    xdg_surface_destroy(b_end->xdg_surface);

    if (b_end->surface)
    {
        wl_surface_destroy(b_end->surface);
        b_end->surface = NULL;
    }
}

void wayland_window_set_event_callback(void* backend, EventCallbackFn event_callback)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;

    b_end->event_callback = event_callback;
}

static void wayland_events_poll_and_dispatch(void* backend)
{
    WaylandBackend* b_end = (WaylandBackend*)backend;

    wl_display_dispatch(b_end->display);
}

static void _shm_buffer_create(WaylandBackend* b_end, int32 width, int32 height)
{
    if (b_end->shm_data && b_end->shm_fd >= 0) { return; }

    b_end->buffer_width  = width;
    b_end->buffer_height = height;
    int32 buffer_size    = width * height * sizeof(uint32);

    if (buffer_size <= 0) { return; }

    b_end->shm_fd = memfd_create("FORGE_WM_BUFFER", MFD_CLOEXEC | MFD_ALLOW_SEALING);

    ftruncate(b_end->shm_fd, buffer_size);

    b_end->shm_data = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, b_end->shm_fd, 0);
    if (b_end->shm_data == MAP_FAILED)
    {
        close(b_end->shm_fd);
        return;
    }

    uint32* pixels = (uint32*)b_end->shm_data;
    for (uint32 i = 0; i < width * height; ++i)
    {
        pixels[i] = 0xFF000000u;
    }

    if (!b_end->shared_mem)
    {
        munmap(b_end->shm_data, buffer_size);
        close(b_end->shm_fd);
        return;
    }

    struct wl_shm_pool* shm_pool = wl_shm_create_pool(b_end->shared_mem, b_end->shm_fd, buffer_size);

    b_end->shm_buffer = wl_shm_pool_create_buffer(shm_pool, 0, width, height, width * 4, WL_SHM_FORMAT_ARGB8888);

    wl_shm_pool_destroy(shm_pool);
}

static void _shm_buffer_destroy(WaylandBackend* b_end)
{
    uint64 buffer_size = b_end->buffer_width * b_end->buffer_height * sizeof(uint32);

    if (b_end->shm_buffer)
    {
        wl_buffer_destroy(b_end->shm_buffer);
        b_end->shm_buffer = NULL;
    }
    if (b_end->shm_data != NULL && b_end->shm_fd >= 0)
    {
        munmap(b_end->shm_data, buffer_size);
        close(b_end->shm_fd);

        b_end->shm_data = NULL;
        b_end->shm_fd   = -1;
    }
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
    else if (strcmp(interface, wl_seat_interface.name) == 0)
    {
        b_end->seat = wl_registry_bind(b_end->registry, id, &wl_seat_interface, version);
        wl_seat_add_listener(b_end->seat, &WL_SEAT_LISTENER, (void*)b_end);
    }
}

// clang-format off
static void _on_xdg_wm_base_ping(void* data, struct xdg_wm_base* xdg_wm_base, uint32 serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}
// clang-format on

static void _on_xdg_surface_configure(void* data, struct xdg_surface* xdg_surface, uint32 serial)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    xdg_surface_set_window_geometry(b_end->xdg_surface, 0, 0, b_end->width, b_end->height);

    xdg_surface_ack_configure(xdg_surface, serial);

    if (b_end->shm_buffer)
    {
        _shm_buffer_destroy(b_end);
        wayland_window_show(b_end);
    }
}

static void _on_wl_seat_capabilities(void* data, struct wl_seat* seat, uint32 capabilities)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD)
    {
        if (!b_end->keyboard)
        {
            b_end->keyboard = wl_seat_get_keyboard(seat);
            wl_keyboard_add_listener(b_end->keyboard, &WL_KEYBOARD_LISTENER, (void*)b_end);
        }
    }
    if (capabilities & WL_SEAT_CAPABILITY_POINTER)
    {
        if (!b_end->pointer)
        {
            b_end->pointer = wl_seat_get_pointer(seat);
            wl_pointer_add_listener(b_end->pointer, &WL_POINTER_LISTENER, (void*)b_end);
        }
    }
}

static void _on_wl_seat_name_noop(void* data, struct wl_seat* seat, const char* name)
{
}

static void _on_xdg_toplevel_close(void* data, struct xdg_toplevel* xdg_toplevel)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    Event event = {
        .type         = WindowClose,
        .handled      = false,
        .window_close = {0},
    };
    b_end->event_callback(event);
}

static void _on_xdg_toplevel_configure(
    void* data, struct xdg_toplevel* xdg_toplevel, int32 width, int32 height, struct wl_array* states)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    if (width > 0 && height > 0)
    {
        b_end->width  = width;
        b_end->height = height;

        Event event = {
            .type          = WindowResize,
            .handled       = false,
            .window_resize = {.width = width, .height = height},
        };
        b_end->event_callback(event);
    }
}

static void _on_xdg_toplevel_configure_bounds_noop(void*                data,
                                                   struct xdg_toplevel* xdg_toplevel,
                                                   int32                width,
                                                   int32                height)
{
}

static void _on_xdg_toplevel_wm_capabilities_noop(void*                data,
                                                  struct xdg_toplevel* xdg_toplevel,
                                                  struct wl_array*     capabilities)
{
}

static void _on_wl_keyboard_keymap_noop(
    void* data, struct wl_keyboard* wl_keyboard, uint32 format, int32 fd, uint32 size)
{
}

static void _on_wl_keyboard_enter_noop(
    void* data, struct wl_keyboard* wl_keyboard, uint32 serial, struct wl_surface* surface, struct wl_array* keys)
{
}

static void _on_wl_keyboard_leave_noop(void*               data,
                                       struct wl_keyboard* wl_keyboard,
                                       uint32              serial,
                                       struct wl_surface*  surface)
{
}

static void _on_wl_keyboard_key(
    void* data, struct wl_keyboard* wl_keyboard, uint32 serial, uint32 time, uint32 key, uint32 state)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    Event event   = {0};
    event.handled = false;
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED)
    {
        event.type                   = KeyPress;
        event.key_press.key_code     = key;
        event.key_press.modifier_set = b_end->modifier_state;
    }
    else if (state == WL_KEYBOARD_KEY_STATE_RELEASED)
    {
        event.type                     = KeyRelease;
        event.key_release.key_code     = key;
        event.key_release.modifier_set = b_end->modifier_state;
    }

    b_end->event_callback(event);
}

static void _on_wl_keyboard_modifier(void*               data,
                                     struct wl_keyboard* wl_keyboard,
                                     uint32              serial,
                                     uint32              mods_depressed,
                                     uint32              mods_latched,
                                     uint32              mods_locked,
                                     uint32              group)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    b_end->modifier_state = mods_depressed;
}

static void _on_wl_keyboard_repeat_info_noop(void* data, struct wl_keyboard* wl_keyboard, int32 rate, int32 delay)
{
}

static void _on_wl_pointer_enter_noop(void*              data,
                                      struct wl_pointer* wl_pointer,
                                      uint32             serial,
                                      struct wl_surface* surface,
                                      wl_fixed_t         surface_x,
                                      wl_fixed_t         surface_y)
{
}

static void _on_wl_pointer_leave_noop(void*              data,
                                      struct wl_pointer* wl_pointer,
                                      uint32             serial,
                                      struct wl_surface* surface)
{
}

static void _on_wl_pointer_motion(
    void* data, struct wl_pointer* wl_pointer, uint32 time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    b_end->pointer_x = (float32)wl_fixed_to_double(surface_x);
    b_end->pointer_y = (float32)wl_fixed_to_double(surface_y);

    Event event = {
        .type    = MouseMove,
        .handled = false,
        .mouse_move =
            {
                .x = b_end->pointer_x,
                .y = b_end->pointer_y,
            },
    };
    b_end->event_callback(event);
}

static void _on_wl_pointer_button(
    void* data, struct wl_pointer* wl_pointer, uint32 serial, uint32 time, uint32 button, uint32 state)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    Event event   = {0};
    event.handled = false;
    if (state == WL_POINTER_BUTTON_STATE_PRESSED)
    {
        event.type                           = MouseButtonClick;
        event.mouse_button_click.x           = b_end->pointer_x;
        event.mouse_button_click.y           = b_end->pointer_y;
        event.mouse_button_click.button_code = button;
    }
    else if (state == WL_POINTER_BUTTON_STATE_RELEASED)
    {
        event.type                             = MouseButtonRelease;
        event.mouse_button_release.x           = b_end->pointer_x;
        event.mouse_button_release.y           = b_end->pointer_y;
        event.mouse_button_release.button_code = button;
    }

    b_end->event_callback(event);
}

static void _on_wl_pointer_axis(void* data, struct wl_pointer* wl_pointer, uint32 time, uint32 axis, wl_fixed_t value)
{
    WaylandBackend* b_end = (WaylandBackend*)data;

    Event event   = {0};
    event.handled = false;
    event.type    = MouseScroll;

    if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
    {
        float32 y_offset            = (float32)wl_fixed_to_double(value) > 0 ? -1.0f : 1.0f;
        event.mouse_scroll.y_offset = y_offset;
    }
    else if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
    {
        float32 x_offset            = (float32)wl_fixed_to_double(value) > 0 ? 1.0f : -1.0f;
        event.mouse_scroll.x_offset = x_offset;
    }

    b_end->event_callback(event);
}

static void _on_wl_pointer_frame_noop(void* data, struct wl_pointer* wl_pointer)
{
}

static void _on_wl_pointer_axis_source_noop(void* data, struct wl_pointer* wl_pointer, uint32 axis_source)
{
}

static void _on_wl_pointer_axis_relative_direction_noop(void*              data,
                                                        struct wl_pointer* wl_pointer,
                                                        uint32             axis,
                                                        uint32             direction)
{
}

static void _on_wl_pointer_axis_stop_noop(void* data, struct wl_pointer* wl_pointer, uint32 time, uint32 axis)
{
}
