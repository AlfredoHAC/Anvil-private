#ifndef ANVIL_WINDOW_BACKEND_HEADER
#define ANVIL_WINDOW_BACKEND_HEADER

#include "Window/window.h"

typedef struct AnvlWindowBackend
{
    void* (*backend_init)(void);
    void  (*backend_shutdown)(void*);
    void  (*window_create)(void*, const AnvlWindowOptions);
    void  (*window_show)(void*);
    void  (*window_destroy)(void*);
    void  (*window_set_event_callback)(void*, EventCallbackFn);
    void  (*window_events_poll_and_dispatch)(void*);
    void* (*window_get_handle)(void*);
    void  (*window_present)(void*);
} AnvlWindowBackend;

#endif // !ANVL_WINDOW_BACKEND_HEADER
