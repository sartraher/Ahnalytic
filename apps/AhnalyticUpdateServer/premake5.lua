project "AhnalyticUpdateServer"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir ("../../out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("../../out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "AhnalyticBase" }

    -- Virtual folders for Visual Studio
    vpaths {
        ["Source Files"] = { "**.cpp" },
        ["Header Files"] = { "**.h", "**.hpp" }
    }

    files {
        "*.cpp",
        "*.hpp",
        "*.h",
		"resource.rc",
		"../../images/update.ico"
    }

    includedirs {
        "../../libs",
        "../../3rdParty/SrvLib",
        "../../3rdParty"
    }

    links {
        "SrvLib",
        "AhnalyticBase"
    }

    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"
        links { "ws2_32" }  -- optional for networking

    filter "system:linux or system:macosx"
        pic "On"
        links { "pthread" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "_DEBUG", "_CONSOLE" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG", "_CONSOLE" }
        flags { "linktimeoptimization" }

    filter "platforms:x64"
        vectorextensions "AVX2"

    filter {}