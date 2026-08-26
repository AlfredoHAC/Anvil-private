#include "anvlpch.h"

#include "Windowing/Windows/wgl_context.h"

#include <glad/wgl.h>
#include <wingdi.h>
#include <winuser.h>

#define ANVL_EMPTY_WGL_CONTEXT (AnvlWGLGraphicsContext){0}

static void _dummy_cleanup(HWND        dummy_window,
                           HDC         dummy_device_context,
                           HGLRC       dummy_context,
                           const char* class_name,
                           HINSTANCE   instance);
static void _wgl_context_rollback(HWND window, AnvlWGLGraphicsContext* context);

static bool wgl_extensions_loaded = false;

AnvlWGLGraphicsContext wgl_context_create(
    HWND                                 window,
    const struct AnvlGraphicRequirements graphics_requirements)
{
    AnvlWGLGraphicsContext context = {0};

    context.device_context = GetDC(window);
    if (!context.device_context)
    {
        ANVIL_CORE_ERROR("WGL Context not created:");
        ANVIL_CORE_ERROR("-> Failed to get window device context (0x%x).",
                         GetLastError());

        return ANVL_EMPTY_WGL_CONTEXT;
    }

    // clang-format off
    int32 pixel_format_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
        WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,         graphics_requirements.red_bits   +
                                    graphics_requirements.green_bits +
                                    graphics_requirements.blue_bits  +
                                    graphics_requirements.alpha_bits,
        WGL_DEPTH_BITS_ARB,         graphics_requirements.depth_bits,
        WGL_STENCIL_BITS_ARB,       graphics_requirements.stencil_bits,
        WGL_SAMPLE_BUFFERS_ARB,     graphics_requirements.sample_count > 0 ? GL_TRUE : GL_FALSE,
        WGL_SAMPLES_ARB,            graphics_requirements.sample_count,
        0
    };
    // clang-format on

    int32  pixel_format_id    = 0;
    uint32 pixel_format_count = 0;

    bool result = wglChoosePixelFormatARB(context.device_context,
                                          pixel_format_attributes,
                                          NULL,
                                          1,
                                          &pixel_format_id,
                                          &pixel_format_count);
    if (!result || pixel_format_id == 0 || pixel_format_count == 0)
    {
        ANVIL_CORE_ERROR("WGL Context not created:");
        ANVIL_CORE_ERROR(
            "-> Could not retrieve a valid Pixel Format config (0x%x).",
            GetLastError());

        _wgl_context_rollback(window, &context);

        return ANVL_EMPTY_WGL_CONTEXT;
    }

    result = SetPixelFormat(context.device_context, pixel_format_id, NULL);
    if (!result)
    {
        ANVIL_CORE_ERROR("WGL Context not created:");
        ANVIL_CORE_ERROR("-> Failed to set Pixel Format config (0x%x).",
                         GetLastError());

        _wgl_context_rollback(window, &context);

        return ANVL_EMPTY_WGL_CONTEXT;
    }

    // clang-format off
    int32 context_attributes[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, graphics_requirements.major_version,
        WGL_CONTEXT_MINOR_VERSION_ARB, graphics_requirements.minor_version,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        #ifdef ANVIL_CONFIG_DEBUG
        WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
        #else
        WGL_CONTEXT_FLAGS_ARB,         0,
        #endif
        0
    };
    // clang-format on

    context.handle = wglCreateContextAttribsARB(context.device_context,
                                                NULL,
                                                context_attributes);
    if (!context.handle)
    {
        ANVIL_CORE_ERROR("WGL Context not created:");
        ANVIL_CORE_ERROR("-> Failed to create graphics context to the given "
                         "attributes (0x%x).",
                         GetLastError());

        _wgl_context_rollback(window, &context);

        return ANVL_EMPTY_WGL_CONTEXT;
    }

    result = wglMakeCurrent(context.device_context, context.handle);
    if (!result)
    {
        ANVIL_CORE_ERROR("WGL Context not created:");
        ANVIL_CORE_ERROR("-> Failed to make graphics context current (0x%x).",
                         GetLastError());

        _wgl_context_rollback(window, &context);

        return ANVL_EMPTY_WGL_CONTEXT;
    }

    return context;
}

void wgl_context_destroy(HWND window, AnvlWGLGraphicsContext* context)
{
    if (context->handle || context->device_context)
    {
        _wgl_context_rollback(window, context);
    }

    memset(context, 0, sizeof(AnvlWGLGraphicsContext));
}

// clang-format off
void wgl_context_swap_buffers(HDC device_context)
{
    wglSwapBuffers(device_context);
}
// clang-format on

