#ifndef PLATFORM_HEADER
#define PLATFORM_HEADER

#include "Core/typedefs.h"
#include "graphics.h"

typedef struct NativeWindow NativeWindow;
typedef struct ContextData ContextData;
typedef struct GraphicsContext {
	GraphicsAPI api;

	ContextData* data;

	void(*GraphicsContextDataDestroyFunc)(ContextData*);
} GraphicsContext;

NativeWindow* anvlPlatformWindowCreate(const char* window_title, uint16 window_width, uint16 window_height);
void anvlPlatformWindowUpdate(NativeWindow* window);
void anvlPlatformWindowDestroy(NativeWindow* window);

GraphicsContext* anvlGraphicsContextCreate(GraphicsAPI api, NativeWindow* native_window);
void anvlGraphicsContextDestroy(GraphicsContext* context);

#endif // !PLATFORM_HEADER
