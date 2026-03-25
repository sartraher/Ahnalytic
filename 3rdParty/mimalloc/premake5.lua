project "mimalloc"
   kind "SharedLib"            -- DLL on Windows, .so on Linux
   language "C"
   cdialect "C11"
   systemversion "latest"

   targetdir ("%{wks.location}/out/bin/%{cfg.platform}/%{cfg.buildcfg}")
   objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

   -- Preprocessor defines
   defines { "WIN32", "_WINDOWS", "MI_SHARED_LIB_EXPORT", "MI_BUILD_RELEASE", "MI_SHARED_LIB", "MI_MALLOC_OVERRIDE", "mimalloc_EXPORTS", "MI_ALLOCATOR_INIT" }

   -- Include headers
   includedirs { "include" }

   -- Source files
   files {
        "src/alloc.c",
        "src/alloc-aligned.c",
        "src/alloc-posix.c",
        "src/arena.c",
        "src/arena-meta.c",
        "src/bitmap.c",
        "src/heap.c",
        "src/init.c",
        "src/libc.c",
        "src/options.c",
        "src/os.c",
        "src/page.c",
        "src/page-map.c",
        "src/random.c",
        "src/stats.c",
        "src/theap.c",
        "src/threadlocal.c",
        "src/prim/prim.c"
   }

   -- Windows-specific settings
   filter "system:windows"
      defines { "_CRT_SECURE_NO_WARNINGS" }
      libdirs { "../../3rdParty/mimalloc/bin" }
      links { "mimalloc-redirect" }

      -- Copy mimalloc-redirect.dll from 3rdParty to target dir
      postbuildcommands {
         '{COPY} ../../3rdParty/mimalloc/bin/mimalloc-redirect.dll %{cfg.targetdir}'
      }

   -- Linux-specific settings
   filter "system:linux"
      links { "mimalloc" } -- expects libmimalloc.so installed or built separately
      includedirs { "/usr/local/include" }
      libdirs { "/usr/local/lib" }

   -- Debug configuration
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   -- Release configuration
   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"