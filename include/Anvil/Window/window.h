#ifndef ANVIL_WINDOW_HEADER
#define ANVIL_WINDOW_HEADER

#include "Anvil/Core/types.h"
#include "Anvil/Graphics/requirements.h"
#include "Anvil/Window/event.h"

// Platform native window
typedef struct AnvlWindow AnvlWindow;

// AnvlEvent callback function pointer type
typedef void (*EventCallbackFn)(AnvlEvent* event);

typedef struct AnvlWindowOptions
{
    const char*             title;
    uint16                  width;
    uint16                  height;
    AnvlGraphicRequirements requirements;
} AnvlWindowOptions;

AnvlWindow* anvl_window_create(const AnvlWindowOptions window_options);
void        anvl_window_show(AnvlWindow* window);
void        anvl_window_update(AnvlWindow* window);
void        anvl_window_destroy(AnvlWindow* window);

void* anvl_window_get_handle(const AnvlWindow* window);

#endif // !ANVIL_WINDOW_HEADER
