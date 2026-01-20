workspace "Ahnalytic"
    configurations { "Debug", "Release" }
    platforms { "x64" }

    filter "platforms:x64"
        architecture "x86_64"

    filter {}
	
-- 3rdParty
group "3rdParty"	
group "3rdParty/compression"
dofile("3rdParty/libarchive/premake5.lua")
--dofile("3rdParty/fastpfor/premake5.lua")
dofile("3rdParty/libbsc/premake5.lua")
--dofile("3rdParty/lz4/premake5.lua")
--dofile("3rdParty/zstd/premake5.lua")
dofile("3rdParty/lzma/premake5.lua")
dofile("3rdParty/zlib/premake5.lua")

group "3rdParty/other"
dofile("3rdParty/SrvLib/premake5.lua")
dofile("3rdParty/openssl-3.5.4/premake5.lua")
dofile("3rdParty/expat/premake5.lua")

group "3rdParty/soci"
dofile("3rdParty/soci/premake5.lua")

group "3rdParty/TreeSitter"
dofile("3rdParty/tree-sitter/premake5.lua")
dofile("3rdParty/tree-sitter-cpp/premake5.lua")

-- libs
group "libs"
dofile("libs/AhnalyticBase/premake5.lua")

-- apps
group "apps"
dofile("apps/AhnalyticScannerServer/premake5.lua")
dofile("apps/AhnalyticUpdateServer/premake5.lua")
dofile("apps/AhnalyticUpdateRunner/premake5.lua")

-- web
group "web"
dofile("web/AhnalyticScannerUi/source/premake5.lua")

-- install
group "install"
project "Install"
    kind "Utility"
    language "C"
    targetname "install_build"
	
	targetdir ("%{wks.location}/out/lib/%{cfg.platform}/%{cfg.buildcfg}")
    objdir    ("%{wks.location}/out/obj/%{cfg.platform}/%{cfg.buildcfg}/%{prj.name}")
	
	dependson { "AhnalyticScannerServer", "AhnalyticUpdateServer" }
    
    filter "system:linux or system:macosx"
		prebuildcommands {
			("")
		}
	
	filter "system:windows"
		prebuildcommands {
			("if not exist \"%{wks.location}bin\" ( packbuild.cmd %{wks.location}\\bin %{wks.location}\\out\\bin\\%{cfg.platform}\\%{cfg.buildcfg} )")
		}
		
-- tests, visual studio only
if _ACTION:match("vs") and os.target() == "windows" then
    group "tests"
    
    local tests = { "CompressionTest", "DatabaseTest", "ImportTest", "SearchTest" }
    for _, t in ipairs(tests) do
        externalproject(t)
            kind "ConsoleApp"
            language "C++"
            location("tests/" .. t)
            dependson { "AhnalyticBase" }
    end
end

filter {} 