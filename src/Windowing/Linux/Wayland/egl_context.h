#ifndef ANVIL_EGL_CONTEXT_HEADER
#define ANVIL_EGL_CONTEXT_HEADER

#include "Anvil/Window/window.h"
#include "Windowing/Linux/Wayland/wayland_backend.h"

AnvlEGLGraphicsContext egl_context_create(
    struct wl_display*      display,
    struct wl_surface*      surface,
    const AnvlWindowOptions window_options);
void egl_context_destroy(AnvlEGLGraphicsContext context);

#endif // !ANVIL_EGL_CONTEXT_HEADER
