project "mimalloc"
   kind "SharedLib"           -- DLL on Windows, SO on Linux
   language "C"

   -- Output directories
   targetdir ("%{wks.location}/../out/bin/%{cfg.platform}/%{cfg.buildcfg}")
   objdir    ("%{wks.location}/../out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")

   -- Preprocessor defines
   defines { "WIN32", "_WINDOWS", "MI_SHARED_LIB_EXPORT", "MI_BUILD_RELEASE",
             "MI_SHARED_LIB", "MI_MALLOC_OVERRIDE", "mimalloc_EXPORTS", "MI_ALLOCATOR_INIT" }

   -- Include headers
   includedirs { "include" }

   -- All source files
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

   -- C standard
   cdialect "C11"
   systemversion "latest"

   --------------------------------------
   -- Windows settings
   --------------------------------------
   filter "system:windows"
      defines { "_CRT_SECURE_NO_WARNINGS" }

      -- Link to external mimalloc-redirect DLL
      libdirs { "../../3rdParty/mimalloc/bin" }
      links { "mimalloc-redirect" }

      -- Copy mimalloc-redirect.dll after build
      postbuildcommands {
         "{COPY} ../../3rdParty/mimalloc/bin/mimalloc-redirect.dll %{cfg.targetdir}"
      }

   --------------------------------------
   -- Linux settings
   --------------------------------------
   filter "system:linux"
      targetname "mimalloc"   -- produces libmimalloc.so
      pic "On"                -- required for shared libraries
      defines { "_GNU_SOURCE" }

      -- If you have a system-installed mimalloc, you can link like this:
      -- libdirs { "/usr/local/lib" }
      -- links { "mimalloc" }
      -- includedirs { "/usr/local/include" }

      -- No post-build copy needed

   --------------------------------------
   -- Debug configuration
   --------------------------------------
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   --------------------------------------
   -- Release configuration
   --------------------------------------
   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"