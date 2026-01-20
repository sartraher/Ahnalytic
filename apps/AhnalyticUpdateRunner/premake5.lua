project "AhnalyticUpdateRunner"
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
        "*.h",		
		"../../images/update.ico"
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
		"zlibstatic"
    }   

    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"
        links { "ws2_32" }  -- optional for networking

    filter "system:linux or system:macosx"
        pic "On"
        links { 
			"ssl",
			"crypto",
			"pthread" 
		}

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "_DEBUG", "_CONSOLE" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG", "_CONSOLE" }
        flags { "linktimeoptimization" }

    filter "platforms:x64"
        vectorextensions "AVX2"

    filter {}