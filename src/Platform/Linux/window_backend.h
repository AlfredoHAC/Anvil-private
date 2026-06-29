#ifndef ANVL_WINDOW_BACKEND_HEADER
#define ANVL_WINDOW_BACKEND_HEADER

#include "anvlpch.h"

typedef struct
{
    void* (*backend_init)(void);
    void  (*window_create)(void*, const char*, uint16, uint16);
    void  (*window_show)(void *);
    void  (*window_destroy)(void *);
} WindowBackend;

#endif // !ANVL_WINDOW_BACKEND_HEADER
