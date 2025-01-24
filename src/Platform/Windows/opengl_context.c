#include "anvlpch.h"

#include "opengl_context.h"

// TODO: replace with the definitions in the 'wglext.h' file
#define WGL_DRAW_TO_WINDOW_ARB            0x2001
#define WGL_SUPPORT_OPENGL_ARB            0x2010
#define WGL_DOUBLE_BUFFER_ARB             0x2011
#define WGL_PIXEL_TYPE_ARB                0x2013
#define WGL_TYPE_RGBA_ARB                 0x202B
#define WGL_COLOR_BITS_ARB                0x2014
#define WGL_DEPTH_BITS_ARB                0x2022
#define WGL_STENCIL_BITS_ARB              0x2023

#define WGL_CONTEXT_MAJOR_VERSION_ARB     0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB     0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB      0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB  0x00000001

typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC) (HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList, UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC) (HDC hDC, HGLRC hShareContext, const int* attribList);
//

struct ContextData {
	HGLRC handle;
};

static PFNWGLCHOOSEPIXELFORMATARBPROC	  wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC  wglCreateContextAttribsARB;
static PIXELFORMATDESCRIPTOR			  pixel_format_descriptor;

void anvlOpenGLContextInit() {
	pixel_format_descriptor.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pixel_format_descriptor.nVersion = 1;
	pixel_format_descriptor.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pixel_format_descriptor.iPixelType = PFD_TYPE_RGBA;
	pixel_format_descriptor.cColorBits = 32;
	pixel_format_descriptor.cAlphaBits = 8;
	pixel_format_descriptor.cDepthBits = 24;
	pixel_format_descriptor.cStencilBits = 8;

	WNDCLASSA dummy_window_class = { 0 };
	dummy_window_class.style = CS_OWNDC;
	dummy_window_class.lpfnWndProc = DefWindowProc;
	dummy_window_class.hInstance = GetModuleHandleA(null);
	dummy_window_class.lpszClassName = "DummyWindowClass";
	if (!RegisterClassA(&dummy_window_class)) {
		return;
	}

	HWND dummy_window_handle = CreateWindowA(
		"DummyWindowClass", "DummyWindow",
		WS_OVERLAPPEDWINDOW,
		0, 0, 1, 1,
		null,
		null,
		dummy_window_class.hInstance,
		null
	);
	if (!dummy_window_handle) {
		return;
	}

	HDC dummy_window_device_context = GetDC(dummy_window_handle);
	int32 dummy_context_pixel_format = ChoosePixelFormat(dummy_window_device_context, &pixel_format_descriptor);
	if (!dummy_context_pixel_format) {
		return;
	}
	if (!SetPixelFormat(dummy_window_device_context, dummy_context_pixel_format, &pixel_format_descriptor)) {
		return;
	}

	HGLRC dummy_context = wglCreateContext(dummy_window_device_context);
	if (!dummy_context) {
		return;
	}
	if (!wglMakeCurrent(dummy_window_device_context, dummy_context)) {
		return;
	}

	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB) {
		return;
	}

	wglMakeCurrent(null, null);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window_handle, dummy_window_device_context);
	DestroyWindow(dummy_window_handle);
}

ContextData* anvlOpenGLContextCreate(HDC device_context)
{
	ContextData* context_data = malloc(sizeof(ContextData));
	if (!context_data) {
		return null;
	}

	int32 pixel_attributes[] = {
		WGL_DRAW_TO_WINDOW_ARB, true,
		WGL_SUPPORT_OPENGL_ARB, true,
		WGL_DOUBLE_BUFFER_ARB,	true,
		WGL_PIXEL_TYPE_ARB,		WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,		32,
		WGL_DEPTH_BITS_ARB,		24,
		WGL_STENCIL_BITS_ARB,	8,
		0
	};

	int32 pixel_format = 0;
	uint32 pixel_format_count = 0;
	wglChoosePixelFormatARB(device_context, pixel_attributes, null, 1, &pixel_format, &pixel_format_count);
	if (pixel_format_count == 0) {
		return null;
	}
	SetPixelFormat(device_context, pixel_format, &pixel_format_descriptor);

	// TODO: Improve this code to have more important attributes
	int32 graphics_context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	//

	context_data->handle = wglCreateContextAttribsARB(device_context, null, graphics_context_attributes);
	if (!context_data->handle) {
		return null;
	}

	wglMakeCurrent(device_context, context_data->handle);

	return context_data;
}

void anvlOpenGLContextDestroy(ContextData* context_data)
{
	if (context_data) {
		wglMakeCurrent(null, null);

		if (context_data->handle) {
			wglDeleteContext(context_data->handle);
		}

		free(context_data);
		context_data = null;
	}
}
