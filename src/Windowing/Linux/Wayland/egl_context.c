#include "anvlpch.h"

#include "Windowing/Linux/Wayland/egl_context.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <wayland-egl-core.h>

#define EMPTY_EGL_CONTEXT (AnvlEGLGraphicsContext){0}

AnvlEGLGraphicsContext egl_context_create(
    struct wl_display*      display,
    struct wl_surface*      surface,
    const AnvlWindowOptions window_options)
{
    ANVIL_ASSERT(surface != NULL);

    AnvlEGLGraphicsContext context_data = {0};

    context_data.display =
        eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_EXT, display, NULL);
    if (context_data.display == EGL_NO_DISPLAY)
    {
        ANVIL_CORE_ERROR("Failed to obtain EGL display (0x%04X).",
                         eglGetError());
        return EMPTY_EGL_CONTEXT;
    }

    EGLint major_version, minor_version;
    bool   result =
        eglInitialize(context_data.display, &major_version, &minor_version);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to initialize EGL (0x%04X).", eglGetError());
        return EMPTY_EGL_CONTEXT;
    }

    result = eglBindAPI(EGL_OPENGL_API);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to bind OpenGL API (0x%04X).", eglGetError());
        eglTerminate(context_data.display);
        return EMPTY_EGL_CONTEXT;
    }

    // clang-format off
    EGLint config_attributes[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        window_options.graphics_requirements.red_bits,
        EGL_GREEN_SIZE,      window_options.graphics_requirements.green_bits,
        EGL_BLUE_SIZE,       window_options.graphics_requirements.blue_bits,
        EGL_ALPHA_SIZE,      window_options.graphics_requirements.alpha_bits,
        EGL_DEPTH_SIZE,      window_options.graphics_requirements.depth_bits,
        EGL_STENCIL_SIZE,    window_options.graphics_requirements.stencil_bits,
        EGL_SAMPLE_BUFFERS,  window_options.graphics_requirements.sample_count > 0 ? 1 : 0,
        EGL_SAMPLES,         window_options.graphics_requirements.sample_count,
        EGL_NONE
    };
    // clang-format on

    EGLConfig egl_config   = NULL;
    EGLint    config_count = 0;
    result                 = eglChooseConfig(context_data.display,
                                             config_attributes,
                                             &egl_config,
                                             1,
                                             &config_count);
    if (!result || config_count == 0)
    {
        ANVIL_CORE_ERROR("Could not retrieve a valid EGL config (0x%04X).",
                         eglGetError());
        eglTerminate(context_data.display);
        return EMPTY_EGL_CONTEXT;
    }

    context_data.egl_window = wl_egl_window_create(surface,
                                                   window_options.width,
                                                   window_options.height);
    context_data.surface =
        eglCreateWindowSurface(context_data.display,
                               egl_config,
                               (EGLNativeWindowType)context_data.egl_window,
                               NULL);
    if (context_data.surface == EGL_NO_SURFACE)
    {
        ANVIL_CORE_ERROR("Failed to create EGL surface (0x%04X).",
                         eglGetError());
        wl_egl_window_destroy(context_data.egl_window);
        eglTerminate(context_data.display);
        return EMPTY_EGL_CONTEXT;
    }

    // clang-format off
    const EGLint context_attributes[] = {
        EGL_CONTEXT_MAJOR_VERSION,  window_options.graphics_requirements.major_version,
        EGL_CONTEXT_MINOR_VERSION,  window_options.graphics_requirements.minor_version,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        #ifdef ANVIL_CONFIG_DEBUG
        EGL_CONTEXT_OPENGL_DEBUG, EGL_TRUE,
        #endif
        EGL_NONE
    };
    // clang-format on

    context_data.handle = eglCreateContext(context_data.display,
                                           egl_config,
                                           EGL_NO_CONTEXT,
                                           context_attributes);
    if (context_data.handle == EGL_NO_CONTEXT)
    {
        ANVIL_CORE_ERROR("Failed to create EGL context (0x%04X).",
                         eglGetError());
        eglDestroySurface(context_data.display, context_data.surface);
        wl_egl_window_destroy(context_data.egl_window);
        eglTerminate(context_data.display);
        return EMPTY_EGL_CONTEXT;
    }

    result = eglMakeCurrent(context_data.display,
                            context_data.surface,
                            context_data.surface,
                            context_data.handle);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to create make EGL context current (0x%04X).",
                         eglGetError());
        eglDestroyContext(context_data.display, context_data.handle);
        eglDestroySurface(context_data.display, context_data.surface);
        wl_egl_window_destroy(context_data.egl_window);
        eglTerminate(context_data.display);
        return EMPTY_EGL_CONTEXT;
    }

    return context_data;
}

void egl_context_destroy(AnvlEGLGraphicsContext context)
{
    ANVIL_ASSERT(memcmp(&context,
                        &(AnvlEGLGraphicsContext){0},
                        sizeof(AnvlEGLGraphicsContext)) != 0);

    eglMakeCurrent(context.display,
                   EGL_NO_SURFACE,
                   EGL_NO_SURFACE,
                   EGL_NO_CONTEXT);
    eglDestroyContext(context.display, context.handle);
    eglDestroySurface(context.display, context.surface);
    wl_egl_window_destroy(context.egl_window);
    eglTerminate(context.display);
}
