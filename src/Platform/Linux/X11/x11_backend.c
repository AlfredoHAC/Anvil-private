#include "anvlpch.h"

#include "Platform/Linux/X11/x11_backend.h"
#include "Platform/event.h"
#include "Tools/logger.h"

#include <xcb/xcb.h>
#include <xcb/xproto.h>

typedef struct X11Backend
{
    // XCB Connection
    struct xcb_connection_t* display;

    // XCB Screen
    const xcb_setup_t* setup;
    xcb_screen_t*      screen;

    // XCB Window
    xcb_window_t window_id;

    // Event callback
    EventCallbackFn event_callback;

    // XCB Window Close event
    xcb_atom_t wm_delete_window_atom;
} X11Backend;

static void* x11_backend_init();
static void  x11_backend_shutdown(void* backend);
static void  x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  x11_window_show(void* backend);
static void  x11_window_destroy(void* backend);
static void  x11_window_set_event_callback(void* backend, EventCallbackFn event_callback);

static void x11_events_poll_and_dispatch(void* backend);

static const WindowBackend X11_BACKEND = {
    .backend_init                    = x11_backend_init,
    .backend_shutdown                = x11_backend_shutdown,
    .window_create                   = x11_window_create,
    .window_show                     = x11_window_show,
    .window_destroy                  = x11_window_destroy,
    .window_set_event_callback       = x11_window_set_event_callback,
    .window_events_poll_and_dispatch = x11_events_poll_and_dispatch,
};

const WindowBackend* x11_backend()
{
    //
    return &X11_BACKEND;
}

void* x11_backend_init()
{
    X11Backend* backend_data = malloc(sizeof(X11Backend));

    // XCB Connection start
    backend_data->display = xcb_connect(NULL, NULL);
    if (xcb_connection_has_error(backend_data->display))
    {
        free(backend_data);
        return NULL;
    }

    backend_data->setup                   = xcb_get_setup(backend_data->display);
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(backend_data->setup);
    backend_data->screen                  = screen_iterator.data;
    if (!backend_data->screen)
    {
        xcb_disconnect(backend_data->display);
        free(backend_data);

        return NULL;
    }

    backend_data->wm_delete_window_atom = 0;

    return backend_data;
}

void x11_backend_shutdown(void* backend)
{
    if (!backend) { return; }

    X11Backend* b_end = (X11Backend*)backend;
    xcb_disconnect(b_end->display);

    free(b_end);
}

