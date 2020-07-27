project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "on"

	location "../%{prj.name}"
	
	outputDirectory = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	srcDirectory = "../%{prj.name}/"
	
	targetdir ("bin/" .. outputDirectory .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDirectory .. "/%{prj.name}")
	
    files
    {
        srcDirectory .. "include/glad/glad.h",
        srcDirectory .. "include/KHR/khrplatform.h",
        srcDirectory .. "src/glad.c"
    }

    includedirs
    {
        srcDirectory .. "include"
    }
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"