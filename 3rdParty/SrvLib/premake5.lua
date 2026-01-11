project "SrvLib"
    kind "StaticLib"
    language "C++"
    characterset "Unicode"

    targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    files {
        "BaseSrv.cpp",
        "ServMain.cpp",
        "SrvCtrl.cpp",
        "BaseSrv.h",
        "Service.h",
        "SrvCtrl.h"
    }

    filter "system:windows"
        systemversion "10.0"
        cppdialect "C++20"

    -- Win32/x86
    filter "platforms:x86"
        architecture "x86"
        filter "configurations:Debug"
            runtime "Debug"
            symbols "On"
            optimize "Off"
            defines { "WIN32", "_DEBUG", "_CONSOLE" }
            flags { "MultiProcessorCompile" }
        filter "configurations:Release"
            runtime "Release"
            optimize "Speed"
            defines { "WIN32", "NDEBUG", "_CONSOLE" }
            flags { "LinkTimeOptimization", "MultiProcessorCompile" }

    -- x64
    filter "platforms:x64"
        architecture "x86_64"
        filter "configurations:Debug"
            runtime "Debug"
            symbols "On"
            optimize "Off"
            defines { "_DEBUG", "_CONSOLE" }
            flags { "MultiProcessorCompile" }
        filter "configurations:Release"
            runtime "Release"
            optimize "Speed"
            defines { "NDEBUG", "_CONSOLE" }
            flags { "LinkTimeOptimization", "MultiProcessorCompile" }

    filter {}
