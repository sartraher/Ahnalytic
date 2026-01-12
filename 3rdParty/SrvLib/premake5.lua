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
			
	filter "system:linux or system:macosx"
		buildoptions {
			"-std=c++17",
			"-I/usr/include/x86_64-linux-gnu/c++/13"
		}

    filter {}
