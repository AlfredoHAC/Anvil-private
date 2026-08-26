#ifndef ANVIL_WGL_CONTEXT_HEADER
#define ANVIL_WGL_CONTEXT_HEADER

#include "Anvil/Window/window.h"

#include <windows.h>

typedef struct AnvlWGLGraphicsContext
{
    HDC   device_context;
    HGLRC handle;
} AnvlWGLGraphicsContext;

AnvlWGLGraphicsContext wgl_context_create(
    HWND                                 window,
    const struct AnvlGraphicRequirements graphics_requirements);
void wgl_context_destroy(HWND window, AnvlWGLGraphicsContext* context);
void wgl_context_swap_buffers(HDC device_context);
bool wgl_context_load_extensions();

#endif // !ANVIL_WGL_CONTEXT_HEADER
