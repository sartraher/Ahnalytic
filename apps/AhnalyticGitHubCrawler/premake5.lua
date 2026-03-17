project "AhnalyticGitHubCrawler"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "AhnalyticBase" }

    -- Virtual folders for Visual Studio
    vpaths {
        ["Source Files"] = { "**.cpp" },
        ["Header Files"] = { "**.h", "**.hpp" }
    }

    files {
        "*.cpp",
        "*.hpp",
        "*.h"
    }

    includedirs {
        "../../libs",
		"../../3rdParty/zlib",
        "../../3rdParty/zlib/build",
        "../../3rdParty/SrvLib",
		"../../3rdParty/thread-pool/include",
		"../../3rdParty/unordered_dense/include",
		"../../3rdParty/cpp-httplib",
		"../../3rdParty/openssl-3.5.4/include",
		"../../3rdParty/json/include",
        "../../3rdParty"
    }
	
	libdirs {
		"../../3rdParty",
		"../../3rdParty/openssl-3.5.4",
		"../../out/lib/%{cfg.platform}/%{cfg.buildcfg}",
		"../../out/bin/%{cfg.platform}/%{cfg.buildcfg}"
	}

    links {
        "SrvLib",
        "AhnalyticBase",
		"archive_static",
		"libexpat",
		"Tree-Sitter",
		"Tree-Sitter-CPP",
		"soci_sqlite3_4_1",
		"soci_core_4_1",
		"libbsc",
		"LzmaLib",
		"zlibstatic",
        "libssl_static",
        "libcrypto_static",
		"mimalloc"
        }  

	defines { "MI_MALLOC_OVERRIDE" }		

    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"
        links { "ws2_32" }  -- optional for networking
		
	filter "system:linux"
		links {        
        "zstd",   -- libzstd
        "bz2",    -- libbz2
        "lz4",    -- liblz4
        "lzma",   -- liblzma
        "z"       -- zlib
		}

    filter "system:linux or system:macosx"
        pic "On"
        links { 
			"ssl",
			"crypto",
			"pthread" 
		}
		buildoptions { "`pkg-config --cflags libxml-2.0`" }
		linkoptions { "`pkg-config --libs libxml-2.0`" }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "_DEBUG", "_CONSOLE" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG", "_CONSOLE" }
        linktimeoptimization "On"

    filter "platforms:x64"
        vectorextensions "AVX2"

    filter {}