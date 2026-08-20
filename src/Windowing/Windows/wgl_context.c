#include "anvlpch.h"

#include "Windowing/Windows/wgl_context.h"

#include <glad/wgl.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>

static void _dummy_cleanup(HWND        dummy_window,
                           HDC         dummy_device_context,
                           HGLRC       dummy_context,
                           const_char* class_name,
                           HINSTANCE   instance);

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
        ANVIL_CORE_ERROR("WGL Extensions not loaded:")
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
