target("TPTests")
    set_kind("binary")
    set_group("Tests")
    add_includedirs(
        ".", "../encoding")
    add_headerfiles("**.h")
    add_files("*.cpp")
    add_files("../client/MainMenu/PresentationPolicy.cpp")
    add_files("../client/MainMenu/Localization.cpp")
    if is_plat("windows") then
        add_files("../client/MainMenu/VideoPlayer.cpp")
        add_syslinks("d3d11", "mfuuid", "mfplat", "mfreadwrite", "ole32", "oleaut32")
    end
    add_deps(
        "SkyrimEncoding",
        "CommonLib",
        "CampaignPersistence",
        "CampaignRuntime",
        "CampaignClient",
        "CampaignProtocol")
    add_packages(
        "tiltedcore",
        "hopscotch-map",
        "catch2",
        "mimalloc",
        "glm",
        "sqlite3")
