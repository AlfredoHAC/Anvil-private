#include "anvlpch.h"

#include "Core/application.h"
#include "Platform/platform.h"

#include "Tools/logger.h"

static bool app_running = false;

typedef struct Application
{
    NativeWindow* window;
} Application;

Application* anvlAppInit(const ApplicationHints hints)
{
    Application* app = malloc(sizeof(Application));
    if (!app) return NULL;
    anvlLoggerSetLevel(Trace);

    ANVIL_CORE_INFO("Starting application.");
    ANVIL_CORE_INFO("-> Name: %s", hints.name);
    ANVIL_CORE_INFO("-> Window title: %s", hints.name);
    ANVIL_CORE_INFO("-> Window width: %d", hints.width);
    ANVIL_CORE_INFO("-> Window height: %d", hints.height);

    app->window = anvlPlatformWindowCreate(hints.name, hints.width, hints.height);
    if (!app->window)
    {
        free(app);
        return NULL;
    }

    anvlPlatformSetWindowEventCallback(app->window, anvlApplicationOnEvent);
    anvlPlatformWindowShow(app->window);

    app_running = true;
    return app;
}

void anvlAppRun(Application* app)
{
    if(!app) return;

    while (app_running)
    {
        anvlPlatformWindowUpdate(app->window);
    }
}

void anvlAppShutdown(Application* app)
{
    if (!app) return;

    anvlPlatformWindowDestroy(app->window);

    free(app);
}

void anvlApplicationOnEvent(Event event)
{
    ANVIL_CORE_DEBUG("Event captured!");

    switch (event.type)
    {
    case WindowClose:
        anvlApplicationOnWindowClose();
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

void anvlApplicationOnWindowClose()
{
    app_running = false;
}
