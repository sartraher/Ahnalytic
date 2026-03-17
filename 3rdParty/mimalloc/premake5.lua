project "mimalloc"
    kind "StaticLib"
    language "C"

    targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
    files {
        "src/static.c",
        "include/**.h"
    }

    includedirs {
        "..",
		"./include"
    }

    defines { "MI_STATIC_LIB" }