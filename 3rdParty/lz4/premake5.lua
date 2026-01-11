project "liblz4_static"
    kind "StaticLib"
    language "C"
    cdialect "C11"

    targetname "liblz4_static"

    targetdir ("../../out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("../../out/obj/%{cfg.platform}/%{cfg.buildcfg}/liblz4_static")

    files {
        "./lib/lz4.c",
        "./lib/lz4frame.c",
        "./lib/lz4hc.c",
        "./lib/xxhash.c",

        "./lib/lz4.h",
        "./lib/lz4frame.h",
        "./lib/lz4frame_static.h",
        "./lib/lz4hc.h",
        "./lib/xxhash.h"
    }

    includedirs {
        "./lib"
    }

    warnings "Extra"

    characterset "Unicode"

    defines {
        "LZ4_DLL_EXPORT=1"
    }

    filter "system:windows"
        systemversion "10.0"
        defines { "WIN32" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        fatalwarnings "On"
        defines { "_DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG" }

    filter "platforms:Win32"
        architecture "x86"

    filter "platforms:x64"
        architecture "x86_64"

    filter {}
