workspace "Ahnalytics"
    configurations { "Debug", "Release" }
    platforms { "Win32", "x64" }

    filter "platforms:x64"
        architecture "x86_64"

    filter {}
	
-- 3rdParty
group "3rdParty"	
group "3rdParty/compression"
dofile("3rdParty/libarchive/premake5.lua")
dofile("3rdParty/fastpfor/premake5.lua")
dofile("3rdParty/libbsc/premake5.lua")
dofile("3rdParty/lz4/premake5.lua")
dofile("3rdParty/zstd/premake5.lua")
dofile("3rdParty/lzma/premake5.lua")
dofile("3rdParty/zlib/premake5.lua")

group "3rdParty/other"
dofile("3rdParty/SrvLib/premake5.lua")
dofile("3rdParty/openssl-3.5.4/premake5.lua")

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

-- tests, visual studio only
group "tests"
externalproject "CompressionTest"
    kind "ConsoleApp"
    language "C++"
    location "tests/CompressionTest" -- path where the .vcxproj resides
	
externalproject "DatabaseTest"
    kind "ConsoleApp"
    language "C++"
    location "tests/DatabaseTest" -- path where the .vcxproj resides
	
externalproject "ImportTest"
    kind "ConsoleApp"
    language "C++"
    location "tests/ImportTest" -- path where the .vcxproj resides
	
externalproject "SearchTest"
    kind "ConsoleApp"
    language "C++"
    location "tests/SearchTest" -- path where the .vcxproj resides
