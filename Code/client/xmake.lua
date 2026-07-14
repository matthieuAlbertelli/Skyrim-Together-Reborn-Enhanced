
local function build_client(name)
target(name)
    set_kind("static")
    set_group("Client")
    add_includedirs(".","../../Libraries/")
    set_pcxxheader("TiltedOnlinePCH.h")

    -- Build the CEF/Angular UI before the client so the install step always
    -- packages the interface matching the native trade bridge.
    before_build(function(target)
        local uiroot = path.join(target:scriptdir(), "..", "skyrim_ui")
        local node_modules = path.join(uiroot, "node_modules")

        -- Keep the process calls directly inside the XMake callback. Local
        -- helper closures do not inherit XMake's sandboxed os.execv binding
        -- in the project version used here.
        if not os.isdir(node_modules) then
            os.execv("cmd.exe", {"/d", "/s", "/c", "pnpm.cmd install"}, {curdir = uiroot})
        end

        os.execv("cmd.exe", {"/d", "/s", "/c", "pnpm.cmd run deploy:production"}, {curdir = uiroot})
    end)

    -- exclude game specifc stuff
    add_headerfiles("**.h|Games/Skyrim/**|Services/Vivox/**")
    add_files("**.cpp|Games/Skyrim/**|Services/Vivox/**")

    after_install(function(target)
        -- copy dlls
        for _, pkg_with_dlls in ipairs({"cef", "discord"}) do
            local linkdir = target:pkg(pkg_with_dlls):get("linkdirs")
            local bindir = path.join(linkdir, "..", "bin")
            os.cp(bindir, target:installdir())
        end

        -- Install the Angular output at the exact path expected by CEF:
        -- <runtime>/UI/index.html. Remove stale hashed bundles first.
        local uiroot = path.join(target:scriptdir(), "..", "skyrim_ui")
        local uidist = path.join(uiroot, "dist", "UI")
        local installbin = path.join(target:installdir(), "bin")
        local installui = path.join(installbin, "UI")

        if not os.isdir(uidist) then
            raise("Compiled Skyrim Together UI not found: " .. uidist)
        end

        os.rm(installui)
        os.mkdir(installui)
        os.cp(path.join(uidist, "*"), installui)

        -- retain cursor resources even when the Angular build is skipped
        local uisrc = path.join(uiroot, "src")
        os.cp(path.join(uisrc, "assets", "images", "cursor.dds"), path.join(installbin, "assets", "images", "cursor.dds"))
        os.cp(path.join(uisrc, "assets", "images", "cursor.png"), path.join(installbin, "assets", "images", "cursor.png"))
        os.rm(path.join(installbin, "**Tests.exe"))
    end)

    add_files("Games/Skyrim/**.cpp")
    add_headerfiles("Games/Skyrim/**.h")
    -- rather hacky:
    add_includedirs("Games/Skyrim")
    add_deps("SkyrimEncoding")
    add_deps(
        "UiProcess",
        "CommonLib",
        "BaseLib",
        "ImGuiImpl",
        "TiltedConnect",
        "TiltedReverse",
        "TiltedHooks",
        "TiltedUi",
        {inherit = true}
    )

    add_packages(
        "tiltedcore",
        "spdlog",
        "hopscotch-map",
        "cryptopp",
        "gamenetworkingsockets",
        "discord",
        "imgui",
        "cef",
        "minhook",
        "entt",
        "glm",
        "mem",
        "xbyak")

    if has_config("vivox") then
        add_files("Services/Vivox/**.cpp")
        add_headerfiles("Services/Vivox/**.h")
        add_includedirs("Services/Vivox")
        add_deps("Vivox")
        add_defines("TP_VIVOX=1")
    else
        add_defines("TP_VIVOX=0")
    end

    add_syslinks(
        "version",
        "dbghelp",
        "kernel32")
end

add_requires("tiltedcore")

build_client("SkyrimTogetherClient")
