project "AhnalyticScannerServer"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++20"
    architecture "x86_64"

    targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    dependson { "AhnalyticBase" }

    files {
        "*.cpp",
        "*.hpp",
        "*.h",
        "../../images/logo.ico",
		"../../3rdParty/mimalloc/miforce.cpp"
    }

    vpaths {
        ["Source Files"] = { "**.cpp" },
        ["Header Files"] = { "**.h", "**.hpp" }
    }

    includedirs {
        "../../libs",
        "../../3rdParty/SrvLib",
        "../../3rdParty",
        "../../3rdParty/unordered_dense/include",
        "../../3rdParty/mimalloc/include"
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

        -- 🔥 IMPORTANT: mimalloc LAST
        "mimalloc"
    }

    -- Required for mimalloc override visibility
    defines {
        "MI_MALLOC_OVERRIDE"
    }

    -- 🔥 CRITICAL: force mimalloc symbols + avoid static CRT conflicts
    linkoptions {
        "/INCLUDE:mi_version",
        "/NODEFAULTLIB:libcpmt.lib",
        "/NODEFAULTLIB:libcmt.lib"
    }

    ------------------------------------------------------
    -- Windows
    ------------------------------------------------------
    filter "system:windows"
        systemversion "latest"
        characterset "Unicode"

        files { "resource.rc" }

        links {
            "ws2_32"
        }

        buildoptions {
            "/bigobj",
            "/utf-8"
        }

        linkoptions {
            "/STACK:16777216" -- 16MB stack
        }

    ------------------------------------------------------
    -- Linux
    ------------------------------------------------------
    filter "system:linux"
        links {
            "pthread",
            "zstd",
            "bz2",
            "lz4",
            "lzma",
            "z"
        }

        buildoptions {
            "`pkg-config --cflags libxml-2.0`"
        }

        linkoptions {
            "`pkg-config --libs libxml-2.0`",
            "-Wl,-z,stack-size=16777216"
        }

    ------------------------------------------------------
    -- macOS
    ------------------------------------------------------
    filter "system:macosx"
        pic "On"
        links { "pthread" }

    ------------------------------------------------------
    -- Configurations
    ------------------------------------------------------
    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines { "_DEBUG", "_CONSOLE" }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        symbols "Off"
        linktimeoptimization "On"
        defines { "NDEBUG", "_CONSOLE" }

    ------------------------------------------------------
    -- Platform
    ------------------------------------------------------
    filter "platforms:x64"
        vectorextensions "AVX2"

    filter {}