static void _register_wm_delete_window_message(X11Backend* b_end)
{
    xcb_intern_atom_cookie_t protocols_cookie =
        xcb_intern_atom(b_end->display, 0, strlen("WM_PROTOCOLS"), "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(b_end->display, protocols_cookie, NULL);

    if (!protocols_reply)
    {
        ANVIL_CORE_WARN("WM_PROTOCOLS not supported.");
        return;
    }

    xcb_intern_atom_cookie_t wm_del_cookie =
        xcb_intern_atom(b_end->display, 0, strlen("WM_DELETE_WINDOW"), "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* wm_del_reply = xcb_intern_atom_reply(b_end->display, wm_del_cookie, NULL);

    if (!wm_del_reply)
    {
        ANVIL_CORE_WARN("WM_DELETE_WINDOW event not supported.");
        free(protocols_reply);
        return;
    }

    b_end->wm_delete_window_atom = wm_del_reply->atom;

    xcb_change_property(b_end->display,
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

void x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{
    X11Backend* b_end = (X11Backend*)backend;

    b_end->window_id = xcb_generate_id(b_end->display);

    uint32 mask          = XCB_CW_BACK_PIXEL | XCB_CW_BORDER_PIXEL | XCB_CW_EVENT_MASK;
    uint32 mask_values[] = {
        b_end->screen->black_pixel,
        b_end->screen->white_pixel,
        XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
            XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE |
            XCB_EVENT_MASK_BUTTON_MOTION,
    };

    xcb_create_window(b_end->display,                // XCB connection
                      XCB_COPY_FROM_PARENT,          // Window depth
                      b_end->window_id,              // Window id
                      b_end->screen->root,           // Window parent
                      0,                             // X
                      0,                             // Y
                      width,                         // Width
                      height,                        // Height
                      0,                             // Border width
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, // Window class
                      b_end->screen->root_visual,    // Window Visual
                      mask,                          // Bitmask list
                      mask_values);                  // Mask values (array)

    // Changes window title
    xcb_change_property(b_end->display,
                        XCB_PROP_MODE_REPLACE,
                        b_end->window_id,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(window_title),
                        window_title);

    // Register WM_DELETE (Window Close) event message
    _register_wm_delete_window_message(b_end);
}

void x11_window_show(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    xcb_map_window(b_end->display, b_end->window_id);

    xcb_flush(b_end->display);
}

void x11_window_destroy(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    if (b_end->window_id == 0) { return; }

    xcb_destroy_window(b_end->display, b_end->window_id);
    xcb_flush(b_end->display);
    b_end->window_id = 0;
}

void x11_window_set_event_callback(void* backend, EventCallbackFn event_callback)
{
    X11Backend* b_end = (X11Backend*)backend;

    b_end->event_callback = event_callback;
}

static void _dispatch_x11_messages(X11Backend* b_end, xcb_generic_event_t* xcb_event)
{
    switch (xcb_event->response_type & ~0x80)
    {
        case XCB_CLIENT_MESSAGE:
        {
            xcb_client_message_event_t* client_msg = (xcb_client_message_event_t*)xcb_event;
            if (b_end->wm_delete_window_atom == 0) { break; }

            if (client_msg->data.data32[0] == b_end->wm_delete_window_atom)
            {
                Event event = {
                    .type         = WindowClose,
                    .handled      = false,
                    .window_close = {0},
                };
                b_end->event_callback(event);

                if (!event.handled)
                {
                    xcb_destroy_window(b_end->display, b_end->window_id);
                    b_end->window_id = 0;
                }
            }
            break;
        }
        case XCB_CONFIGURE_NOTIFY:
        {
            xcb_configure_notify_event_t* cfg_notify = (xcb_configure_notify_event_t*)xcb_event;

            if (!(cfg_notify->width == 0) || !(cfg_notify->height == 0))
            {
                Event event = {
                    .type          = WindowResize,
                    .handled       = false,
                    .window_resize = {.width = cfg_notify->width, .height = cfg_notify->height},
                };
                b_end->event_callback(event);
            }

            break;
        }
        case XCB_KEY_PRESS:
        {
            xcb_key_press_event_t* key_press = (xcb_key_press_event_t*)xcb_event;

            Event event = {
                .type      = KeyPress,
                .handled   = false,
                .key_press = {.key_code = key_press->detail, .modifier_set = 0},
            };
            b_end->event_callback(event);

            break;
        }
        case XCB_KEY_RELEASE:
        {
            xcb_key_release_event_t* key_press = (xcb_key_release_event_t*)xcb_event;

            Event event = {
                .type        = KeyRelease,
                .handled     = false,
                .key_release = {.key_code = key_press->detail, .modifier_set = 0},
            };
            b_end->event_callback(event);

            break;
        }
        case XCB_MOTION_NOTIFY:
        {
            xcb_motion_notify_event_t* motion_notify = (xcb_motion_notify_event_t*)xcb_event;

            Event event = {
                .type       = MouseMove,
                .handled    = false,
                .mouse_move = {.x = motion_notify->event_x, .y = motion_notify->event_y},
            };
            b_end->event_callback(event);

            break;
        }
        case XCB_BUTTON_PRESS:
        {
            xcb_button_press_event_t* button_press = (xcb_button_press_event_t*)xcb_event;

            xcb_button_t button = button_press->detail;
            if (button <= XCB_BUTTON_INDEX_3)
            {
                Event event = {
                    .type    = MouseButtonClick,
                    .handled = false,
                    .mouse_button_click =
                        {
                            .button_code  = button,
                            .x            = button_press->event_x,
                            .y            = button_press->event_y,
                            .modifier_set = 0,
                        },
                };

                b_end->event_callback(event);
            }
            else if (button == XCB_BUTTON_INDEX_4)
            {
                Event event = {
                    .type    = MouseScroll,
                    .handled = false,
                    .mouse_scroll =
                        {
                            .x_offset = 0.0f,
                            .y_offset = 1.0f,
                        },
                };

                b_end->event_callback(event);
            }
            else if (button == XCB_BUTTON_INDEX_5)
            {
                Event event = {
                    .type    = MouseScroll,
                    .handled = false,
                    .mouse_scroll =
                        {
                            .x_offset = 0.0f,
                            .y_offset = -1.0f,
                        },
                };

                b_end->event_callback(event);
            }

            break;
        }
        case XCB_BUTTON_RELEASE:
        {
            xcb_button_release_event_t* button_release = (xcb_button_release_event_t*)xcb_event;
            if (button_release->detail > XCB_BUTTON_INDEX_3) { break; }

            Event event = {
                .type    = MouseButtonRelease,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = button_release->detail,
                        .x            = button_release->event_x,
                        .y            = button_release->event_y,
                        .modifier_set = 0,
                    },
            };

            b_end->event_callback(event);

            break;
        }
    }

    free(xcb_event);
}

void x11_events_poll_and_dispatch(void* backend)
{
    X11Backend* b_end = (X11Backend*)backend;

    xcb_generic_event_t* xcb_event;

    while ((xcb_event = xcb_poll_for_event(b_end->display)))
    {
        _dispatch_x11_messages(b_end, xcb_event);
    }
}
