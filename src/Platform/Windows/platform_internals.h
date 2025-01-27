#ifndef PLATFORM_INTERNALS_HEADER
#define PLATFORM_INTERNALS_HEADER

#include "Platform/platform.h"

#include <windows.h>
#include <windowsx.h>

typedef struct ContextData ContextData;

typedef ContextData* (*PFNGRAPHICSCONTEXTCREATEFUNC)(NativeWindow*);
typedef void (*PFNGRAPHICSCONTEXTMAKECURRENTFUNC)(GraphicsContext*);
typedef void (*PFNGRAPHICSCONTEXTDESTROYFUNC)(GraphicsContext*);

struct NativeWindow {
	HWND        handle;
	HINSTANCE   instance;
	HDC			device_context;
};

struct GraphicsContext {
	GraphicsAPI api;
	ContextData* data;

	PFNGRAPHICSCONTEXTCREATEFUNC		GraphicsContextCreate;
	PFNGRAPHICSCONTEXTMAKECURRENTFUNC	GraphicsContextMakeCurrent;
	PFNGRAPHICSCONTEXTDESTROYFUNC		GraphicsContextDestroy;
};

#endif // PLATFORM_INTERNALS_HEADER
