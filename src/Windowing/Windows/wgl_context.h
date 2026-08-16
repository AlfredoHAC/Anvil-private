#ifndef ANVIL_WGL_CONTEXT_HEADER
#define ANVIL_WGL_CONTEXT_HEADER

#include <windows.h>

HGLRC wgl_context_create(HDC   device_context_handle,
                         int32 major_version,
                         int32 minor_version);
void  wgl_context_destroy(HGLRC graphics_context_handle);
void  wgl_context_load_extensions();

#endif // !ANVIL_WGL_CONTEXT_HEADER
