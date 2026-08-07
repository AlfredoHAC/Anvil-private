#include "anvlpch.h"

#include "Core/application.h"
#include "Core/layer.h"

#include "Tools/logger.h"

typedef struct Application
{
    NativeWindow* window;
} Application;

static void _on_application_event(Layer* layer, Event* event);
static void _on_application_window_close();

static bool  app_running = false;
static Layer app_layer   = {
    .name      = "Application_Layer",
    .on_update = NULL,
    .on_event  = _on_application_event,
};

Application* anvl_application_init(NativeWindow* window)
{
    ANVIL_CORE_INFO("Starting application.");

    Application* app = malloc(sizeof(Application));
    if (!app) { return NULL; }
    memset(app, 0, sizeof(Application));

    anvl_logger_set_level(ANVL_LOG_LEVEL_TRACE);

    if (window) { app->window = window; }

    anvl_layer_stack_push(&app_layer);

    app_running = true;
    return app;
}

void anvl_application_run(Application* app)
{
    ANVIL_ASSERT(app != NULL);

    if (!app->window)
    {
        ANVIL_CORE_ERROR("Application can not run without a window.");
        return;
    }

    while (app_running)
    {
        anvl_layer_stack_call_update();
        anvl_window_update(app->window);
    }
}

void anvl_application_shutdown(Application* app)
{
    ANVIL_ASSERT(app != NULL);

    anvl_layer_stack_clear();

    free(app);
}

void anvl_application_window_set(Application* app, NativeWindow* window)
{
    ANVIL_ASSERT(app != NULL && window != NULL);

    app->window = window;
}

static void _on_application_event(Layer* layer, Event* event)
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