bool wgl_context_load_extensions()
{
    if (wgl_extensions_loaded) { return true; }

    const char* class_name = "anvl_dummy_window_class";
    HINSTANCE   instance   = GetModuleHandle(NULL);

    WNDCLASSEX dummy_window_class = {
        .hInstance     = instance,
        .cbSize        = sizeof(WNDCLASSEX),
        .style         = CS_OWNDC,
        .lpszClassName = class_name,
        .lpfnWndProc   = DefWindowProc,
    };
    ATOM registered = RegisterClassEx(&dummy_window_class);
    if (!registered)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to register Dummy Window class (0x%04x).",
                         GetLastError());

        return false;
    }

    HWND dummy_window = CreateWindowEx(0,
                                       class_name,
                                       "Dummy Window",
                                       WS_OVERLAPPEDWINDOW,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       1,
                                       1,
                                       NULL,
                                       NULL,
                                       dummy_window_class.hInstance,
                                       NULL);
    if (!dummy_window)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to create Dummy Window (0x%04x).",
                         GetLastError());

        _dummy_cleanup(NULL, NULL, NULL, class_name, instance);

        return false;
    }

    HDC dummy_device_context = GetDC(dummy_window);
    if (!dummy_device_context)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR(
            "-> Failed to get Dummy Window device context (0x%04x).",
            GetLastError());

        _dummy_cleanup(dummy_window, NULL, NULL, class_name, instance);

        return false;
    }

    PIXELFORMATDESCRIPTOR dummy_pixel_format_descriptor = {
        .nSize    = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags  = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        .iPixelType   = PFD_TYPE_RGBA,
        .iLayerType   = PFD_MAIN_PLANE,
        .cColorBits   = 32,
        .cDepthBits   = 24,
        .cStencilBits = 8,
    };

    int32 dummy_pixel_format =
        ChoosePixelFormat(dummy_device_context, &dummy_pixel_format_descriptor);
    if (!dummy_pixel_format)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to get valid Pixel Format (0x%04x).",
                         GetLastError());

        _dummy_cleanup(dummy_window,
                       dummy_device_context,
                       NULL,
                       class_name,
                       instance);

        return false;
    }

    bool result = SetPixelFormat(dummy_device_context,
                                 dummy_pixel_format,
                                 &dummy_pixel_format_descriptor);
    if (!result)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to set Pixel Format (0x%04x).",
                         GetLastError());

        _dummy_cleanup(dummy_window,
                       dummy_device_context,
                       NULL,
                       class_name,
                       instance);

        return false;
    }

    HGLRC dummy_context = wglCreateContext(dummy_device_context);
    if (!dummy_context)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to create Dummy Context (0x%04x).",
                         GetLastError());

        _dummy_cleanup(dummy_window,
                       dummy_device_context,
                       NULL,
                       class_name,
                       instance);

        return false;
    }

    result = wglMakeCurrent(dummy_device_context, dummy_context);
    if (!result)
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to make Dummy Context current (0x%04x).",
                         GetLastError());

        _dummy_cleanup(dummy_window,
                       dummy_device_context,
                       dummy_context,
                       class_name,
                       instance);

        return false;
    }

    int32 version = gladLoaderLoadWGL(dummy_device_context);
    if (version < GLAD_MAKE_VERSION(1, 0))
    {
        ANVIL_CORE_ERROR("WGL Extensions not loaded:");
        ANVIL_CORE_ERROR("-> Failed to load GLAD.");

        _dummy_cleanup(dummy_window,
                       dummy_device_context,
                       dummy_context,
                       class_name,
                       instance);

        return false;
    }

    wgl_extensions_loaded = true;

    _dummy_cleanup(dummy_window,
                   dummy_device_context,
                   dummy_context,
                   class_name,
                   instance);

    return true;
}

static void _dummy_cleanup(HWND        dummy_window,
                           HDC         dummy_device_context,
                           HGLRC       dummy_context,
                           const char* class_name,
                           HINSTANCE   instance)
{

    if (dummy_context)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(dummy_context);
    }

    if (dummy_device_context) { ReleaseDC(dummy_window, dummy_device_context); }
    if (dummy_window) { DestroyWindow(dummy_window); }
    if (class_name && instance) { UnregisterClassA(class_name, instance); }
}

static void _wgl_context_rollback(HWND window, AnvlWGLGraphicsContext* context)
{
    if (context->handle)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(context->handle);
    }

    if (context->device_context) { ReleaseDC(window, context->device_context); }
}
