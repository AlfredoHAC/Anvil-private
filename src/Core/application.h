#ifndef APPLICATION_HEADER
#define APPLICATION_HEADER

#include "Core/typedefs.h"
#include "Core/event.h"

typedef struct ApplicationData ApplicationData;
typedef struct Application {
	char name[64];

	struct {
		uint16 window_width;
		uint16 window_height;
	} hints;

	ApplicationData* internal;
} Application;

bool anvlAppInit(Application* app);
void anvlAppRun(Application* app);
bool anvlAppShutdown(Application* app);

void anvlApplicationOnEvent(Event event);
void anvlApplicationOnWindowClose();

#endif // !APPLICATION_HEADER
