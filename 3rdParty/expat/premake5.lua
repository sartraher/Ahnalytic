project "expat"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
	architecture "x86_64"
	
	targetname "libexpat"
	
	targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	files {
		"./lib/xmlparse.c",		
		"./lib/xmlrole.c",
		"./lib/xmltok.c",
		"./build/lib/libexpat.def"
    }
	
	includedirs {
        "./lib",
        "./build"
    }

	filter "system:windows"
        defines { 
            "WIN32","_WINDOWS","VER_FILEVERSION=2,7,3,0","_CRT_SECURE_NO_WARNINGS",[[CMAKE_INTDIR="RelWithDebInfo"]],"expat_EXPORTS"
        }
		
	filter "system:linux or system:macosx"
        defines { 
            "VER_FILEVERSION=2,7,3,0","_CRT_SECURE_NO_WARNINGS",[[CMAKE_INTDIR="RelWithDebInfo"]],"expat_EXPORTS"
        }
		buildoptions { 
            "-std=c++17", 
            "-I/usr/include/x86_64-linux-gnu/c++/13" 
        }