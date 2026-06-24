#ifndef ANVL_APPLICATION_HEADER
#define ANVL_APPLICATION_HEADER

#include "Platform/event.h"
#include "Platform/typedefs.h"

typedef struct Application Application;
typedef struct
{
    const char* name;
    uint16 width;
    uint16 height;
} ApplicationHints;

Application* anvlAppInit(const ApplicationHints hints);
void anvlAppRun(Application* app);
void anvlAppShutdown(Application* app);

void anvlApplicationOnEvent(Event event);
void anvlApplicationOnWindowClose();

#endif // !ANVL_APPLICATION_HEADER
