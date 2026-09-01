#include "anvlpch.h"

#include "Core/application.h"
#include "Core/layer.h"

#include "Tools/logger.h"

typedef struct AnvlApplication
{
    AnvlWindow* window;

    AnvlRenderFrameFn frame_begin_func;
    AnvlRenderFrameFn frame_end_func;
    void*             frame_begin_func_renderer;
    void*             frame_end_func_renderer;
} AnvlApplication;

static void _on_application_event(AnvlLayer* layer, AnvlEvent* event);
static void _on_application_window_close();

static bool      app_running = false;
static AnvlLayer app_layer   = {
    .name      = "Application_Layer",
    .on_update = NULL,
    .on_render = NULL,
    .on_event  = _on_application_event,
};

AnvlApplication* anvl_application_init(AnvlWindow* window)
{
    ANVIL_CORE_INFO("Starting application.");

    AnvlApplication* app = malloc(sizeof(AnvlApplication));
    if (!app) { return NULL; }
    memset(app, 0, sizeof(AnvlApplication));

    anvl_logger_set_level(ANVL_LOG_LEVEL_TRACE);

    if (window) { app->window = window; }

    anvl_layer_stack_push(&app_layer);

    app_running = true;
    return app;
}

void anvl_application_run(AnvlApplication* app)
{
    ANVIL_ASSERT(app != NULL);

    if (!app->window)
    {
        ANVIL_CORE_ERROR("AnvlApplication can not run without a window.");
        return;
    }

    while (app_running)
    {
        anvl_window_update(app->window);
        anvl_layer_stack_call_update();
        if (app->frame_begin_func && app->frame_end_func)
        {
            app->frame_begin_func(app->frame_begin_func_renderer);

            anvl_layer_stack_call_render(app->frame_begin_func_renderer);

            app->frame_end_func(app->frame_end_func_renderer);
        }
    }
}

void anvl_application_shutdown(AnvlApplication* app)
{
    ANVIL_ASSERT(app != NULL);

    anvl_layer_stack_clear();

    free(app);
}

void anvl_application_set_window(AnvlApplication* app, AnvlWindow* window)
{
    ANVIL_ASSERT(app != NULL && window != NULL);

    app->window = window;
}

void anvl_application_set_render_frame_begin(AnvlApplication*  app,
                                             AnvlRenderFrameFn frame_begin_func,
                                             void*             renderer)
{
    ANVIL_ASSERT(app != NULL && frame_begin_func != NULL);

    app->frame_begin_func          = frame_begin_func;
    app->frame_begin_func_renderer = renderer;
}

void anvl_application_set_render_frame_end(AnvlApplication*  app,
                                           AnvlRenderFrameFn frame_end_func,
                                           void*             renderer)
{
    ANVIL_ASSERT(app != NULL && frame_end_func != NULL);

    app->frame_end_func          = frame_end_func;
    app->frame_end_func_renderer = renderer;
}

static void _on_application_event(AnvlLayer* layer, AnvlEvent* event)
{
    if (event->type == ANVL_EVENT_TYPE_WINDOW_CLOSE)
    {
        _on_application_window_close();

        event->handled = true;
    }
}

// clang-format off
static void _on_application_window_close()
{
    app_running = false;
}
// clang-format on
