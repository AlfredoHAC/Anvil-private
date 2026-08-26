#include "Windowing/Linux/X11/x11_backend.h"
#include "anvlpch.h"

#include "Windowing/Linux/X11/glx_context.h"

static void _glx_context_rollback(Display*                display,
                                  GLXWindow               glx_window,
                                  AnvlGLXGraphicsContext* context);

GLXContext glx_context_create(Display*                display,
                              AnvlGLXGraphicsContext* context,
                              int32                   major_version,
                              int32                   minor_version)
{
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

    GLXContext glx_context = glXCreateContextAttribsARB(display,
                                                        context->fbconfig,
                                                        None,
                                                        true,
                                                        context_attributes);
    if (!glx_context)
    {
        ANVIL_CORE_ERROR("GLX context not created:");
        ANVIL_CORE_ERROR("-> Failed to create GLX context.");

        _glx_context_rollback(display, None, context);

        return None;
    }

    return glx_context;
}

GLXWindow glx_context_make_current(Display*                display,
                                   xcb_window_t            window,
                                   AnvlGLXGraphicsContext* context)
{
    GLXWindow glx_window =
        glXCreateWindow(display, context->fbconfig, (Window)window, NULL);
    if (!glx_window)
    {
        ANVIL_CORE_ERROR("GLX context not created:");
        ANVIL_CORE_ERROR("-> Failed to create GLX window.");

        _glx_context_rollback(display, None, context);

        return None;
    }

    bool result = glXMakeContextCurrent(display,
                                        (Window)glx_window,
                                        (Window)glx_window,
                                        context->handle);
    if (!result)
    {
        ANVIL_CORE_ERROR("GLX context not created:");
        ANVIL_CORE_ERROR("-> Failed to make GLX context current.");

        _glx_context_rollback(display, glx_window, context);

        return None;
    }

    return glx_window;
}

void glx_context_destroy(Display* display, AnvlGLXGraphicsContext* context)
{
    _glx_context_rollback(display, None, context);

    memset(context, 0, sizeof(AnvlGLXGraphicsContext));
}

// clang-format off
void glx_context_swap_buffers(Display* display, GLXWindow window)
{
    glXSwapBuffers(display, window);
}
// clang-format on

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

bool glx_context_load_extensions(Display* display)
{
    int32 version = gladLoaderLoadGLX(display, DefaultScreen(display));
    if (version < GLAD_MAKE_VERSION(1, 3))
    {
        ANVIL_CORE_ERROR("GLX context not created:");
        ANVIL_CORE_ERROR("-> Failed to load GLAD.");

        return false;
    }

    return true;
}

static void _glx_context_rollback(Display*                display,
                                  GLXWindow               glx_window,
                                  AnvlGLXGraphicsContext* context)
{
    if (context->handle) { glXMakeContextCurrent(display, None, None, NULL); }

    GLXWindow context_window = glx_window ? glx_window : context->glx_window;
    if (context_window) { glXDestroyWindow(display, context_window); }

    if (context->handle) { glXDestroyContext(display, context->handle); }

    if (context->visual) { XFree(context->visual); }
}
