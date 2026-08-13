#ifndef ANVIL_GRAPHICS_REQUIREMENTS_HEADER
#define ANVIL_GRAPHICS_REQUIREMENTS_HEADER

#include "Anvil/Core/types.h"

typedef struct AnvlGraphicRequirements
{
    struct AnvlPixelFormat
    {
        int32 red_bits;
        int32 green_bits;
        int32 blue_bits;
        int32 alpha_bits;
        int32 depth_bits;
        int32 stencil_bits;
    } pixel_format;

    int32 sample_count;
} AnvlGraphicRequirements;

#endif // !ANVIL_GRAPHICS_REQUIREMENTS_HEADER
