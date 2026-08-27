#include "anvlpch.h"

#include "Tools/logger.h"
#include "Window/event.h"
#include "Windowing/Linux/X11/glx_context.h"
#include "Windowing/Linux/X11/x11_backend.h"

#include <X11/Xlib-xcb.h>

typedef struct X11Backend
{
    // Xlib display
    Display* x11_display;

    // XCB display/connection
    struct xcb_connection_t* xcb_display;
    xcb_screen_t*            screen;
    xcb_colormap_t           colormap_id;
    xcb_window_t             window_id;

    AnvlGLXGraphicsContext context;

    uint16 width;
    uint16 height;

    EventCallbackFn event_callback;

    // XCB Window Close event
    xcb_atom_t wm_delete_window_atom;
} X11Backend;

static void*  x11_backend_init();
static void   x11_backend_shutdown(void* backend);
static void   x11_window_create(void*                   backend,
                                const AnvlWindowOptions window_options);
static void   x11_window_show(void* backend);
static void   x11_window_destroy(void* backend);
static void   x11_window_set_event_callback(void*           backend,
                                            EventCallbackFn event_callback);
static void   x11_events_poll_and_dispatch(void* backend);
static void*  x11_window_get_handle(void* backend);
static uint16 x11_window_get_width(void* backend);
static uint16 x11_window_get_height(void* backend);
static void   x11_window_present(void* backend);

static void _dispatch_x11_messages(X11Backend*          b_end,
                                   xcb_generic_event_t* xcb_event);

static const AnvlWindowBackend X11_BACKEND = {
    .backend_init                    = x11_backend_init,
    .backend_shutdown                = x11_backend_shutdown,
    .window_create                   = x11_window_create,
    .window_show                     = x11_window_show,
    .window_destroy                  = x11_window_destroy,
    .window_set_event_callback       = x11_window_set_event_callback,
    .window_events_poll_and_dispatch = x11_events_poll_and_dispatch,
    .window_get_handle               = x11_window_get_handle,
    .window_present                  = x11_window_present,
    .window_get_width                = x11_window_get_width,
    .window_get_height               = x11_window_get_height,
};

// clang-format off
const AnvlWindowBackend* x11_backend()
{
    return &X11_BACKEND;
}
// clang-format on

void* x11_backend_init()
{
    X11Backend* backend_data = malloc(sizeof(X11Backend));
    memset(backend_data, 0, sizeof(X11Backend));

    backend_data->x11_display = XOpenDisplay(NULL);
    if (!backend_data->x11_display)
    {
        ANVIL_CORE_ERROR("Failed to open Xlib display.");
        free(backend_data);
        return NULL;
    }

    backend_data->xcb_display = XGetXCBConnection(backend_data->x11_display);
    if (!backend_data->xcb_display)
    {
        ANVIL_CORE_ERROR("Failed to get XCB connection from Xlib display.");
        XCloseDisplay(backend_data->x11_display);
        free(backend_data);
        return NULL;
    }

    const xcb_setup_t*    setup = xcb_get_setup(backend_data->xcb_display);
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);
    backend_data->screen                  = screen_iterator.data;
    if (!backend_data->screen)
    {
        xcb_disconnect(backend_data->xcb_display);
        free(backend_data);

        return NULL;
    }

    backend_data->wm_delete_window_atom = 0;

    return backend_data;
}

void x11_backend_shutdown(void* backend)
{
    ANVIL_ASSERT(backend != NULL);

    X11Backend* b_end = (X11Backend*)backend;

    XCloseDisplay(b_end->x11_display);

    memset(b_end, 0, sizeof(X11Backend));
    free(b_end);
}

