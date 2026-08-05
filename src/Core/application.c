#include "anvlpch.h"

#include "Core/application.h"
#include "Core/layer.h"
#include "Windowing/window.h"

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

Application* anvl_application_init(const ApplicationOptions opts)
{
    Application* app = malloc(sizeof(Application));
    if (!app) { return NULL; }
    anvl_logger_set_level(ANVL_LOG_LEVEL_TRACE);

    ANVIL_CORE_INFO("Starting application.");
    ANVIL_CORE_INFO("-> Name: %s", opts.name);
    ANVIL_CORE_INFO("-> Window title: %s", opts.name);
    ANVIL_CORE_INFO("-> Window width: %d", opts.width);
    ANVIL_CORE_INFO("-> Window height: %d", opts.height);

    app->window = anvl_platform_window_create(opts.name, opts.width, opts.height);
    if (!app->window)
    {
        free(app);
        return NULL;
    }

    anvl_layer_stack_push(&app_layer);

    anvl_platform_window_set_event_callback(app->window, anvl_layer_stack_dispatch_event);
    anvl_platform_window_show(app->window);

    app_running = true;
    return app;
}

void anvl_application_run(Application* app)
{
    ANVIL_ASSERT(app != NULL);

    while (app_running)
    {
        anvl_layer_stack_call_update();
        anvl_platform_window_update(app->window);
    }
}

void anvl_application_shutdown(Application* app)
{
    ANVIL_ASSERT(app != NULL);

    anvl_layer_stack_clear();
    anvl_platform_window_unset_event_callback(app->window);
    anvl_platform_window_destroy(app->window);

    free(app);
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
