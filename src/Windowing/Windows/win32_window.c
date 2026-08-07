#include "anvlpch.h"

#include "Core/layer.h"
#include "Window/window.h"

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

struct AnvlWindow
{
    HWND      handle;
    HINSTANCE instance;

    EventCallbackFn event_callback;
};

static void    _set_event_callback(AnvlWindow* window, EventCallbackFn event_callback);
static void    _unset_event_callback(AnvlWindow* window);
static LRESULT _dispatch_win32_event(AnvlWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam);
static LRESULT CALLBACK _native_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam);
static void             _peek_and_dispatch_win32_messages(AnvlWindow* window);

AnvlWindow* anvl_window_create(const WindowOptions window_options)
{
    AnvlWindow* window = malloc(sizeof(AnvlWindow));
    memset(window, 0, sizeof(AnvlWindow));
    ANVIL_ASSERT(window != NULL);

    WNDCLASSEXA window_class   = {0};
    window_class.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.cbSize        = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc   = _native_window_proc;
    window_class.hInstance     = window->instance;
    window_class.lpszClassName = "ANVL Main Window";
    window_class.hIcon         = NULL;

    if (!RegisterClassExA(&window_class))
    {
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
        free(window);
        return NULL;
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
    if (window)
    {
        _unset_event_callback(window);

        if (window->instance)
        {
            UnregisterClassA("ANVL Main Window", window->instance);
            window->instance = NULL;
        }

        if (window->handle)
        {
            DestroyWindow(window->handle);
            window->handle = NULL;
        }

        free(window);
    }
}

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

static LRESULT _dispatch_win32_event(AnvlWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam)
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
                        .y_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1.0f : -1.0f,
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
                        .x_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1.0f : -1.0f,
                        .y_offset = 0,
                    },
            };
            window->event_callback(&event);

            break;
        }
    }

    return DefWindowProcA(window->handle, umsg, wparam, lparam);
}

static LRESULT CALLBACK _native_window_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
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
