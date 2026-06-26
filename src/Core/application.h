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
} ApplicationOptions;

Application* anvl_application_init(const ApplicationOptions opts);
void anvl_application_run(Application* app);
void anvl_application_shutdown(Application* app);

void anvl_application_on_event(Event event);
void anvl_application_on_window_close();

#endif // !ANVL_APPLICATION_HEADER
