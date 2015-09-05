workspace "Enum"
    configurations { "Debug", "Release" }

project "test"
    kind "ConsoleApp"
    language "C++"
    files { "include/**.hpp", "src/**.cpp" }
    includedirs { "include" }
    buildoptions { "--std=c++14" }

    filter { "configurations:Debug" }
        flags { "Symbols" }

    filter { "configurations:Release" }
        optimize "On"
