
target("BaseLib")
    add_configfiles("BuildInfo.h.in")
    set_kind("static")
    set_group("common")
    add_includedirs(".", "../", path.absolute(get_config("builddir") or "build", os.projectdir()), {public = true})
    add_headerfiles("**.h")
    add_files("**.cpp")
    add_packages(
        "tiltedcore",
        "hopscotch-map", 
        "gtest",
        "spdlog")
