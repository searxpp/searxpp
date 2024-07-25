add_rules("mode.debug", "mode.release")

-- includes("script/packages.lua")
add_requires("quickjs")
add_requires("spdlog")

set_languages("c++17")

target("searxpp")
    set_kind("binary")
    add_files("src/**.cpp")
    add_packages("quickjs", "spdlog")
    add_installfiles("test/*.js")