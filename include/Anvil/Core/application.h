#ifndef ANVIL_APPLICATION_HEADER
#define ANVIL_APPLICATION_HEADER

#include "Anvil/Window/window.h"

typedef void (*RenderFrameFn)(void*);

typedef struct AnvlApplication AnvlApplication;

AnvlApplication* anvl_application_init(AnvlWindow* window);
void             anvl_application_run(AnvlApplication* app);
void             anvl_application_shutdown(AnvlApplication* app);

void anvl_application_window_set(AnvlApplication* app, AnvlWindow* window);
void anvl_application_render_frame_begin_set(AnvlApplication* app,
                                             RenderFrameFn    frame_begin_func,
                                             void*            renderer);
void anvl_application_render_frame_end_set(AnvlApplication* app,
                                           RenderFrameFn    frame_end_func,
                                           void*            renderer);

#endif // !ANVIL_APPLICATION_HEADER