static void _register_wm_delete_window_message(X11Backend* b_end)
{
    xcb_intern_atom_cookie_t protocols_cookie =
        xcb_intern_atom(b_end->xcb_display,
                        0,
                        strlen("WM_PROTOCOLS"),
                        "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* protocols_reply =
        xcb_intern_atom_reply(b_end->xcb_display, protocols_cookie, NULL);

    if (!protocols_reply)
    {
        ANVIL_CORE_WARN("WM_PROTOCOLS not supported.");
        return;
    }

    xcb_intern_atom_cookie_t wm_del_cookie =
        xcb_intern_atom(b_end->xcb_display,
                        0,
                        strlen("WM_DELETE_WINDOW"),
                        "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* wm_del_reply =
        xcb_intern_atom_reply(b_end->xcb_display, wm_del_cookie, NULL);

    if (!wm_del_reply)
    {
        ANVIL_CORE_WARN("WM_DELETE_WINDOW event not supported.");
        free(protocols_reply);
        return;
    }

    b_end->wm_delete_window_atom = wm_del_reply->atom;

    xcb_change_property(b_end->xcb_display,
                        XCB_PROP_MODE_REPLACE,
                        b_end->window_id,
                        protocols_reply->atom,
                        XCB_ATOM_ATOM,
                        32,
                        1,
                        &(wm_del_reply->atom));

    free(wm_del_reply);
    free(protocols_reply);
}

static void x11_window_create(void*                   backend,
                              const AnvlWindowOptions window_options)
{
    X11Backend* b_end = (X11Backend*)backend;

    b_end->window_id = xcb_generate_id(b_end->xcb_display);

    if (window_options.graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_OPENGL)
    {
        bool glx_extensions_loaded =
            glx_context_load_extensions(b_end->x11_display);

        b_end->context.fbconfig =
            glx_context_choose_fbconfig(b_end->x11_display,
                                        window_options.graphics_requirements);

        if (b_end->context.fbconfig)
        {
            b_end->context.visual =
                glx_context_get_visual_info(b_end->x11_display,
                                            b_end->context.fbconfig);
        }

        if (!glx_extensions_loaded || !b_end->context.fbconfig ||
            !b_end->context.visual)
        {
            ANVIL_CORE_WARN("Failed to create Window in OpenGL graphics mode.");
            ANVIL_CORE_WARN("-> Falling back to default Window.");

            b_end->context.fbconfig = NULL;
            b_end->context.visual   = NULL;
        }
    }

    xcb_visualid_t window_visual = b_end->context.visual
                                       ? b_end->context.visual->visualid
                                       : b_end->screen->root_visual;
    uint8 window_depth = b_end->context.visual ? b_end->context.visual->depth
                                               : b_end->screen->root_depth;

    b_end->colormap_id = xcb_generate_id(b_end->xcb_display);
    xcb_create_colormap(b_end->xcb_display,
                        XCB_COLORMAP_ALLOC_NONE,
                        b_end->colormap_id,
                        b_end->screen->root,
                        window_visual);
    xcb_flush(b_end->xcb_display);

    uint32 mask = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK |
                  XCB_CW_COLORMAP;
    uint32 mask_values[] = {
        b_end->screen->black_pixel,
        b_end->screen->white_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS |
            XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
            XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_BUTTON_MOTION,
        b_end->colormap_id,
    };

    xcb_create_window(b_end->xcb_display,            // XCB connection
                      window_depth,                  // Window depth
                      b_end->window_id,              // Window id
                      b_end->screen->root,           // Window parent
                      0,                             // X
                      0,                             // Y
                      window_options.width,          // Width
                      window_options.height,         // Height
                      1,                             // Border width
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, // Window class
                      window_visual,                 // Window Visual
                      mask,                          // Bitmask list
                      mask_values);                  // Mask values (array)

    b_end->width  = window_options.width;
    b_end->height = window_options.height;

    if (b_end->context.fbconfig && b_end->context.visual &&
        window_options.graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_OPENGL)
    {
        b_end->context.handle = glx_context_create(
            b_end->x11_display,
            &b_end->context,
            window_options.graphics_requirements.major_version,
            window_options.graphics_requirements.minor_version);

        if (b_end->context.handle)
        {
            b_end->context.glx_window =
                glx_context_make_current(b_end->x11_display,
                                         b_end->window_id,
                                         &b_end->context);
        }

        if (!b_end->context.handle || !b_end->context.glx_window)
        {
            ANVIL_CORE_WARN("Failed to create Window in OpenGL graphics mode.");
            ANVIL_CORE_WARN("-> Falling back to default Window.");
        }
    }

    // Changes window title
    xcb_change_property(b_end->xcb_display,
                        XCB_PROP_MODE_REPLACE,
                        b_end->window_id,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(window_options.title),
                        window_options.title);

    // Register WM_DELETE (Window Close) event message
    _register_wm_delete_window_message(b_end);

    xcb_flush(b_end->xcb_display);
}

void x11_window_show(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    xcb_map_window(b_end->xcb_display, b_end->window_id);

    xcb_flush(b_end->xcb_display);
}

void x11_window_destroy(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    if (b_end->window_id == 0) { return; }

    if (b_end->context.handle)
    {
        glx_context_destroy(b_end->x11_display, &b_end->context);
    }

    xcb_destroy_window(b_end->xcb_display, b_end->window_id);
    b_end->window_id = 0;

    if (b_end->colormap_id)
    {
        xcb_free_colormap(b_end->xcb_display, b_end->colormap_id);
        b_end->colormap_id = 0;
    }

    xcb_flush(b_end->xcb_display);
}

void x11_window_set_event_callback(void*           backend,
                                   EventCallbackFn event_callback)
{
    X11Backend* b_end = (X11Backend*)backend;

    b_end->event_callback = event_callback;
}

void x11_events_poll_and_dispatch(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    xcb_generic_event_t* xcb_event;

    while ((xcb_event = xcb_poll_for_event(b_end->xcb_display)))
    {
        _dispatch_x11_messages(b_end, xcb_event);
    }
}

void* x11_window_get_handle(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    return (void*)&(b_end->window_id);
}

static uint16 x11_window_get_width(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    return b_end->width;
}

static uint16 x11_window_get_height(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    return b_end->height;
}

static void x11_window_present(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    if (!b_end->context.handle) { return; }

    glx_context_swap_buffers(b_end->x11_display, b_end->context.glx_window);
}

static void _dispatch_x11_messages(X11Backend*          b_end,
                                   xcb_generic_event_t* xcb_event)
{
    switch (xcb_event->response_type & ~0x80)
    {
        case XCB_CLIENT_MESSAGE:
        {
            xcb_client_message_event_t* client_msg =
                (xcb_client_message_event_t*)xcb_event;
            if (b_end->wm_delete_window_atom == 0) { break; }

            if (client_msg->data.data32[0] == b_end->wm_delete_window_atom)
            {
                AnvlEvent event = {
                    .type         = ANVL_EVENT_TYPE_WINDOW_CLOSE,
                    .handled      = false,
                    .window_close = {0},
                };
                b_end->event_callback(&event);

                if (!event.handled) { x11_window_destroy(b_end); }
            }
            break;
        }
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t* cfg_notify =
                (xcb_configure_notify_event_t*)xcb_event;

            uint16 width  = cfg_notify->width;
            uint16 height = cfg_notify->height;

            if (!(width == 0) || !(height == 0))
            {
                AnvlEvent event = {
                    .type          = ANVL_EVENT_TYPE_WINDOW_RESIZE,
                    .handled       = false,
                    .window_resize = {.width = width, .height = height},
                };
                b_end->event_callback(&event);

                b_end->width  = width;
                b_end->height = height;
            }

            break;
        }
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t* key_press =
                (xcb_key_press_event_t*)xcb_event;

            AnvlEvent event = {
                .type      = ANVL_EVENT_TYPE_KEY_PRESS,
                .handled   = false,
                .key_press = {.key_code = key_press->detail, .modifier_set = 0},
            };
            b_end->event_callback(&event);

            break;
        }
        case XCB_KEY_RELEASE:
        {
            xcb_key_release_event_t* key_press =
                (xcb_key_release_event_t*)xcb_event;

            AnvlEvent event = {
                .type        = ANVL_EVENT_TYPE_KEY_RELEASE,
                .handled     = false,
                .key_release = {.key_code     = key_press->detail,
                                .modifier_set = 0},
            };
            b_end->event_callback(&event);

            break;
        }
        case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t* motion_notify =
                (xcb_motion_notify_event_t*)xcb_event;

            AnvlEvent event = {
                .type       = ANVL_EVENT_TYPE_MOUSE_MOVE,
                .handled    = false,
                .mouse_move = {.x = motion_notify->event_x,
                               .y = motion_notify->event_y},
            };
            b_end->event_callback(&event);

            break;
        }
        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t* button_press =
                (xcb_button_press_event_t*)xcb_event;

            xcb_button_t button = button_press->detail;
            if (button <= XCB_BUTTON_INDEX_3)
            {
                AnvlEvent event = {
                    .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
                    .handled = false,
                    .mouse_button_click =
                        {
                            .button_code  = button,
                            .x            = button_press->event_x,
                            .y            = button_press->event_y,
                            .modifier_set = 0,
                        },
                };

                b_end->event_callback(&event);
            }
            else if (button == XCB_BUTTON_INDEX_4)
            {
                AnvlEvent event = {
                    .type    = ANVL_EVENT_TYPE_MOUSE_SCROLL,
                    .handled = false,
                    .mouse_scroll =
                        {
                            .x_offset = 0.0f,
                            .y_offset = 1.0f,
                        },
                };

                b_end->event_callback(&event);
            }
            else if (button == XCB_BUTTON_INDEX_5)
            {
                AnvlEvent event = {
                    .type    = ANVL_EVENT_TYPE_MOUSE_SCROLL,
                    .handled = false,
                    .mouse_scroll =
                        {
                            .x_offset = 0.0f,
                            .y_offset = -1.0f,
                        },
                };

                b_end->event_callback(&event);
            }

            break;
        }
        case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t* button_release =
                (xcb_button_release_event_t*)xcb_event;
            if (button_release->detail > XCB_BUTTON_INDEX_3) { break; }

            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = button_release->detail,
                        .x            = button_release->event_x,
                        .y            = button_release->event_y,
                        .modifier_set = 0,
                    },
            };

            b_end->event_callback(&event);

            break;
        }
    }

    free(xcb_event);
}
