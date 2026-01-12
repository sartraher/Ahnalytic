project "TreeSitter"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
	architecture "x86_64"
	targetname "Tree-Sitter"
	
	targetdir ("../../out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("../../out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

	vpaths {
		["Header Files"] = { "**.h", "**.hpp" },
		["Source Files"] = { "**.cpp" },
	}

    -- Universal include dirs
    includedirs { 
		"./lib/include",
		"./lib/src"
	}

    files {
		"lib/src/alloc.h",
		"lib/src/array.h",
		"lib/src/atomic.h",
		"lib/src/clock.h",
		"lib/src/error_costs.h",
		"lib/src/get_changed_ranges.h",
		"lib/src/host.h",
		"lib/src/language.h",
		"lib/src/length.h",
		"lib/src/lexer.h",
		"lib/src/parser.h",
		"lib/src/point.h",
		"lib/src/reduce_action.h",
		"lib/src/reusable_node.h",
		"lib/src/stack.h",
		"lib/src/subtree.h",
		"lib/src/tree.h",
		"lib/src/tree_cursor.h",
		"lib/src/ts_assert.h",
		"lib/src/unicode.h",
		"lib/src/wasm_store.h",
		"lib/src/alloc.c",
		"lib/src/get_changed_ranges.c",
		"lib/src/language.c",
		"lib/src/lexer.c",
		"lib/src/node.c",
		"lib/src/parser.c",
		"lib/src/query.c",
		"lib/src/stack.c",
		"lib/src/subtree.c",
		"lib/src/tree.c",
		"lib/src/tree_cursor.c",
		"lib/src/wasm_store.c"
    }

    -- Character set
    systemversion "latest"
    characterset "Unicode"

    filter "system:windows"
        staticruntime "Off"  -- lets /MD or /MDd be applied automatically

    filter "configurations:Debug"
        runtime "Debug"      -- /MDd
        symbols "On"
        defines { "_DEBUG", "_LIB" }

    filter "configurations:Release"
        runtime "Release"    -- /MD
        optimize "Speed"
        defines { "NDEBUG", "_LIB" }

    -- Reset filter
    filter {}
