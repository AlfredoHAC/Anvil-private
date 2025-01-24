#ifndef OPENGL_CONTEXT_HEADER
#define OPENGL_CONTEXT_HEADER

#include "platform.h"

#include <windows.h>

void anvlOpenGLContextInit();
ContextData* anvlOpenGLContextCreate(HDC device_context);
void anvlOpenGLContextDestroy(ContextData* context_data);

#endif // !OPENGL_CONTEXT_HEADER
