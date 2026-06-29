#include "anvlpch.h"

#include "Platform/Linux/X11/x11_backend.h"

#include <xcb/xcb.h>

typedef struct X11Backend
{
    struct xcb_connection_t* display;
} X11Backend;

static void* x11_backend_init();
static void  x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height);
static void  x11_window_show(void* backend);
static void  x11_window_destroy(void* backend);

static const WindowBackend X11_BACKEND = {
    .backend_init   = x11_backend_init,
    .window_create  = x11_window_create,
    .window_show    = x11_window_show,
    .window_destroy = x11_window_destroy,
};

const WindowBackend* x11_backend()
{
    //
    return &X11_BACKEND;
}

void* x11_backend_init()
{
    X11Backend* backend_data = malloc(sizeof(X11Backend));

    ANVIL_CORE_TRACE("X11 Backend initialized.");

    return backend_data;
}

void x11_window_create(void* backend, const char* window_title, uint16 width, uint16 height)
{ ANVIL_CORE_TRACE("X11 Window Created."); }

void x11_window_show(void* backend)
{ ANVIL_CORE_TRACE("X11 Window Showed."); }

void x11_window_destroy(void* backend)
{ ANVIL_CORE_TRACE("X11 Window Destroyed."); }
