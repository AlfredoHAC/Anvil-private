#ifndef ANVIL_WINDOW_HEADER
#define ANVIL_WINDOW_HEADER

#include "Anvil/Core/types.h"
#include "Anvil/Window/event.h"

// Platform native window
typedef struct AnvlWindow AnvlWindow;

// AnvlEvent callback function pointer type
typedef void (*AnvlEventCallbackFn)(AnvlEvent* event);

typedef enum AnvlWindowGraphicsMode
{
    ANVL_WINDOW_GRAPHICS_MODE_NONE = 0,
    ANVL_WINDOW_GRAPHICS_MODE_OPENGL
} AnvlWindowGraphicsMode;

typedef struct AnvlWindowOptions
{
    const char* title;
    uint16      width;
    uint16      height;

    AnvlWindowGraphicsMode graphics_mode;
    struct AnvlGraphicRequirements
    {
        int32 red_bits;
        int32 green_bits;
        int32 blue_bits;
        int32 alpha_bits;
        int32 depth_bits;
        int32 stencil_bits;
        int32 sample_count;

        int32 major_version;
        int32 minor_version;
    } graphics_requirements;
} AnvlWindowOptions;

typedef struct AnvlWindowNativeHandle
{
    uintptr primary;
    uintptr secondary;
} AnvlWindowNativeHandle;

AnvlWindow* anvl_window_create(const AnvlWindowOptions window_options);
void        anvl_window_show(AnvlWindow* window);
void        anvl_window_update(AnvlWindow* window);
void        anvl_window_destroy(AnvlWindow* window);

AnvlWindowNativeHandle anvl_window_get_native_handle(const AnvlWindow* window);
uint16                 anvl_window_get_width(const AnvlWindow* window);
uint16                 anvl_window_get_height(const AnvlWindow* window);
void                   anvl_window_present(const AnvlWindow* window);

#endif // !ANVIL_WINDOW_HEADER
