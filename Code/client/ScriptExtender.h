#pragma once
#include <filesystem>
#include <string>
// Utility to import the 3rd-party Script Extender made by Ian Patterson et al.
// Required for non-Creation Kit mods such as SkyUI
bool ValidateScriptExtender(const std::filesystem::path& aGamePath, std::string& aError);
bool LoadScriptExtender(const std::filesystem::path& aGamePath);

// Reports DLL presence only, not plugin initialization. Retained wire semantics.
bool IsScriptExtenderLoaded();
