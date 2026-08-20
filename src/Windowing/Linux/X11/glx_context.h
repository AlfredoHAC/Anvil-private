
#ifndef ANVIL_GLX_CONTEXT_HEADER
#define ANVIL_GLX_CONTEXT_HEADER

#include "Window/window.h"
#include "Windowing/Linux/X11/x11_backend.h"

#include <X11/Xlib.h>
#include <glad/glx.h>
#include <xcb/xcb.h>

GLXContext glx_context_create(Display*    display,
                              GLXFBConfig fbconfig,
                              int32       major_version,
                              int32       minor_version);
GLXWindow  glx_context_make_current(Display*     display,
                                    GLXFBConfig  fbconfig,
                                    xcb_window_t window,
                                    GLXContext   context);
void glx_context_destroy(Display* display, AnvlGLXGraphicsContext* context);
GLXFBConfig glx_context_choose_fbconfig(
    Display*                             display,
    const struct AnvlGraphicRequirements requirements);
XVisualInfo* glx_context_get_visual_info(Display*    display,
                                         GLXFBConfig fbconfig);

#endif // !ANVIL_GLX_CONTEXT_HEADER
