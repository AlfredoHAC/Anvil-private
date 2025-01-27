#ifndef GRAPHICS_HEADER
#define GRAPHICS_HEADER

#include "Core/typedefs.h"

typedef enum GraphicsAPI {
	None = 0,
	OpenGL,
	OpenGL_ES, // TODO: Remove this
	Vulkan,
	DirectX11,
	DirectX12
} GraphicsAPI;

#endif // !GRAPHICS_HEADER

