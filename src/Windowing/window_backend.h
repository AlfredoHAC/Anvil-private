#ifndef ANVIL_WINDOW_BACKEND_HEADER
#define ANVIL_WINDOW_BACKEND_HEADER

#include "Window/window.h"

typedef struct AnvlWindowBackend
{
    void*                    (*init)(void);
    void                     (*shutdown)(void*);
    void                     (*create_window)(void*, const AnvlWindowOptions);
    void                     (*show_window)(void*);
    void                     (*destroy_window)(void*);
    void                     (*set_event_callback)(void*, AnvlEventCallbackFn);
    void                     (*poll_and_dispatch_events)(void*);
    AnvlWindowPlatform (*get_window_native_platform)(void);
    AnvlWindowNativeHandle   (*get_window_native_handle)(void*);
    uint16                   (*get_window_width)(void*);
    uint16                   (*get_window_height)(void*);
    void                     (*present)(void*);
} AnvlWindowBackend;

#endif // !ANVL_WINDOW_BACKEND_HEADER
