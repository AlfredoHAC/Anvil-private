#ifndef ANVIL_WINDOW_HEADER
#define ANVIL_WINDOW_HEADER

#include "Anvil/Core/types.h"
#include "Anvil/Window/event.h"

// Platform native window
typedef struct NativeWindow NativeWindow;

// Event callback function pointer type
typedef void (*EventCallbackFn)(Event* event);

typedef struct WindowOptions
{
    const char* title;
    uint16      width;
    uint16      height;
} WindowOptions;

NativeWindow* anvl_window_create(const WindowOptions window_options);
void          anvl_window_show(NativeWindow* window);
void          anvl_window_update(NativeWindow* window);
void          anvl_window_destroy(NativeWindow* window);

#endif // !ANVIL_WINDOW_HEADER
