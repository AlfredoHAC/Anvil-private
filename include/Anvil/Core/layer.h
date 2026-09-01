#ifndef ANVIL_LAYER_HEADER
#define ANVIL_LAYER_HEADER

#include "Anvil/Window/event.h"

typedef struct AnvlLayer AnvlLayer;

typedef void (*AnvlLayerOnUpdateFn)(AnvlLayer* layer);
typedef void (*AnvlLayerOnRenderFn)(AnvlLayer* layer, void* user_data);
typedef void (*AnvlLayerOnEventFn)(AnvlLayer* layer, AnvlEvent* event);

struct AnvlLayer
{
    const char*     name;
    AnvlLayerOnUpdateFn on_update;
    AnvlLayerOnRenderFn on_render;
    AnvlLayerOnEventFn  on_event;
};

void   anvl_layer_stack_push(AnvlLayer* layer);
void   anvl_layer_stack_pop();
void   anvl_layer_stack_remove(AnvlLayer* layer);
uint32 anvl_layer_stack_length();
void   anvl_layer_stack_clear();
void   anvl_layer_stack_dispatch_event(AnvlEvent* event);
void   anvl_layer_stack_call_update();
void   anvl_layer_stack_call_render(void* user_data);

#endif // !ANVIL_LAYER_HEADER
