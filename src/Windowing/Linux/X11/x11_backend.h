#ifndef ANVIL_WINDOW_BACKEND_X11_HEADER
#define ANVIL_WINDOW_BACKEND_X11_HEADER

#include "Windowing/window_backend.h"

#include <X11/Xlib.h>
#include <glad/glx.h>

typedef struct X11Backend X11Backend;

typedef struct AnvlGLXGraphicsContext
{
    GLXWindow    glx_window;
    GLXFBConfig  fbconfig;
    XVisualInfo* visual;
    GLXContext   handle;
} AnvlGLXGraphicsContext;

const AnvlWindowBackend* x11_backend();

#endif // !ANVL_WINDOW_BACKEND_X11_HEADER
