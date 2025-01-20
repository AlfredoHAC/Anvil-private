#ifndef APPLICATION_HEADER
#define APPLICATION_HEADER

#include "Core/typedefs.h"
#include "Platform/platform.h"

#include <stdbool.h>

typedef struct ApplicationInternalData ApplicationInternalData;

typedef struct ApplicationData {
	char name[64];

	struct {
		uint16 window_width;
		uint16 window_height;
	} hints;

	ApplicationInternalData* internal;
} ApplicationData;

bool anvlAppInit(ApplicationData* app);
void anvlAppRun(ApplicationData* app);
bool anvlAppShutdown(ApplicationData* app);

#endif // !APPLICATION_HEADER
