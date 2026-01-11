project "soci_core"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
    characterset "MBCS"

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
		"SOCI_DLL",
		"_CRT_SECURE_NO_DEPRECATE",
		"_CRT_SECURE_NO_WARNINGS",
		"_CRT_NONSTDC_NO_WARNING",
		"_SCL_SECURE_NO_WARNINGS",
		"NOMINMAX",
		"WIN32_LEAN_AND_MEAN",
		[[DEFAULT_BACKENDS_PATH="C:/Program Files (x86)/SOCI/lib"]],
		[[SOCI_LIB_PREFIX="soci_"]],
		[[SOCI_LIB_SUFFIX=".dll"]],
		[[SOCI_DEBUG_POSTFIX=""]],
		[[SOCI_ABI_VERSION="4_1"]]
	}


    filter "configurations:Debug"
        defines { "_DEBUG", "CMAKE_INTDIR=\"Debug\"", "soci_core_EXPORTS" }
        runtime "Debug"
        symbols "On"
        optimize "Off"

    filter "configurations:Release"
        defines { "NDEBUG", "CMAKE_INTDIR=\"Release\"", "soci_core_EXPORTS" }
        runtime "Release"
        optimize "Speed"
        symbols "Off"

    filter "configurations:MinSizeRel"
        defines { "NDEBUG", "CMAKE_INTDIR=\"MinSizeRel\"", "soci_core_EXPORTS" }
        runtime "Release"
        optimize "Size"
        symbols "Off"

    filter "configurations:RelWithDebInfo"
        defines { "NDEBUG", "CMAKE_INTDIR=\"RelWithDebInfo\"", "soci_core_EXPORTS" }
        runtime "Release"
        optimize "Speed"
        symbols "On"

    filter "system:windows"
        systemversion "10.0.26100.0"
        links { "kernel32", "user32", "gdi32", "winspool", "shell32",
                "ole32", "oleaut32", "uuid", "comdlg32", "advapi32" }
        buildoptions { "/bigobj", "/utf-8" }
        linkoptions { "/machine:x64" }

    filter {}

project "soci_odbc"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "soci_core" }

    files {
        "src/backends/odbc/**.cpp",
        "include/soci/odbc/**.h"
    }

    includedirs {
        "./include/private",
        "./include",
        "./build/include"
    }

    defines {
        "WIN32",
        "_WINDOWS",
        "SOCI_DLL",
        "_CRT_SECURE_NO_DEPRECATE",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_WARNING",
        "_SCL_SECURE_NO_WARNINGS",
        "NOMINMAX",
        "WIN32_LEAN_AND_MEAN",
        "soci_odbc_EXPORTS"
    }
	
	filter "system:windows"
		buildoptions { "/Zc:wchar_t" } -- Treat wchar_t as built-in
		characterset "MBCS"  -- important: sets MultiByte, not Unicode

    filter "configurations:Debug"
        defines { [[CMAKE_INTDIR="Debug"]] }
        links {
            "odbc32.lib",
            "%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}/soci_core.lib",
            "kernel32.lib", "user32.lib", "gdi32.lib",
            "winspool.lib", "shell32.lib", "ole32.lib",
            "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"
        }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG", [[CMAKE_INTDIR="Release"]] }
        links {
            "odbc32.lib",
            "%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}/soci_core.lib",
            "kernel32.lib", "user32.lib", "gdi32.lib",
            "winspool.lib", "shell32.lib", "ole32.lib",
            "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"
        }
        runtime "Release"
        optimize "Speed"

    filter {}
 
 project "soci_sqlite3"
    kind "SharedLib"
    language "C++"
    cppdialect "C++14"
    systemversion "10.0"
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "soci_core" }

    files {
        "src/backends/sqlite3/**.cpp",
        "3rdparty/sqlite3/sqlite3.c",
        "include/soci/sqlite3/**.h"
    }

    includedirs {
        "./include",
        "./include/private",
        "./3rdparty/sqlite3",
        "./build/include"
    }

    defines { "WIN32", "_WINDOWS", "SOCI_DLL", "_CRT_SECURE_NO_DEPRECATE", "_CRT_SECURE_NO_WARNINGS",
              "_CRT_NONSTDC_NO_WARNING", "_SCL_SECURE_NO_WARNINGS", "NOMINMAX", "WIN32_LEAN_AND_MEAN",
              "soci_sqlite3_EXPORTS" }

    filter "configurations:Debug"
        defines { [[CMAKE_INTDIR="Debug"]] }
        links {
            "odbc32.lib",
            "%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}/soci_core.lib",
            "kernel32.lib", "user32.lib", "gdi32.lib",
            "winspool.lib", "shell32.lib", "ole32.lib",
            "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"
        }
        runtime "Debug"
        symbols "On"

    filter "configurations:Release"
        defines { "NDEBUG", [[CMAKE_INTDIR="Release"]] }
        links {
            "odbc32.lib",
            "%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}/soci_core.lib",
            "kernel32.lib", "user32.lib", "gdi32.lib",
            "winspool.lib", "shell32.lib", "ole32.lib",
            "oleaut32.lib", "uuid.lib", "comdlg32.lib", "advapi32.lib"
        }
        runtime "Release"
