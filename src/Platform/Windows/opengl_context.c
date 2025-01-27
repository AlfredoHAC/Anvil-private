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
	HDC   device_context;
};

static PFNWGLCHOOSEPIXELFORMATARBPROC	  _wglChoosePixelFormatARB;
static PFNWGLCREATECONTEXTATTRIBSARBPROC  _wglCreateContextAttribsARB;
static PIXELFORMATDESCRIPTOR			  _pixel_format_descriptor;

void _GetOpenGLExtensionsProcs() {
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
	int32 dummy_context_pixel_format = ChoosePixelFormat(dummy_window_device_context, &_pixel_format_descriptor);
	if (!dummy_context_pixel_format) {
		return;
	}
	if (!SetPixelFormat(dummy_window_device_context, dummy_context_pixel_format, &_pixel_format_descriptor)) {
		return;
	}

	HGLRC dummy_context = wglCreateContext(dummy_window_device_context);
	if (!dummy_context) {
		return;
	}
	if (!wglMakeCurrent(dummy_window_device_context, dummy_context)) {
		return;
	}

	_wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	_wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

	if (!_wglChoosePixelFormatARB || !_wglCreateContextAttribsARB) {
		return;
	}

	wglMakeCurrent(null, null);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummy_window_handle, dummy_window_device_context);
	DestroyWindow(dummy_window_handle);
}

void anvlOpenGLContextInit(GraphicsContext* context) {
	context->GraphicsContextCreate		= anvlOpenGLContextCreate;
	context->GraphicsContextMakeCurrent = anvlOpenGLContextMakeCurrent;
	context->GraphicsContextDestroy		= anvlOpenGLContextDestroy;

	_pixel_format_descriptor.nSize			= sizeof(PIXELFORMATDESCRIPTOR);
	_pixel_format_descriptor.nVersion		= 1;
	_pixel_format_descriptor.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	_pixel_format_descriptor.iPixelType		= PFD_TYPE_RGBA;
	_pixel_format_descriptor.cColorBits		= 32;
	_pixel_format_descriptor.cAlphaBits		= 8;
	_pixel_format_descriptor.cDepthBits		= 24;
	_pixel_format_descriptor.cStencilBits	= 8;

	_GetOpenGLExtensionsProcs();
}

ContextData* anvlOpenGLContextCreate(NativeWindow* window)
{
	ContextData* context_data = malloc(sizeof(ContextData));
	if (!context_data) {
		return null;
	}

	context_data->device_context = window->device_context;

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
	_wglChoosePixelFormatARB(context_data->device_context, pixel_attributes, null, 1, &pixel_format, &pixel_format_count);
	if (pixel_format_count == 0) {
		return null;
	}
	SetPixelFormat(context_data->device_context, pixel_format, &_pixel_format_descriptor);

	// TODO: Improve this code to have more important attributes
	int32 graphics_context_attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};
	//

	context_data->handle = _wglCreateContextAttribsARB(context_data->device_context, null, graphics_context_attributes);
	if (!context_data->handle) {
		return null;
	}

	return context_data;
}

void anvlOpenGLContextMakeCurrent(GraphicsContext* context)
{
	wglMakeCurrent(context->data->device_context, context->data->handle);
}

void anvlOpenGLContextDestroy(GraphicsContext* context)
{
	if (context) {
		wglMakeCurrent(null, null);

		if (context->data->handle) {
			wglDeleteContext(context->data->handle);
		}

		if (context->data) {
			free(context->data);
			context->data = null;
		}

		context->GraphicsContextCreate		= null;
		context->GraphicsContextMakeCurrent = null;
		context->GraphicsContextDestroy		= null;

		free(context);
		context = null;
	}
}
