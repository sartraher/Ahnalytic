project "LzmaLib"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"

    characterset "MBCS"

    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/LzmaLib")

    files {
        "./C/Alloc.c",
        "./C/CpuArch.c",
        "./C/LzFind.c",
        "./C/LzFindMt.c",
        "./C/LzFindOpt.c",
        "./C/LzmaDec.c",
        "./C/LzmaEnc.c",
        "./C/LzmaLib.c",
        "./C/Threads.c",
        "./C/Util/LzmaLib/LzmaLibExports.c",
        "./C/Util/LzmaLib/resource.rc",
        "./C/Util/LzmaLib/LzmaLib.def"
    }

    includedirs {
        ".",
		"./C",
		"./C/Util/LzmaLib"
    }

    defines {
        "_USRDLL",
        "LZMALIB_EXPORTS",
        "COMPRESS_MF_MT"
    }

    filter "system:windows"
        system "windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines { "_DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG" }

    filter "platforms:x86"
        architecture "x86"

    filter "platforms:x64"
    architecture "x86_64"
    linkoptions {
        "/DEF:\"%{prj.location}/C/Util/LzmaLib/LzmaLib.def\""
    }

    filter {}
