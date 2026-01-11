project "FastPFor"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17" -- original project does not force C++20
    staticruntime "off"

    -- output layout
    targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    -- sources
    files {
        "src/**.cpp",
        "src/**.c",
        "headers/**.h"
    }

    includedirs {
        "headers"
    }

    filter "system:windows"
        systemversion "latest"
        -- Premake does NOT support MultiByte.
        -- This is the correct equivalent (no Unicode defines)
        characterset "Default"

    filter "platforms:x64"
        architecture "x86_64"

    -- AVX
    filter "action:vs*"
        vectorextensions "AVX"

    -- Debug
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"

    -- Release
    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        symbols "On"
