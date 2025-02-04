#include "anvlpch.h"

#include "Platform/platform.h"

#include <windows.h>
#include <windowsx.h>

struct NativeWindow {
	HWND        handle;
	HINSTANCE   instance;
};

static LRESULT _ProcessEvent(NativeWindow* window, UINT umsg, WPARAM wparam, LPARAM lparam) {

	switch (umsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	case WM_DESTROY:
		return 0;
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		break;
	case WM_MOUSEMOVE:
		break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_XBUTTONDOWN:
		break;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_XBUTTONUP:
		break;
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		break;
	}

	return DefWindowProcA(window->handle, umsg, wparam, lparam);
}

static LRESULT CALLBACK _NativeWindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
	NativeWindow* window = (NativeWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (window && window->handle == hwnd) {
		return _ProcessEvent(window, umsg, wparam, lparam);
	}

	return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

NativeWindow* anvlPlatformWindowCreate(const char* window_title, uint16 window_width, uint16 window_height) {
	NativeWindow* window = malloc(sizeof(NativeWindow));
	if (!window) {
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

	if (!RegisterClassExA(&window_class)) {
		return null;
	}

	window->handle = CreateWindowExA(
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

	if (!window->handle) {
		return null;
	}

	SetWindowLongPtrA(window->handle, GWLP_USERDATA, (LONG_PTR)window);
	ShowWindow(window->handle, SW_SHOWNORMAL);

	return window;
}

static void _PollEvents(NativeWindow* window) {
	MSG msg;
	while (PeekMessageA(&msg, window->handle, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}

void anvlPlatformWindowUpdate(NativeWindow* window) {
	_PollEvents(window);
}


void anvlPlatformWindowDestroy(NativeWindow* window) {
	if (window) {
		if (window->handle) {
			DestroyWindow(window->handle);
			window->handle = null;
		}

		free(window);
		window = null;
	}
}


