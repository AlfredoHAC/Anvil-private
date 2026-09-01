#ifndef ANVIL_LAYER_HEADER
#define ANVIL_LAYER_HEADER

#include "Anvil/Window/event.h"

typedef struct AnvlLayer AnvlLayer;

typedef void (*LayerOnUpdateFn)(AnvlLayer* layer);
typedef void (*LayerOnRenderFn)(AnvlLayer* layer, void* user_data);
typedef void (*LayerOnEventFn)(AnvlLayer* layer, AnvlEvent* event);

struct AnvlLayer
{
    const char*     name;
    LayerOnUpdateFn on_update;
    LayerOnRenderFn on_render;
    LayerOnEventFn  on_event;
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
