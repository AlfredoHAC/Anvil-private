#include "anvlpch.h"

#include "Core/application.h"
#include "Platform/platform.h"

// TODO: rename ApplicationInternalData -> ApplicationData
struct ApplicationInternalData {
	NativeWindow* window;
	GraphicsContext* graphics_context;
};
//

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

	app->internal->graphics_context = anvlGraphicsContextCreate(app->hints.graphics_api, app->internal->window);
	anvlGraphicsContextMakeCurrent(app->internal->graphics_context);

	GraphicsContextAPIInfo api_info = anvlGraphicsContextGetAPIInfo(app->internal->graphics_context);
	printf("\nGraphics Context\n");
	printf("-> Version: %s\n", api_info.version);
	printf("-> Vendor: %s\n", api_info.vendor);
	printf("-> Renderer: %s\n", api_info.renderer);
	printf("-> Shading Language version: %s\n", api_info.shading_language);

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
	if (app) {
		anvlGraphicsContextDestroy(app->internal->graphics_context);
		app->internal->graphics_context = null;
		anvlPlatformWindowDestroy(app->internal->window);
		app->internal->window = null;

		if (app->internal) {
			free(app->internal);
			app->internal = null;
		}
	}

	return true;
}

