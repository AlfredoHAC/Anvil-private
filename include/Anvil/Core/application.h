#ifndef ANVIL_APPLICATION_HEADER
#define ANVIL_APPLICATION_HEADER

#include "Anvil/Window/window.h"

typedef struct Application Application;

Application* anvl_application_init(NativeWindow* window);
void         anvl_application_run(Application* app);
void         anvl_application_shutdown(Application* app);

void anvl_application_window_set(Application* app, NativeWindow* window);

#endif // !ANVIL_APPLICATION_HEADER
