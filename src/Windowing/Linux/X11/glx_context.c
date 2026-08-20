#include "anvlpch.h"

#include "Windowing/Linux/X11/glx_context.h"

GLXContext glx_context_create(Display*    display,
                              GLXFBConfig fbconfig,
                              int32       major_version,
                              int32       minor_version)
{
    ANVIL_ASSERT(display != NULL);

    // clang-format off
    int32 context_attributes[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, major_version,
        GLX_CONTEXT_MINOR_VERSION_ARB, minor_version,
        GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
        #ifdef ANVIL_CONFIG_DEBUG
        GLX_CONTEXT_FLAGS_ARB,         GLX_CONTEXT_DEBUG_BIT_ARB,
        #else
        GLX_CONTEXT_FLAGS_ARB,         0,
        #endif
        None
    };
    // clang-format on

    GLXContext context = glXCreateContextAttribsARB(display,
                                                    fbconfig,
                                                    NULL,
                                                    true,
                                                    context_attributes);
    if (!context)
    {
        ANVIL_CORE_ERROR("Failed to create GLX context.");
        return NULL;
    }

    return context;
}

GLXWindow glx_context_make_current(Display*     display,
                                   GLXFBConfig  fbconfig,
                                   xcb_window_t window,
                                   GLXContext   context)
{
    ANVIL_ASSERT(display != NULL && window != 0 && context != NULL);

    GLXWindow glx_window =
        glXCreateWindow(display, fbconfig, (Window)window, NULL);
    if (!glx_window)
    {
        ANVIL_CORE_ERROR("Failed to create GLX window.");
        return None;
    }

    bool result = glXMakeContextCurrent(display,
                                        (Window)glx_window,
                                        (Window)glx_window,
                                        context);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to make GLX context current.");
        glXDestroyWindow(display, glx_window);
        return None;
    }

    return glx_window;
}

void glx_context_destroy(Display* display, AnvlGLXGraphicsContext* context)
{
    ANVIL_ASSERT(context != NULL);

    glXMakeContextCurrent(display, None, None, NULL);
    glXDestroyContext(display, context->handle);

    if (context->glx_window) { glXDestroyWindow(display, context->glx_window); }
    if (context->visual) { XFree(context->visual); }

    memset(context, 0, sizeof(AnvlGLXGraphicsContext));
}

GLXFBConfig glx_context_choose_fbconfig(
    Display*                             display,
    const struct AnvlGraphicRequirements requirements)
{
    ANVIL_ASSERT(display != NULL);

    // clang-format off
    int32 fbconfig_attributes[] = {
        GLX_X_RENDERABLE,       true,
        GLX_X_VISUAL_TYPE,      GLX_TRUE_COLOR,
        GLX_RENDER_TYPE,        GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE,      GLX_WINDOW_BIT,
        GLX_DOUBLEBUFFER,       true,
        GLX_RED_SIZE,           requirements.red_bits,
        GLX_GREEN_SIZE,         requirements.green_bits,
        GLX_BLUE_SIZE,          requirements.blue_bits,
        GLX_ALPHA_SIZE,         requirements.alpha_bits,
        GLX_DEPTH_SIZE,         requirements.depth_bits,
        GLX_STENCIL_SIZE,       requirements.stencil_bits,
        GLX_SAMPLE_BUFFERS,     requirements.sample_count > 0 ? 1 : 0,
        GLX_SAMPLES,            requirements.sample_count,
        None
    };
    // clang-format on

    int32        fbconfig_count = 0;
    GLXFBConfig* fbconfigs      = glXChooseFBConfig(display,
                                                    DefaultScreen(display),
                                                    fbconfig_attributes,
                                                    &fbconfig_count);
    if (!fbconfigs || fbconfig_count == 0)
    {
        ANVIL_CORE_ERROR("Could not retrieve a valid FBConfig.");
        return NULL;
    }

    GLXFBConfig chosen_one = fbconfigs[0];
    XFree(fbconfigs);

    return chosen_one;
}

XVisualInfo* glx_context_get_visual_info(Display* display, GLXFBConfig fbconfig)
{
    ANVIL_ASSERT(display != NULL && fbconfig != NULL);

    XVisualInfo* visual = glXGetVisualFromFBConfig(display, fbconfig);
    if (!visual)
    {
        ANVIL_CORE_ERROR("Failed to retrieve a valid VisualInfo.");
        return NULL;
    }

    return visual;
}
