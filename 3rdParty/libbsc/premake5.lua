project "libbsc"
    kind "StaticLib"
    language "C++"
    cppdialect "C++17"

    targetdir ("../../out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("../../out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

    files {
        "./libbsc/adler32/adler32.cpp",
        "./libbsc/bwt/bwt.cpp",
        "./libbsc/bwt/libsais/libsais.c",
        "./libbsc/coder/coder.cpp",
        "./libbsc/coder/qlfc/qlfc.cpp",
        "./libbsc/coder/qlfc/qlfc_model.cpp",
        "./libbsc/filters/detectors.cpp",
        "./libbsc/filters/preprocessing.cpp",
        "./libbsc/libbsc/libbsc.cpp",
        "./libbsc/lzp/lzp.cpp",
        "./libbsc/platform/platform.cpp",
        "./libbsc/st/st.cpp"
    }

    vectorextensions "AVX2"

    filter "system:windows"
        systemversion "10.0"
        defines {
            "WIN32",
            "_WINDOWS"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "On"
        optimize "Off"
        defines {
            "_DEBUG",
            'CMAKE_INTDIR="Debug"'
        }

    filter "configurations:Release"
        runtime "Release"
        optimize "Speed"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR="Release"'
        }

    filter "configurations:MinSizeRel"
        runtime "Release"
        optimize "Size"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR="MinSizeRel"'
        }

    filter "configurations:RelWithDebInfo"
        runtime "Release"
        symbols "On"
        optimize "Speed"
        defines {
            "NDEBUG",
            'CMAKE_INTDIR="RelWithDebInfo"'
        }

    filter {}
