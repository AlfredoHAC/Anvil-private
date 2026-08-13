#ifndef ANVIL_GRAPHICS_CONTEXT_HEADER
#define ANVIL_GRAPHICS_CONTEXT_HEADER

#include "Graphics/requirements.h"

typedef struct AnvlGraphicsContext AnvlGraphicsContext;

AnvlGraphicsContext* anvl_graphics_context_create(
    void*                         window_handle,
    const AnvlGraphicRequirements requirements);
void anvl_graphics_context_destroy(AnvlGraphicsContext* context);

void* anvl_graphics_context_get_handle(const AnvlGraphicsContext* context);

#endif // !ANVIL_GRAPHICS_CONTEXT_HEADER
