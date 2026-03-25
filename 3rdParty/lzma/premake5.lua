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
        "./C/Threads.c",
		"../../3rdParty/mimalloc/miforce.cpp"
    }

    includedirs {
        ".",
        "./C",
        "./C/Util/LzmaLib",
		"../../3rdParty/mimalloc/include"
    }
	
	libdirs {
        "../../out/lib/%{cfg.platform}/%{cfg.buildcfg}",
        "../../out/bin/%{cfg.platform}/%{cfg.buildcfg}"
    }

    defines {
        "LZMALIB_EXPORTS",
        "COMPRESS_MF_MT"
    }
	
	links { "mimalloc" }
	defines { "MI_MALLOC_OVERRIDE" }

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
