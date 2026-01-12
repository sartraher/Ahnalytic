project "soci_core"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
	architecture "x86_64"
	targetname "soci_core_4_1"

    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    files {
        "./src/core/**.cpp",
        "./include/soci/**.h",
        "./build/include/soci/soci-config.h"
    }

    includedirs {
        "./include",
		"./include/private",
        "./include/soci",
        "./include/soci/private",
        "./build/include"
    }

      defines {
        "SOCI_CORE",
        [[SOCI_ABI_VERSION="4_1"]]
    }

	filter "system:windows"
		includedirs {
			"./build/windows/include"
		}

    filter "system:windows"
        characterset "MBCS"
        systemversion "10.0.26100.0"

        defines {
            "SOCI_DLL",
            "WIN32_LEAN_AND_MEAN",
            "NOMINMAX",
            "_CRT_SECURE_NO_WARNINGS",
            [[DEFAULT_BACKENDS_PATH="C:/Program Files (x86)/SOCI/lib"]],
            [[SOCI_LIB_PREFIX="soci_"]],
            [[SOCI_LIB_SUFFIX=".dll"]],
            [[SOCI_DEBUG_POSTFIX=""]],
        }

        links {
            "kernel32", "user32", "gdi32", "winspool",
            "shell32", "ole32", "oleaut32",
            "uuid", "comdlg32", "advapi32"
        }

        buildoptions { "/bigobj", "/utf-8" }

	filter "system:linux or system:macosx"      
		includedirs {
			"./build/linux/include"
		}

    filter "system:linux or system:macosx"        
        pic "On"
        defines {
            "SOCI_DLL",
            [[SOCI_LIB_SUFFIX=".so"]],
            [[SOCI_LIB_PREFIX="libsoci_"]],
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines { "_DEBUG", "soci_core_EXPORTS" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        symbols "Off"
        defines { "NDEBUG", "soci_core_EXPORTS" }
		
	filter "system:linux or system:macosx"
		buildoptions {
			"-std=c++17",
			"-I/usr/include/x86_64-linux-gnu/c++/13"
		}

    filter {}
 
 project "soci_sqlite3"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
    architecture "x86_64"
	
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	targetname "soci_sqlite3_4_1"
	dependson { "soci_core" }

    files {
        "src/backends/sqlite3/**.cpp",
        "3rdparty/sqlite3/sqlite3.c",
        "include/soci/sqlite3/**.h"
    }

    includedirs {
        "./include",
        "./include/private",
        "./3rdparty/sqlite3"
    }

    -- Windows-specific defines and includes
    filter "system:windows"
        defines { 
            "WIN32", "_WINDOWS", "SOCI_DLL", 
            "_CRT_SECURE_NO_DEPRECATE", "_CRT_SECURE_NO_WARNINGS",
            "_CRT_NONSTDC_NO_WARNING", "_SCL_SECURE_NO_WARNINGS", 
            "NOMINMAX", "WIN32_LEAN_AND_MEAN", "soci_sqlite3_EXPORTS"
        }
        includedirs { "./build/windows/include" }
        links {
            "odbc32.lib",
            "%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}/soci_core_4_1.lib",
            "kernel32.lib", "user32.lib", "gdi32.lib",
            "winspool.lib", "shell32.lib", "ole32.lib",
            "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"
        }

    -- Linux/macOS-specific defines and includes
    filter "system:linux or system:macosx"
        defines { 
            "SOCI_INT64_T_IS_LONG", 
            "SOCI_LONG_LONG_IS_64BIT", 
            "SOCI_DLL", 
            "soci_sqlite3_EXPORTS"
        }
        includedirs { "./build/linux/include" }
        buildoptions { 
            "-std=c++17", 
            "-I/usr/include/x86_64-linux-gnu/c++/13" 
        }

    -- Configuration-specific defines
    filter "configurations:Debug"
        defines { [[CMAKE_INTDIR="Debug"]] }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG", [[CMAKE_INTDIR="Release"]] }
        runtime "Release"
        optimize "Speed"

    filter {}

