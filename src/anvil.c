#include "anvlpch.h" // IWYU pragma: keep

#include "Core/application.h"

int main()
{
    const ApplicationHints hints = {
        .name   = "AnvilFramework",
        .width  = 1280,
        .height = 720,
    };

    Application* app = anvlAppInit(hints);
    if (!app) return 1;

    anvlAppRun(app);

    anvlAppShutdown(app);
    app = NULL;

    return 0;
}
