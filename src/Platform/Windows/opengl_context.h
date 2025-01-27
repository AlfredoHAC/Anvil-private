#ifndef OPENGL_CONTEXT_HEADER
#define OPENGL_CONTEXT_HEADER

#include "platform_internals.h"

#include <windows.h>

void anvlOpenGLContextInit(GraphicsContext* context);
ContextData* anvlOpenGLContextCreate(NativeWindow* window);
void anvlOpenGLContextMakeCurrent(GraphicsContext* context);
void anvlOpenGLContextDestroy(GraphicsContext* context);

#endif // !OPENGL_CONTEXT_HEADER
