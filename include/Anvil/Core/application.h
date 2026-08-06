#ifndef ANVIL_APPLICATION_HEADER
#define ANVIL_APPLICATION_HEADER

#include "Anvil/Core/types.h"

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

#endif // !ANVIL_APPLICATION_HEADER
