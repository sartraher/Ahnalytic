project "zstd"
    kind "SharedLib"
    language "C"
    cdialect "C11"

    characterset "MBCS"

    targetname "libzstd"

    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/zstd")

    files {
        "lib/common/*.c",
        "lib/compress/*.c",
        "lib/decompress/*.c",
        "lib/dictBuilder/*.c",
        "lib/legacy/*.c"
    }

    includedirs {
        "lib",
        "lib/common",
        "lib/compress",
        "lib/decompress",
        "lib/dictBuilder",
        "lib/legacy"
    }

    defines {
        "ZSTD_DLL_EXPORT=1",
        "ZSTD_MULTITHREAD=1",
        "ZSTD_LEGACY_SUPPORT=5",
        "_CRT_SECURE_NO_WARNINGS"
    }

    filter "system:windows"
        systemversion "latest"

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

    filter "platforms:x86"
        architecture "x86"

    filter "platforms:x64"
        architecture "x86_64"

    filter {}
