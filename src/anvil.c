#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#if defined(_MSC_VER)
#	define ANVIL_COMPILER_MSVC 1

// Target OS (Windows, Linux, macOS)
#	if defined(_WIN32) || defined(_WIN64)
#		define ANVIL_PLATFORM_WINDOWS 1
#	else
#		error OS platform not supported!
#	endif
#endif

#define nullptr ((void*)0)

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

int main() {
    const char* app_name = "AnvilFramework";
    char app_window_title[50];
    int16_t app_window_width = 1280;
    int16_t app_window_height = 720;

    strncpy(app_window_title, app_name, sizeof(app_window_title));

    printf("Hello from Anvil!\n");
    printf("App hints:\n");
    printf("-> Name: %s\n", app_name);
    printf("-> Window title: %s\n", app_window_title);
    printf("-> Window width: %d\n", app_window_width);
    printf("-> Window height: %d\n", app_window_height);

#   ifdef ANVIL_PLATFORM_WINDOWS

    HWND window_handle;
    HINSTANCE window_instance = GetModuleHandleA(nullptr);

    WNDCLASSEXA window_class = { 0 };
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.cbSize = sizeof(WNDCLASSEXA);
    window_class.lpfnWndProc = _WindowProc;
    window_class.hInstance = window_instance;
    window_class.lpszClassName = "ANVL Main Window";
    window_class.hIcon = nullptr;

    if (!RegisterClassExA(&window_class)) {
        exit(EXIT_FAILURE);
    }

    window_handle = CreateWindowExA(
        WS_EX_APPWINDOW | WS_EX_ACCEPTFILES,
        window_class.lpszClassName,
        app_window_title,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        app_window_width, app_window_height,
        nullptr,
        nullptr,
        window_instance,
        (LPVOID) nullptr
    );

    if (!window_handle) {
        exit(EXIT_FAILURE);
    }

    ShowWindow(window_handle, SW_SHOWNORMAL);
    UpdateWindow(window_handle);

    MSG msg;
    while (GetMessageA(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

#   endif // ANVIL_PLATFORM_WINDOWS

    return 0;
}
