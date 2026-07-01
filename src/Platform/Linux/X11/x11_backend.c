#include "anvlpch.h"

#include "Platform/Linux/X11/x11_backend.h"

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

} X11Backend;

static void* x11_backend_init();
static void  x11_backend_shutdown(void* backend);
static void  x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  x11_window_show(void* backend);
static void  x11_window_destroy(void* backend);

static const WindowBackend X11_BACKEND = {
    .backend_init     = x11_backend_init,
    .backend_shutdown = x11_backend_shutdown,
    .window_create    = x11_window_create,
    .window_show      = x11_window_show,
    .window_destroy   = x11_window_destroy,
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

    return backend_data;
}

void x11_backend_shutdown(void* backend)
{
    if (!backend) { return; }

    X11Backend* b_end = (X11Backend*)backend;
    xcb_disconnect(b_end->display);

    free(b_end);
}

void x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{
    X11Backend* b_end = (X11Backend*)backend;

    b_end->window_id = xcb_generate_id(b_end->display);

    xcb_create_window(b_end->display,                // XCB connection
                      XCB_COPY_FROM_PARENT,          // Window depth
                      b_end->window_id,              // Window id
                      b_end->screen->root,           // Window parent
                      0,                             // X
                      0,                             // Y
                      width,                         // Width
                      height,                        // Height
                      1,                             // Border width
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, // Window class
                      b_end->screen->root_visual,    // Window Visual
                      0,                             // Bitmask list
                      NULL);                         //

    // Changes window title
    xcb_change_property(b_end->display,
                        XCB_PROP_MODE_REPLACE,
                        b_end->window_id,
                        XCB_ATOM_WM_NAME,
                        XCB_ATOM_STRING,
                        8,
                        strlen(window_title),
                        window_title);
}

void x11_window_show(void* backend)
{ ANVIL_CORE_TRACE("X11 Window Showed."); }

void x11_window_destroy(void* backend)
{ ANVIL_CORE_TRACE("X11 Window Destroyed."); }
