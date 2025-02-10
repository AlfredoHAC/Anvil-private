#include "anvlpch.h"

#include "Platform/platform.h"

#include <windows.h>
#include <windowsx.h>

struct NativeWindow
{
	HWND        handle;
	HINSTANCE   instance;

	PFEVENTCALLBACKFUNC EventCallback;
};

static LRESULT _ProcessEvent(NativeWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam)
{

	switch (umsg)
	{
	case WM_CLOSE:
	{
		Event event =
		{
			.type = WindowClose,
			.handled = false,
			.window_close = {0}
		};

		window->EventCallback(event);
		break;
	}
	case WM_SIZE:
	{
		Event event =
		{
			.type = WindowResize,
			.handled = false,
			.window_resize =
			{
				.width = (uint16)LOWORD(lparam),
				.height = (uint16)HIWORD(lparam)
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		Event event =
		{
			.type = KeyPress,
			.handled = false,
			.key_press =
			{
				.key_code = (uint16)wparam,
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{
		Event event =
		{
			.type = KeyRelease,
			.handled = false,
			.key_release =
			{
				.key_code = (uint16)wparam,
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_MOUSEMOVE:
	{
		Event event =
		{
			.type = MouseMove,
			.handled = false,
			.mouse_move =
			{
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam)
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_LBUTTONDOWN:
	{
		Event event =
		{
			.type = MouseButtonClick,
			.handled = false,
			.mouse_button_click =
			{
				.button_code = 1,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_MBUTTONDOWN:
	{
		Event event =
		{
			.type = MouseButtonClick,
			.handled = false,
			.mouse_button_click =
			{
				.button_code = 2,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_RBUTTONDOWN:
	{
		Event event =
		{
			.type = MouseButtonClick,
			.handled = false,
			.mouse_button_click =
			{
				.button_code = 3,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_XBUTTONDOWN:
	{
		Event event =
		{
			.type = MouseButtonClick,
			.handled = false,
			.mouse_button_click =
			{
				.button_code = (uint8)GET_XBUTTON_WPARAM(wparam),
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_LBUTTONUP:
	{
		Event event =
		{
			.type = MouseButtonRelease,
			.handled = false,
			.mouse_button_release =
			{
				.button_code = 1,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_MBUTTONUP:
	{
		Event event =
		{
			.type = MouseButtonRelease,
			.handled = false,
			.mouse_button_release =
			{
				.button_code = 2,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_RBUTTONUP:
	{
		Event event =
		{
			.type = MouseButtonRelease,
			.handled = false,
			.mouse_button_release =
			{
				.button_code = 3,
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_XBUTTONUP:
	{
		Event event =
		{
			.type = MouseButtonRelease,
			.handled = false,
			.mouse_button_release =
			{
				.button_code = (uint8)GET_XBUTTON_WPARAM(wparam),
				.x = (float32)GET_X_LPARAM(lparam),
				.y = (float32)GET_Y_LPARAM(lparam),
				.modifier_set = 0
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_MOUSEWHEEL:
	{
		Event event =
		{
			.type = MouseScroll,
			.handled = false,
			.mouse_scroll =
			{
				.x_offset = 0,
				.y_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1.0f : -1.0f,
			}
		};

		window->EventCallback(event);
		break;
	}
	case WM_MOUSEHWHEEL:
	{
		Event event =
		{
			.type = MouseScroll,
			.handled = false,
			.mouse_scroll =
			{
				.x_offset = (float32)GET_WHEEL_DELTA_WPARAM(wparam) > 0 ? 1.0f : -1.0f,
				.y_offset = 0,
			}
		};

		window->EventCallback(event);
		break;
	}
	}

	return DefWindowProcA(window->handle, umsg, wparam, lparam);
}

static LRESULT CALLBACK _NativeWindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	NativeWindow* window = (NativeWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (window && window->handle == hwnd)
	{
		return _ProcessEvent(window, umsg, wparam, lparam);
	}

	return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

NativeWindow* anvlPlatformWindowCreate(const char* window_title, uint16 window_width, uint16 window_height) {
	NativeWindow* window = malloc(sizeof(NativeWindow));
	if (!window)
	{
		return null;
	}

	WNDCLASSEXA window_class = { 0 };
	window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	window_class.cbSize = sizeof(WNDCLASSEXA);
	window_class.lpfnWndProc = _NativeWindowProc;
	window_class.hInstance = window->instance;
	window_class.lpszClassName = "ANVL Main Window";
	window_class.hIcon = null;

	if (!RegisterClassExA(&window_class))
	{
		return null;
	}

	window->handle = CreateWindowExA
	(
		WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
		window_class.lpszClassName,
		window_title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		window_width, window_height,
		null,
		null,
		window->instance,
		(LPVOID)null
	);

	if (!window->handle)
	{
		return null;
	}

	SetWindowLongPtrA(window->handle, GWLP_USERDATA, (LONG_PTR)window);

	return window;
}

void anvlPlatformWindowShow(NativeWindow* window)
{
	ShowWindow(window->handle, SW_SHOWNORMAL);
}

static void _PollEvents(NativeWindow* window)
{
	MSG msg;
	while (PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

void anvlPlatformWindowUpdate(NativeWindow* window)
{
	_PollEvents(window);
}


void anvlPlatformWindowDestroy(NativeWindow* window)
{
	if (window)
	{
		if (window->handle)
		{
			DestroyWindow(window->handle);
			window->handle = null;
		}

		free(window);
		window = null;
	}
}

void anvlPlatformSetWindowEventCallback(NativeWindow* window, PFEVENTCALLBACKFUNC event_callback)
{
	if (!event_callback)
	{
		return;
	}

	window->EventCallback = event_callback;
}


