project "AhnalyticScannerServer"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "AhnalyticBase" }

    -- Virtual folder grouping inside VS solution (optional)
    vpaths {
        ["Source Files"] = { "**.cpp" },
        ["Header Files"] = { "**.h", "**.hpp" }
    }

    files {
        "*.cpp",
        "*.hpp",
        "*.h",
		"../../images/logo.ico"
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
        links { "ws2_32" }  -- if you need WinSock
		files {
			"resource.rc"
		}

    filter "system:linux or system:macosx"
        pic "On"
        links { "pthread" }  -- standard threading library

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