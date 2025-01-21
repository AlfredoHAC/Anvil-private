#include "anvlpch.h"

#include "Core/application.h"
#include "Platform/platform.h"

struct ApplicationInternalData {
	NativeWindow* window;
};

bool anvlAppInit(ApplicationData* app) {
	app->internal = malloc(sizeof(ApplicationInternalData));
	if (!app->internal) {
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

	return true;
}

void anvlAppRun(ApplicationData* app)
{
	while (true) {
		anvlPlatformWindowUpdate(app->internal->window);
	}
}

bool anvlAppShutdown(ApplicationData* app)
{
	anvlPlatformWindowDestroy(app->internal->window);

	if (app->internal) {
		free(app->internal);
	}

	return true;
}

