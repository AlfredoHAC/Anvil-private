#ifndef PLATFORM_HEADER
#define PLATFORM_HEADER

#include "Core/typedefs.h"
#include "graphics.h"

typedef struct NativeWindow NativeWindow;
typedef struct GraphicsContext GraphicsContext;

NativeWindow* anvlPlatformWindowCreate(const char* window_title, uint16 window_width, uint16 window_height);
void anvlPlatformWindowUpdate(NativeWindow* window);
void anvlPlatformWindowDestroy(NativeWindow* window);

GraphicsContext* anvlGraphicsContextCreate(GraphicsAPI api, NativeWindow* native_window);
void anvlGraphicsContextMakeCurrent(GraphicsContext* context);
void anvlGraphicsContextDestroy(GraphicsContext* context);

#endif // !PLATFORM_HEADER
