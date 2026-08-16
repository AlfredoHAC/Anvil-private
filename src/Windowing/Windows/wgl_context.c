#include "anvlpch.h"

#include "Windowing/Windows/wgl_context.h"

#include <glad/wgl.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

static bool wgl_extensions_loaded = false;

HGLRC wgl_context_create(HDC   device_context_handle,
                         int32 major_version,
                         int32 minor_version)
{
    ANVIL_ASSERT(device_context_handle != NULL);

    // clang-format off
    int32 context_attributes_list[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, major_version,
        WGL_CONTEXT_MINOR_VERSION_ARB, minor_version,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        #ifdef ANVIL_CONFIG_DEBUG
        WGL_CONTEXT_FLAGS_ARB,         WGL_CONTEXT_DEBUG_BIT_ARB,
        #else
        WGL_CONTEXT_FLAGS_ARB,         0,
        #endif
        0
    };
    // clang-format on

    HGLRC graphics_context_handle =
        wglCreateContextAttribsARB(device_context_handle,
                                   NULL,
                                   context_attributes_list);
    if (!graphics_context_handle)
    {
        ANVIL_CORE_ERROR(
            "Failed to create graphics context to the given attributes (0x%x).",
            GetLastError());
        return NULL;
    }

    bool result =
        wglMakeCurrent(device_context_handle, graphics_context_handle);
    if (!result)
    {
        ANVIL_CORE_ERROR("Failed to make graphics context current (0x%x).",
                         GetLastError());
        wglDeleteContext(graphics_context_handle);
        return NULL;
    }

    return graphics_context_handle;
}

void wgl_context_destroy(HGLRC graphics_context_handle)
{
    ANVIL_ASSERT(graphics_context_handle != NULL);

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext((HGLRC)graphics_context_handle);
}

void wgl_context_load_extensions()
{
    if (wgl_extensions_loaded) { return; }

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

    wgl_extensions_loaded = true;

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy_context);
    ReleaseDC(dummy_window, dummy_device_context);
    DestroyWindow(dummy_window);
    UnregisterClassA(dummy_window_class.lpszClassName,
                     dummy_window_class.hInstance);
}
