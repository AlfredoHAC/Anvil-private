#ifndef ANVL_WINDOW_BACKEND_HEADER
#define ANVL_WINDOW_BACKEND_HEADER

#include "Platform/platform.h"
#include "anvlpch.h"

typedef struct
{
    void* (*backend_init)(void);
    void  (*backend_shutdown)(void*);
    void  (*window_create)(void*, const char*, uint16, uint16);
    void  (*window_show)(void *);
    void  (*window_destroy)(void *);
    void  (*window_set_event_callback)(void*, EventCallbackFn);
    void  (*window_events_poll_and_dispatch)(void *);
} WindowBackend;

#endif // !ANVL_WINDOW_BACKEND_HEADER
