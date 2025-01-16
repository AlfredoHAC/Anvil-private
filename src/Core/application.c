#include "application.h"

#include "Core/base.h"

#include <stdio.h>
#include <string.h>

#ifdef ANVIL_PLATFORM_WINDOWS
#   include <Windows.h>
#   include <windowsx.h>
#endif

static LRESULT CALLBACK _WindowProc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch (umsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProcA(hwnd, umsg, wparam, lparam);
}

bool anvlAppInit(const char* app_name, uint16 window_width, uint16 window_height) {
    char app_window_title[50];
    strncpy(app_window_title, app_name, sizeof(app_window_title));

    printf("-> Name: %s\n", app_name);
    printf("-> Window title: %s\n", app_window_title);
    printf("-> Window width: %d\n", window_width);
    printf("-> Window height: %d\n", window_height);

#   ifdef ANVIL_PLATFORM_WINDOWS

    HWND window_handle;
    HINSTANCE window_instance = GetModuleHandleA(null);

    WNDCLASSEXA window_class = { 0 };
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc = _WindowProc;
    window_class.hInstance = window_instance;
    window_class.lpszClassName = "ANVL Main Window";
    window_class.hIcon = null;

    if (!RegisterClassExA(&window_class)) {
        return false;
    }

    window_handle = CreateWindowExA(
        WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        window_class.lpszClassName,
        app_window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        window_width, window_height,
        null,
        null,
        window_instance,
        (LPVOID) null
    );

    if (!window_handle) {
        return false;
    }

    ShowWindow(window_handle, SW_SHOWNORMAL);
    UpdateWindow(window_handle);

    MSG msg;
    while (GetMessageA(&msg, null, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

#   endif // ANVIL_PLATFORM_WINDOWS

    return true;
}

