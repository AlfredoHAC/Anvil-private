#ifndef ANVIL_LAYER_HEADER
#define ANVIL_LAYER_HEADER

#include "Anvil/Window/event.h"

typedef struct Layer Layer;

typedef void (*LayerOnUpdateFn)(Layer* layer);
typedef void (*LayerOnEventFn)(Layer* layer, Event* event);

struct Layer
{
    const char*     name;
    LayerOnUpdateFn on_update;
    LayerOnEventFn  on_event;
};

void   anvl_layer_stack_push(Layer* layer);
void   anvl_layer_stack_pop();
void   anvl_layer_stack_remove(Layer* layer);
uint32 anvl_layer_stack_length();
void   anvl_layer_stack_clear();
void   anvl_layer_stack_dispatch_event(Event* event);
void   anvl_layer_stack_call_update();

#endif // !ANVIL_LAYER_HEADER
