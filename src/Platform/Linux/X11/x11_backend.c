#include "anvlpch.h"

#include "Platform/platform.h"
#include "Platform/Linux/X11/x11_backend.h"

#include <xcb/xcb.h>

typedef struct X11Backend
{
    NativeWindow* window;

    struct xcb_connection_t* display;
} X11Backend;
