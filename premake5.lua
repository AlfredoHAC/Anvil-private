workspace "ForgeCore"

    architecture "x86_64"
    startproject "Anvil"

    configurations {
        "Debug",
        "Optimized",
        "Release"
    }

project "Anvil"
    kind "ConsoleApp"
    language "C"

    pchheader "anvlpch.h"
    pchsource "./src/anvlpch.c"

    targetdir "./bin/%{cfg.buildcfg}/"
    objdir "./bin/obj/%{cfg.buildcfg}/"

    files {
        "./src/**.h",
        "./src/**.c"
    }

    includedirs {
        "./src/",
        "./src/**"
    }

    defines {
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:Windows"
        system "windows"

    filter "system:Unix"
        system "linux"

    filter "system:windows"
        systemversion "latest"
        cdialect "C11"

        links {
            "user32",
            "gdi32",
            "opengl32",
        }

        removefiles {
            "./**/Linux/**"
        }

    filter "system:linux"
        systemversion "latest"
        cdialect "gnu11"

        removefiles {
            "./**/Windows/**"
        }

    filter "configurations:Debug"
        defines "ANVIL_CONFIG_DEBUG"
        runtime "Debug"
        symbols "On"

        sanitize {
            "Address",
            "Fuzzer"
        }

        editandcontinue "Off"

        flags {
            "NoIncrementalLink",
            "NoRuntimeChecks"
        }

    filter "configurations:Optimized"
        defines "ANVIL_CONFIG_OPTIMIZED"
        runtime "Release"
        optimize "On"

    filter "configurations:Release"
        defines "ANVIL_CONFIG_RELEASE"
        runtime "Release"
        optimize "Full"
