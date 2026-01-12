project "LzmaLib"
    kind "SharedLib"
    language "C"
    cdialect "C11"

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
        "./C/Threads.c"
    }

    includedirs {
        ".",
        "./C",
        "./C/Util/LzmaLib"
    }

    defines {
        "LZMALIB_EXPORTS",
        "COMPRESS_MF_MT"
    }

    -- Windows-only
    filter "system:windows"
        systemversion "latest"
        characterset "MBCS"
        files {
            "./C/Util/LzmaLib/resource.rc",
            "./C/Util/LzmaLib/LzmaLibExports.c",
            "./C/Util/LzmaLib/LzmaLib.def"
        }
        linkoptions {
            "/DEF:\"%{prj.location}/C/Util/LzmaLib/LzmaLib.def\""
        }

	architecture "x86_64"

    -- Linux / Unix
    filter "system:linux"        
        pic "On"

    filter "configurations:Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        optimize "Speed"

    filter {}
