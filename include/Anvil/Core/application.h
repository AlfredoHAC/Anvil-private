#ifndef ANVIL_APPLICATION_HEADER
#define ANVIL_APPLICATION_HEADER

#include "Anvil/Window/window.h"

typedef struct AnvlApplication AnvlApplication;

AnvlApplication* anvl_application_init(AnvlWindow* window);
void         anvl_application_run(AnvlApplication* app);
void         anvl_application_shutdown(AnvlApplication* app);

void anvl_application_window_set(AnvlApplication* app, AnvlWindow* window);

#endif // !ANVIL_APPLICATION_HEADER
