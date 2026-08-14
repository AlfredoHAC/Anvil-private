#include "anvlpch.h"

#include "Graphics/context.h"

#include <glad/wgl.h>
#include <windows.h>

typedef struct AnvlGraphicsContext
{
    HWND  window_handle;
    HDC   device_context_handle;
    HGLRC graphics_context_handle;
} AnvlGraphicsContext;

static void _load_wgl_extensions();

AnvlGraphicsContext* anvl_graphics_context_create(
    void*                         window_handle,
    const AnvlGraphicRequirements requirements)
{
    ANVIL_ASSERT(window_handle != NULL);

    AnvlGraphicsContext* context =
        (AnvlGraphicsContext*)malloc(sizeof(AnvlGraphicsContext));

    context->window_handle         = (HWND)window_handle;
    context->device_context_handle = GetDC(context->window_handle);
    if (!context->device_context_handle)
    {
        ANVIL_CORE_ERROR("Failed to get window device context (0x%x).",
                         GetLastError());
        free(context);
        return NULL;
    }

    _load_wgl_extensions();

    // clang-format off
    int32 pixel_format_attributes[] = {
        WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
        WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,     requirements.pixel_format.red_bits   +
                                requirements.pixel_format.green_bits +
                                requirements.pixel_format.blue_bits  +
                                requirements.pixel_format.alpha_bits,
        WGL_DEPTH_BITS_ARB,     requirements.pixel_format.depth_bits,
        WGL_STENCIL_BITS_ARB,   requirements.pixel_format.stencil_bits,
        WGL_SAMPLE_BUFFERS_ARB, requirements.sample_count > 0 ? GL_TRUE : GL_FALSE,
        WGL_SAMPLES_ARB,        requirements.sample_count,
        0
    };
    // clang-format on

    int32  pixel_format_id    = 0;
    uint32 pixel_format_count = 0;

    bool result = wglChoosePixelFormatARB(context->device_context_handle,
                                          pixel_format_attributes,
                                          NULL,
                                          1,
                                          &pixel_format_id,
                                          &pixel_format_count);
    if (!result || pixel_format_id == 0 || pixel_format_count == 0)
    {
        ANVIL_CORE_ERROR(
            "Could not retrieve a valid Pixel Format config (0x%x).",
            GetLastError());
        ReleaseDC(context->window_handle, context->device_context_handle);
        free(context);
        return NULL;
    }
    result =
        SetPixelFormat(context->device_context_handle, pixel_format_id, NULL);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to set Pixel Format config (0x%x).",
                         GetLastError());
        ReleaseDC(context->window_handle, context->device_context_handle);
        free(context);
        return NULL;
    }

    // clang-format off
    int32 context_attributes_list[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        #ifdef ANVIL_CONFIG_DEBUG
        WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
        #else
        WGL_CONTEXT_FLAGS_ARB,         0,
        #endif
        0
    };
    // clang-format on

    context->graphics_context_handle =
        wglCreateContextAttribsARB(context->device_context_handle,
                                   NULL,
                                   context_attributes_list);
    if (!context->graphics_context_handle)
    {
        ANVIL_CORE_ERROR(
            "Failed to create graphics context to the given attributes (0x%x).",
            GetLastError());
        ReleaseDC(context->window_handle, context->device_context_handle);
        free(context);
        return NULL;
    }

    wglMakeCurrent(context->device_context_handle,
                   context->graphics_context_handle);

    return context;
}

void anvl_graphics_context_destroy(AnvlGraphicsContext* context)
{
    ANVIL_ASSERT(context != NULL);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(context->graphics_context_handle);

    ReleaseDC(context->window_handle, context->device_context_handle);

    free(context);
}

const void* anvl_graphics_context_get_handle(const AnvlGraphicsContext* context)
{
    ANVIL_ASSERT(context != NULL);

    return (const void*)context->graphics_context_handle;
}

static void _load_wgl_extensions()
{
    WNDCLASSEX dummy_window_class = {
        .hInstance     = GetModuleHandle(NULL),
        .cbSize        = sizeof(WNDCLASSEX),
        .style         = CS_OWNDC,
        .lpszClassName = "anvl_dummy_window_class",
        .lpfnWndProc   = DefWindowProc,
    };
    ATOM registered = RegisterClassEx(&dummy_window_class);
    ANVIL_ASSERT(registered != 0);

    HWND dummy_window = CreateWindowEx(0,
                                       "anvl_dummy_window_class",
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
    ANVIL_ASSERT(dummy_window != NULL);

    HDC dummy_device_context = GetDC(dummy_window);
    ANVIL_ASSERT(dummy_device_context != NULL);

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
    bool result = SetPixelFormat(dummy_device_context,
                                 dummy_pixel_format,
                                 &dummy_pixel_format_descriptor);
    ANVIL_ASSERT(result);

    HGLRC dummy_context = wglCreateContext(dummy_device_context);
    wglMakeCurrent(dummy_device_context, dummy_context);

    int version =
        gladLoadWGL(dummy_device_context, (GLADloadfunc)wglGetProcAddress);
    ANVIL_ASSERT(version >= GLAD_MAKE_VERSION(1, 0));

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_device_context);
    DestroyWindow(dummy_window);
    UnregisterClassA(dummy_window_class.lpszClassName,
                     dummy_window_class.hInstance);
}
