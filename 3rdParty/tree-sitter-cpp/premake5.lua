project "TreeSitterCPP"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    systemversion "latest"
    characterset "Unicode"
	
	dependson { "TreeSitter" }
	
	targetdir ("../../out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("../../out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

	vpaths {
		["Header Files"] = { "**.h", "**.hpp" },
		["Source Files"] = { "**.cpp" },
	}

    includedirs { "src/tree_sitter" }

    files {
        "src/tree_sitter/**.h",
        "src/**.c"
    }

    -- Compile C files as C
    filter "files:**.c"
        language "C"

    -- Precompiled header Win32 only
    filter { "system:windows", "architecture:x86" }
        pchheader "pch.h"
        pchsource "src/pch.cpp"

    -- Configurations
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        staticruntime "On"
        defines { "_DEBUG", "_LIB" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        staticruntime "On"
        defines { "NDEBUG", "_LIB" }

    -- x64 only define
    filter "architecture:x64"
        defines { "TREE_SITTER_HIDE_SYMBOLS" }

    -- Reset filter
    filter {}
