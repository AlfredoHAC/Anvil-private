#ifndef ANVIL_APPLICATION_HEADER
#define ANVIL_APPLICATION_HEADER

#include "Anvil/Window/window.h"

typedef void (*AnvlRenderFrameFn)(void*);

typedef struct AnvlApplication AnvlApplication;

AnvlApplication* anvl_application_init(AnvlWindow* window);
void             anvl_application_run(AnvlApplication* app);
void             anvl_application_shutdown(AnvlApplication* app);

void anvl_application_set_window(AnvlApplication* app, AnvlWindow* window);
void anvl_application_set_render_frame_begin(AnvlApplication*  app,
                                             AnvlRenderFrameFn frame_begin_func,
                                             void*             renderer);
void anvl_application_set_render_frame_end(AnvlApplication*  app,
                                           AnvlRenderFrameFn frame_end_func,
                                           void*             renderer);

#endif // !ANVIL_APPLICATION_HEADER
