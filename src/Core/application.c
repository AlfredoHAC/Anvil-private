#include "anvlpch.h"

#include "Core/application.h"
#include "Core/layer.h"

#include "Tools/logger.h"

typedef struct AnvlApplication
{
    AnvlWindow* window;
} AnvlApplication;

static void _on_application_event(AnvlLayer* layer, AnvlEvent* event);
static void _on_application_window_close();

static bool  app_running = false;
static AnvlLayer app_layer   = {
    .name      = "Application_Layer",
    .on_update = NULL,
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
        anvl_layer_stack_call_update();
        anvl_window_update(app->window);
    }
}

void anvl_application_shutdown(AnvlApplication* app)
{
    ANVIL_ASSERT(app != NULL);

    anvl_layer_stack_clear();

    free(app);
}

void anvl_application_window_set(AnvlApplication* app, AnvlWindow* window)
{
    ANVIL_ASSERT(app != NULL && window != NULL);

    app->window = window;
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
