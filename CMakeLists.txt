cmake_minimum_required(VERSION 3.20)
project(Anvil LANGUAGES C)

add_library(Anvil STATIC
    src/anvil.c
    src/anvlpch.c
    src/Core/application.c
    src/Core/layer.c
    src/Tools/logger.c
)

# ─────────────────────────────────────────────
#  Platform-specific sources
# ─────────────────────────────────────────────
if(WIN32)
    target_sources(Anvil PRIVATE
        src/FileIO/Windows/win32_file.c
        src/Windowing/Windows/win32_window.c
    )
elseif(UNIX AND NOT APPLE)
    target_sources(Anvil PRIVATE
        src/FileIO/Linux/posix_file.c
        src/Windowing/Linux/linux_window.c
        src/Windowing/Linux/X11/x11_backend.c
        src/Windowing/Linux/Wayland/wayland_backend.c
        src/Windowing/Linux/Wayland/xdg_shell_client_protocol.c
        src/Windowing/Linux/Wayland/xdg_shell_decoration_protocol.c
    )
endif()

target_include_directories(Anvil PUBLIC src)

target_precompile_headers(Anvil PRIVATE src/anvlpch.h)

target_compile_definitions(Anvil PRIVATE
    $<$<CONFIG:Debug>:ANVIL_CONFIG_DEBUG>
    $<$<CONFIG:RelWithDebInfo>:ANVIL_CONFIG_OPTIMIZED>
    $<$<CONFIG:Release>:ANVIL_CONFIG_RELEASE>
)

# ─────────────────────────────────────────────
#  Windows
# ─────────────────────────────────────────────
if(WIN32)
    target_compile_definitions(Anvil PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_link_libraries(Anvil PRIVATE user32 gdi32 opengl32)
    target_compile_options(Anvil PRIVATE /wd4201)
endif()

# ─────────────────────────────────────────────
#  Linux
# ─────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    target_compile_definitions(Anvil PRIVATE _GNU_SOURCE)
    find_package(X11)
    if(X11_FOUND)
        target_link_libraries(Anvil PRIVATE X11 xcb)
    endif()
    find_package(WaylandClient QUIET)
    if(WaylandClient_FOUND)
        target_link_libraries(Anvil PRIVATE wayland-client)
    endif()
endif()

# ─────────────────────────────────────────────
#  Sanitizers (Debug only)
# ─────────────────────────────────────────────
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(Anvil PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
    target_link_options(Anvil PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address>
    )
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(Anvil PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=fuzzer>
    )
    target_link_options(Anvil PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=fuzzer>
    )
endif()
