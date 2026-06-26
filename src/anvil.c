#include "anvlpch.h" // IWYU pragma: keep

#include "Core/application.h"

int main()
{
    const ApplicationOptions opts = {
        .name   = "AnvilFramework",
        .width  = 1280,
        .height = 720,
    };

    Application* app = anvl_application_init(opts);
    if (!app) return 1;

    anvl_application_run(app);

    anvl_application_shutdown(app);
    app = NULL;

    return 0;
}
