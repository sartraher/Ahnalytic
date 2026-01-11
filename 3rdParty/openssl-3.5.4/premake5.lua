project "OpenSSL"
    kind "Utility"
    language "C"
    targetname "openssl_build"
	
	targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
    
    local stamp_file = "./.built"

    filter "system:linux or system:macosx"
		prebuildcommands {
			("if not exist %s (./build_unix.sh && echo ok > %s)"):format(stamp_file, stamp_file)
		}
	
	filter "system:windows"
		prebuildcommands {
			("if not exist %s (build_window.bat && echo ok > %s)"):format(stamp_file, stamp_file)
		}