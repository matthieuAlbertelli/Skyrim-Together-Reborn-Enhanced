
target("CommonLib")
    -- Git HEAD is not a normal template dependency. Refresh candidate identity
    -- on every configuration/build; xmake only writes if contents differ.
    set_policy("build.always_update_configfiles", true)
    add_configfiles("BuildInfo.h.in")
    set_kind("static")
    set_group("common")
    add_includedirs(".", "../", "../../build", {public = true})
    add_headerfiles("**.h")
    add_files("**.cpp")
    add_packages("tiltedcore", "hopscotch-map")
