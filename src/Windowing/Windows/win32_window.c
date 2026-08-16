#include "anvlpch.h"

#include "Core/layer.h"
#include "Window/window.h"
#include "Windowing/Windows/wgl_context.h"

#include <glad/wgl.h>
#include <windows.h>
#include <windowsx.h>

struct AnvlWindow
{
    HWND      handle;
    HINSTANCE instance;

    struct GraphicsContext
    {
        HDC   device_context;
        HGLRC handle;
    } context;

    EventCallbackFn event_callback;
};

static void             _set_event_callback(AnvlWindow*     window,
                                            EventCallbackFn event_callback);
static void             _unset_event_callback(AnvlWindow* window);
static LRESULT          _dispatch_win32_event(AnvlWindow* window,
                                              UINT        umsg,
                                              WPARAM      wparam,
                                              LPARAM      lparam);
static LRESULT CALLBACK _native_window_proc(HWND   hwnd,
                                            UINT   umsg,
                                            WPARAM wparam,
                                            LPARAM lparam);
static void             _peek_and_dispatch_win32_messages(AnvlWindow* window);

static const char* window_class_name = "anvl_main_window_class";

AnvlWindow* anvl_window_create(const AnvlWindowOptions window_options)
{
    AnvlWindow* window = malloc(sizeof(AnvlWindow));
    memset(window, 0, sizeof(AnvlWindow));
    ANVIL_ASSERT(window != NULL);

    window->instance = GetModuleHandle(NULL);

    WNDCLASSEXA window_class   = {0};
    window_class.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.cbSize        = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc   = _native_window_proc;
    window_class.hInstance     = window->instance;
    window_class.lpszClassName = window_class_name;
    window_class.hIcon         = NULL;

    if (!RegisterClassExA(&window_class))
    {
        ANVIL_CORE_ERROR("Failed to register window class (0x%x).",
                         GetLastError());
        free(window);
        return NULL;
    }

    window->handle = CreateWindowExA(WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
                                     window_class.lpszClassName,
                                     window_options.title,
                                     WS_OVERLAPPEDWINDOW,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     window_options.width,
                                     window_options.height,
                                     NULL,
                                     NULL,
                                     window->instance,
                                     (LPVOID)NULL);

    if (!window->handle)
    {
        ANVIL_CORE_ERROR("Failed to create window (0x%x).", GetLastError());
        free(window);
        return NULL;
    }

    if (window_options.graphics_mode == ANVL_WINDOW_GRAPHICS_MODE_OPENGL)
    {
        wgl_context_load_extensions();

        window->context.device_context = GetDC(window->handle);
        if (!window->context.device_context)
        {
            ANVIL_CORE_ERROR("Failed to get window device context (0x%x).",
                             GetLastError());
            DestroyWindow(window->handle);
            free(window);
            return NULL;
        }

        // clang-format off
        int32 pixel_format_attributes[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB,  GL_TRUE,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     window_options.graphics_requirements.red_bits   +
                                    window_options.graphics_requirements.green_bits +
                                    window_options.graphics_requirements.blue_bits  +
                                    window_options.graphics_requirements.alpha_bits,
            WGL_DEPTH_BITS_ARB,     window_options.graphics_requirements.depth_bits,
            WGL_STENCIL_BITS_ARB,   window_options.graphics_requirements.stencil_bits,
            WGL_SAMPLE_BUFFERS_ARB, window_options.graphics_requirements.sample_count > 0 ? GL_TRUE : GL_FALSE,
            WGL_SAMPLES_ARB,        window_options.graphics_requirements.sample_count,
            0
        };
        // clang-format on

        int32  pixel_format_id    = 0;
        uint32 pixel_format_count = 0;

        bool result = wglChoosePixelFormatARB(window->context.device_context,
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
            ReleaseDC(window->handle, window->context.device_context);
            DestroyWindow(window->handle);
            free(window);
            return NULL;
        }
        result = SetPixelFormat(window->context.device_context,
                                pixel_format_id,
                                NULL);
        if (!result)
        {
            ANVIL_CORE_ERROR("Failed to set Pixel Format config (0x%x).",
                             GetLastError());
            ReleaseDC(window->handle, window->context.device_context);
            DestroyWindow(window->handle);
            free(window);
            return NULL;
        }

        window->context.handle =
            wgl_context_create(window->context.device_context);
        if (!window->context.handle)
        {
            ReleaseDC(window->handle, window->context.device_context);
            DestroyWindow(window->handle);
            free(window);
            return NULL;
        }
    }

    SetWindowLongPtrA(window->handle, GWLP_USERDATA, (LONG_PTR)window);
    _set_event_callback(window, anvl_layer_stack_dispatch_event);

    return window;
}

// clang-format off
void anvl_window_show(AnvlWindow* window)
{
    ShowWindow(window->handle, SW_SHOWNORMAL);
}

void anvl_window_update(AnvlWindow* window)
{
    _peek_and_dispatch_win32_messages(window);
}
// clang-format on

