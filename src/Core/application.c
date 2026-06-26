#include "anvlpch.h"

#include "Core/application.h"
#include "Platform/platform.h"

#include "Tools/logger.h"

static bool app_running = false;

typedef struct Application
{
    NativeWindow* window;
} Application;

Application* anvl_application_init(const ApplicationOptions opts)
{
    Application* app = malloc(sizeof(Application));
    if (!app) return NULL;
    anvl_logger_set_level(Trace);

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

    anvl_platform_set_window_event_callback(app->window, anvl_application_on_event);
    anvl_platform_window_show(app->window);

    app_running = true;
    return app;
}

void anvl_application_run(Application* app)
{
    if(!app) return;

    while (app_running)
    {
        anvl_platform_window_update(app->window);
    }
}

void anvl_application_shutdown(Application* app)
{
    if (!app) return;

    anvl_platform_window_destroy(app->window);

    free(app);
}

void anvl_application_on_event(Event event)
{
    ANVIL_CORE_DEBUG("Event captured!");

    switch (event.type)
    {
    case WindowClose:
        anvl_application_on_window_close();
        break;
    case WindowResize:
        ANVIL_CORE_DEBUG("Window resize: %dx%d", event.window_resize.width, event.window_resize.height);
        break;
    case KeyPress:
        ANVIL_CORE_DEBUG("Key press: %d", event.key_press.key_code);
        break;
    case KeyRelease:
        ANVIL_CORE_DEBUG("Key release: %d", event.key_release.key_code);
        break;
    case MouseMove:
        ANVIL_CORE_DEBUG("Mouse move: (%.1f,%.1f)", event.mouse_move.x, event.mouse_move.y);
        break;
    case MouseButtonClick:
        ANVIL_CORE_DEBUG("Mouse button click: %d (%.1f,%.1f)", event.mouse_button_click.button_code,
                         event.mouse_button_click.x, event.mouse_button_click.y);
        break;
    case MouseButtonRelease:
        ANVIL_CORE_DEBUG("Mouse button release: %d (%.1f,%.1f)", event.mouse_button_release.button_code,
                         event.mouse_button_release.x, event.mouse_button_release.y);
        break;
    case MouseScroll:
        ANVIL_CORE_DEBUG("Mouse scroll: (%.1f,%.1f)", event.mouse_scroll.x_offset, event.mouse_scroll.y_offset);
        break;
    default:
        break;
    }

    event.handled = true;
}

void anvl_application_on_window_close()
{
    app_running = false;
}
