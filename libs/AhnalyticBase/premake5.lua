
project "AhnalyticBase"
    kind "SharedLib"
    language "C++"
    cppdialect "C++20"
	architecture "x86_64"
    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "OpenSSL", "archive_static", "libbsc", "zlib", "LzmaLib", "SrvLib", "soci_core", "soci_odbc", "soci_sqlite3", "TreeSitter", "TreeSitterCPP" }
	
	vpaths {
		["Header Files/*"] = { "**.h", "**.hpp" },
		["Source Files/*"] = { "**.cpp" },
	}

    files {
        "**.cpp",
        "**.hpp",
        "**.h",
    }

    includedirs {
        "..",		
        "../../3rdParty",
        "../../3rdParty/zlib",
        "../../3rdParty/zlib/build",
        --"../../3rdParty/fastpfor",
        --"../../3rdParty/zstd/lib",
        --"../../3rdParty/lz4/lib",
        "../../3rdParty/lzma/C",
        "../../3rdParty/libbsc/libbsc",
        "../../3rdParty/tree-sitter/lib/include",
        "../../3rdParty/thread-pool/include",
        "../../3rdParty/soci/include",
        "../../3rdParty/soci/build/include",
        "../../3rdParty/expat/Source/lib",
        "../../3rdParty/unordered_dense/include",
        "../../3rdParty/cpp-httplib",
        "../../3rdParty/json/include",
        "../../3rdParty/openssl-3.5.4/include",
        "../../3rdParty/libarchive/libarchive"
    }
	
	defines {
        "AHNALYTICBASE_EXPORTS",
        "LIBARCHIVE_STATIC"
    }


    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"

        defines {
            "_WINDOWS",
            "_USRDLL"
        }

        links {
            "archive_static",
            "libexpat",
            "Tree-Sitter",
            "Tree-Sitter-CPP",
            "soci_sqlite3_4_1",
            "soci_odbc_4_1",
            "soci_core_4_1",
            "libbsc",
            --"libzstd",
            "LzmaLib",
            "zlibstatic",
            --"FastPFor",
            --"liblz4_static",
            "libssl_static",
            "libcrypto_static"
        }

        libdirs {
            "../../3rdParty",
            "../../3rdParty/expat/Bin",
            "../../3rdParty/openssl-3.5.4",
            "../../out/lib/%{cfg.platform}/%{cfg.buildcfg}",
            "../../out/bin/%{cfg.platform}/%{cfg.buildcfg}"
        }

    filter "system:linux or system:macosx"
        pic "On"

        links {
            "archive",
            "expat",
            --"z",
            --"zstd",
            --"lz4",
            "lzma",
            "ssl",
            "crypto",
            "pthread"
        }

	filter "system:windows"
		includedirs {
			"../../3rdParty/soci/build/windows/include"
		}

	filter "system:linux or system:macosx"      
		includedirs {
			"../../3rdParty/soci/build/linux/include"
		}

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        defines { "_DEBUG" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines { "NDEBUG" }		
        flags { "linktimeoptimization" }

    filter "platforms:x64"
        vectorextensions "AVX2"

    filter "system:windows"
        postbuildcommands {
            '{COPY} "%{wks.location}3rdParty/expat/Bin/libexpat.dll" "%{cfg.targetdir}"'
        }

    filter {}
