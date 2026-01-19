project "zlibstatic"
    kind "StaticLib"
    language "C"
    characterset "MBCS"

    targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    files {
        "./adler32.c",
        "./compress.c",
        "./crc32.c",
        "./deflate.c",
        "./gzclose.c",
        "./gzlib.c",
        "./gzread.c",
        "./gzwrite.c",
        "./inflate.c",
        "./infback.c",
        "./inftrees.c",
        "./inffast.c",
        "./trees.c",
        "./uncompr.c",
        "./zutil.c",
        "./build/zconf.h",
        "./zlib.h",
        "./crc32.h",
        "./deflate.h",
        "./gzguts.h",
        "./inffast.h",
        "./inffixed.h",
        "./inflate.h",
        "./inftrees.h",
        "./trees.h",
        "./zutil.h"
    }

    includedirs {
        "%{wks.location}/3rdParty/zlib",
        "%{wks.location}/3rdParty/zlib/build"
    }

    defines {
        "WIN32",
        "_WINDOWS",
        "NO_FSEEKO",
        "_CRT_SECURE_NO_DEPRECATE",
        "_CRT_NONSTDC_NO_DEPRECATE"
    }

    filter "system:windows"
        system "windows"
        systemversion "latest"
        architecture "x86_64"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines {
            "_DEBUG",
            'CMAKE_INTDIR=\\"Debug\\"'
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR=\\"Release\\"'
        }

    filter "configurations:MinSizeRel"
        runtime "Release"
        optimize "Size"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR=\\"MinSizeRel\\"'
        }

    filter "configurations:RelWithDebInfo"
        runtime "Release"
        optimize "Speed"
        symbols "On"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR=\\"RelWithDebInfo\\"'
        }
		
	filter "system:linux"
		defines { "_POSIX_C_SOURCE=200809L" }

    filter {}
