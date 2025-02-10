#include "anvlpch.h"

#include "Core/application.h"
#include "Platform/platform.h"

static bool app_running;

struct ApplicationData {
	NativeWindow* window;
};

bool anvlAppInit(Application* app) {
	app->internal = malloc(sizeof(ApplicationData));
	if (!app->internal) {
		free(app->internal);
		return false;
	}

	char app_window_title[64];
	strncpy(app_window_title, app->name, sizeof(app_window_title) - 1);
	app_window_title[63] = '\0';

	printf("-> Name: %s\n", app->name);
	printf("-> Window title: %s\n", app_window_title);
	printf("-> Window width: %d\n", app->hints.window_width);
	printf("-> Window height: %d\n", app->hints.window_height);

	app->internal->window = anvlPlatformWindowCreate(app_window_title, app->hints.window_width, app->hints.window_height);
	if (!app->internal->window) {
		return false;
	}

	anvlPlatformSetWindowEventCallback(app->internal->window, anvlApplicationOnEvent);
	anvlPlatformWindowShow(app->internal->window);

	app_running = true;
	return true;
}

void anvlAppRun(Application* app)
{
	while (app_running) {
		anvlPlatformWindowUpdate(app->internal->window);
	}
}

bool anvlAppShutdown(Application* app)
{
	if (app) {
		anvlPlatformWindowDestroy(app->internal->window);

		if (app->internal) {
			free(app->internal);
			app->internal = null;
		}
	}

	return true;
}

void anvlApplicationOnEvent(Event event)
{
	printf("Event captured!\n");

	switch (event.type) {
	case WindowClose:
		anvlApplicationOnWindowClose();
		break;
	case WindowResize:
		printf("Window resize: %dx%d\n", event.window_resize.width, event.window_resize.height);
		break;
	case KeyPress:
		printf("Key press: %d\n", event.key_press.key_code);
		break;
	case KeyRelease:
		printf("Key release: %d\n", event.key_release.key_code);
		break;
	case MouseMove:
		printf("Mouse move: (%.1f,%.1f)\n", event.mouse_move.x, event.mouse_move.y);
		break;
	case MouseButtonClick:
		printf("Mouse button click: %d (%.1f,%.1f)\n", event.mouse_button_click.button_code, event.mouse_button_click.x, event.mouse_button_click.y);
		break;
	case MouseButtonRelease:
		printf("Mouse button release: %d (%.1f,%.1f)\n", event.mouse_button_release.button_code, event.mouse_button_release.x, event.mouse_button_release.y);
		break;
	case MouseScroll:
		printf("Mouse scroll: (%.1f,%.1f)\n", event.mouse_scroll.x_offset, event.mouse_scroll.y_offset);
		break;
	}
}

void anvlApplicationOnWindowClose()
{
	app_running = false;
}

