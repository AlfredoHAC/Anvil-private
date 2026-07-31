#ifndef ANVL_APPLICATION_HEADER
#define ANVL_APPLICATION_HEADER

#include "Core/typedefs.h"

typedef struct Application Application;
typedef struct
{
    const char* name;
    uint16      width;
    uint16      height;
} ApplicationOptions;

Application* anvl_application_init(const ApplicationOptions opts);
void         anvl_application_run(Application* app);
void         anvl_application_shutdown(Application* app);

#endif // !ANVL_APPLICATION_HEADER
