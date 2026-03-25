project "spdlog"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    files {
        "include/**.h",
        "src/**.cpp"
    }

    includedirs {
        "include",
		"../../3rdParty/mimalloc/include"
    }

    defines {
        "SPDLOG_COMPILED_LIB"
    }
	
	links { "mimalloc" }
	defines { "MI_MALLOC_OVERRIDE" }

    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        runtime "Release"
        optimize "On"
		
	filter "system:windows"
		systemversion "latest"
		buildoptions { "/utf-8" }
		
	filter "system:linux"
		buildoptions { "-finput-charset=UTF-8", "-fexec-charset=UTF-8" }