#include "anvlpch.h"

#include "Core/layer.h"

#define LAYER_STACK_MAX_LENGTH 32

static Layer* layer_stack[LAYER_STACK_MAX_LENGTH] = {0};
static uint8  layer_stack_length                  = 0;

void anvl_layer_stack_push(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);
    ANVIL_ASSERT(layer_stack_length + 1 <= LAYER_STACK_MAX_LENGTH);

    layer_stack[layer_stack_length] = layer;
    layer_stack_length += 1;
}

void anvl_layer_stack_pop()
{
    ANVIL_ASSERT(layer_stack_length > 0);

    layer_stack[layer_stack_length] = NULL;
    layer_stack_length -= 1;
}

void anvl_layer_stack_remove(Layer* layer)
{
    ANVIL_ASSERT(layer != NULL);
    ANVIL_ASSERT(layer_stack_length > 0);

    int8 layer_index = 0;
    for (int8 i = (int8)layer_stack_length - 1; i >= 0; --i)
    {
        if (layer_stack[i] == layer)
        {
            layer_index = i;
            break;
        }
    }

    for (int8 i = layer_index; i < (int8)layer_stack_length; ++i)
    {
        layer_stack[i] = layer_stack[i + 1];
    }

    layer_stack_length -= 1;
}

// clang-format off
uint32 anvl_layer_stack_length()
{
    return layer_stack_length;
}

void anvl_layer_stack_clear()
{
    layer_stack_length = 0;
}
// clang-format on

void anvl_layer_stack_dispatch_event(Event* event)
{
    for (int8 i = layer_stack_length - 1; i >= 0; --i)
    {
        if (layer_stack[i] == NULL || layer_stack[i]->on_event == NULL) { continue; }

        layer_stack[i]->on_event(layer_stack[i], event);

        if (event->handled) { break; }
    }
}

void anvl_layer_stack_call_update()
{
    for (int8 i = layer_stack_length - 1; i >= 0; --i)
    {
        if (layer_stack[i] == NULL || layer_stack[i]->on_update == NULL) { continue; }

        layer_stack[i]->on_update(layer_stack[i]);
    }
}
