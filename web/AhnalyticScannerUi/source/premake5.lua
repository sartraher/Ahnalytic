project "AhnalyticScannerUi"
    kind "Utility"
    language "C"
    targetname "AhnalyticScannerUi"
	
	targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
    
    filter "system:linux or system:macosx"
		prebuildcommands {
			("npm run build")
		}
	
	filter "system:windows"
		prebuildcommands {
			("cd %{wks.location}/web/AhnalyticScannerUi/source && npm run build")
		}