void anvl_window_destroy(AnvlWindow* window)
{
    ANVIL_ASSERT(window != NULL);

    _unset_event_callback(window);

    if (!window->context.handle)
    {
        wgl_context_destroy(window->context.handle);
    }

    if (window->context.device_context)
    {
        ReleaseDC(window->handle, window->context.device_context);
    }

    DestroyWindow(window->handle);
    UnregisterClassA(window_class_name, window->instance);

    free(window);
}

// clang-format off
void* anvl_window_get_handle(const AnvlWindow* window)
{
    return (void*)window->handle;
}
// clang-format on

void _set_event_callback(AnvlWindow* window, EventCallbackFn event_callback)
{
    ANVIL_ASSERT(event_callback != NULL);

    window->event_callback = event_callback;
}

// clang-format off
void _unset_event_callback(AnvlWindow* window)
{
    window->event_callback = NULL;
}
// clang-format on

static LRESULT _dispatch_win32_event(AnvlWindow* window,
                                     UINT        umsg,
                                     WPARAM      wparam,
                                     LPARAM      lparam)
{

    switch (umsg)
    {
        case WM_CLOSE:
        {
            AnvlEvent event = {
                .type         = ANVL_EVENT_TYPE_WINDOW_CLOSE,
                .handled      = false,
                .window_close = {0},
            };
            window->event_callback(&event);

            if (event.handled) { return 0; }

            break;
        }
        case WM_SIZE:
        {
            if (wparam != SIZE_MINIMIZED)
            {

                AnvlEvent event = {
                    .type    = ANVL_EVENT_TYPE_WINDOW_RESIZE,
                    .handled = false,
                    .window_resize =
                        {
                            .width  = (uint16)LOWORD(lparam),
                            .height = (uint16)HIWORD(lparam),
                        },
                };
                window->event_callback(&event);
            }

            break;
        }
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            AnvlEvent event = {
                .type      = ANVL_EVENT_TYPE_KEY_PRESS,
                .handled   = false,
                .key_press = {.key_code = (uint16)wparam, .modifier_set = 0},
            };
            window->event_callback(&event);

            break;
        }
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            AnvlEvent event = {
                .type        = ANVL_EVENT_TYPE_KEY_RELEASE,
                .handled     = false,
                .key_release = {.key_code = (uint16)wparam, .modifier_set = 0},
            };
            window->event_callback(&event);

            break;
        }
        case WM_MOUSEMOVE:
        {
            AnvlEvent event = {
                .type       = ANVL_EVENT_TYPE_MOUSE_MOVE,
                .handled    = false,
                .mouse_move = {.x = (float32)GET_X_LPARAM(lparam),
                               .y = (float32)GET_Y_LPARAM(lparam)},
            };
            window->event_callback(&event);

            break;
        }
        case WM_LBUTTONDOWN:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
                .handled = false,
                .mouse_button_click =
                    {
                        .button_code  = 1,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_MBUTTONDOWN:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
                .handled = false,
                .mouse_button_click =
                    {
                        .button_code  = 2,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_RBUTTONDOWN:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
                .handled = false,
                .mouse_button_click =
                    {
                        .button_code  = 3,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_XBUTTONDOWN:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_CLICK,
                .handled = false,
                .mouse_button_click =
                    {
                        .button_code  = (uint8)GET_XBUTTON_WPARAM(wparam),
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_LBUTTONUP:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = 1,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_MBUTTONUP:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = 2,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_RBUTTONUP:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = 3,
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_XBUTTONUP:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_BUTTON_RELEASE,
                .handled = false,
                .mouse_button_release =
                    {
                        .button_code  = (uint8)GET_XBUTTON_WPARAM(wparam),
                        .x            = (float32)GET_X_LPARAM(lparam),
                        .y            = (float32)GET_Y_LPARAM(lparam),
                        .modifier_set = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_MOUSEWHEEL:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_SCROLL,
                .handled = false,
                .mouse_scroll =
                    {
                        .x_offset = 0,
                        .y_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0
                                        ? 1.0f
                                        : -1.0f,
                    },
            };
            window->event_callback(&event);

            break;
        }
        case WM_MOUSEHWHEEL:
        {
            AnvlEvent event = {
                .type    = ANVL_EVENT_TYPE_MOUSE_SCROLL,
                .handled = false,
                .mouse_scroll =
                    {
                        .x_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0
                                        ? 1.0f
                                        : -1.0f,
                        .y_offset = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
    }

    return DefWindowProcA(window->handle, umsg, wparam, lparam);
}

static LRESULT CALLBACK _native_window_proc(HWND   hwnd,
                                            UINT   umsg,
                                            WPARAM wparam,
                                            LPARAM lparam)
{
    AnvlWindow* window = (AnvlWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (window != 0 && window->handle == hwnd)
    {
        return _dispatch_win32_event(window, umsg, wparam, lparam);
    }

    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

static void _peek_and_dispatch_win32_messages(AnvlWindow* window)
{
    MSG msg;
    while ((int32)PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE) != 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}
