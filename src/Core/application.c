#include "application.h"

#include "Core/base.h"
#include "Platform/platform.h"

#include <stdio.h>
#include <string.h>

bool anvlAppInit(const char* app_name, uint16 window_width, uint16 window_height) {
    char app_window_title[50];
    strncpy(app_window_title, app_name, sizeof(app_window_title));

    printf("-> Name: %s\n", app_name);
    printf("-> Window title: %s\n", app_window_title);
    printf("-> Window width: %d\n", window_width);
    printf("-> Window height: %d\n", window_height);
    //

    if (!PlatformInit(app_window_title, window_width, window_height)) {
        return false;
    }

    return true;
}

