#ifndef ANVL_WINDOW_BACKEND_HEADER
#define ANVL_WINDOW_BACKEND_HEADER

#include "anvlpch.h"

typedef struct
{
    void* (*init_backend)();
    void  (*create_window)(const char*, uint16, uint16);
    void  (*show_window)(void *);
    void  (*destroy_window)(void *);
} WindowBackend;

#endif // !ANVL_WINDOW_BACKEND_HEADER
