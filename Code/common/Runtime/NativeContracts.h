#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <string_view>

namespace STRE::Runtime
{
// Static admission evidence only; never a claim of validation in Skyrim.
// IDs and proof obligations: docs/development/RUNTIME_1_7_NATIVE_CONTRACTS.md.
// ExactSource means the cited exact-target implementation covers the access.
// Partial source evidence is Missing when the remaining obligation needs RE.
// ExactBinary receipts: docs/audit/2026-10-20-runtime-1.7.104-lot3.md.
// Engine acceptance, including deferred retirement, remains untested.
enum class NativeContract : std::size_t
{
    Loader, TargetImage, SkseBinary, RttiRegistration, UpstreamHooks, PapyrusVm,
    SaveFunction, LoadFunction, SaveProcessBoundary, SavePath, SaveSelection,
    ContinueDispatch, SaveLoadCallbacks, PauseMenu, ResetMain, CreationConsole,
    FinalAppearance, LocalSeating, GrabRelease, MenuPresentation, DialogueProgress,
    AppearanceDiagnostics, Count
};
enum class NativeScope { Bootstrap, Campaign, Optional, Diagnostic };
enum class NativeEvidence { Missing, ExactSource, ExactBinary };

struct NativeContractRequirement
{
    NativeContract Id;
    std::string_view Key;
    NativeScope Scope;
    NativeEvidence Required;
    NativeEvidence Available;

    constexpr bool Mandatory() const
    {
        return Scope == NativeScope::Bootstrap || Scope == NativeScope::Campaign;
    }
    constexpr bool Qualified() const
    {
        return Available == NativeEvidence::ExactBinary ||
               (Required == NativeEvidence::ExactSource && Available == NativeEvidence::ExactSource);
    }
};

inline constexpr std::array NativeContracts{
    NativeContractRequirement{NativeContract::Loader, "B01-loader", NativeScope::Bootstrap, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::TargetImage, "B02-target-image", NativeScope::Bootstrap, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::SkseBinary, "B03-skse-binary", NativeScope::Bootstrap, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::RttiRegistration, "B04-rtti-registration", NativeScope::Bootstrap, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::UpstreamHooks, "B05-upstream-hooks", NativeScope::Bootstrap, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::PapyrusVm, "C01-papyrus-vm", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::SaveFunction, "C02-save-function", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::LoadFunction, "C03-load-function", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::SaveProcessBoundary, "C04-save-process", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::SavePath, "C05-save-path", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::SaveSelection, "C06-save-selection", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::ContinueDispatch, "C07-continue-dispatch", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::SaveLoadCallbacks, "C08-save-load-callbacks", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::PauseMenu, "C09-pause-menu", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::ResetMain, "C10-reset-main", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::CreationConsole, "C11-creation-console", NativeScope::Campaign, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::FinalAppearance, "C12-final-appearance", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::LocalSeating, "C13-local-seating", NativeScope::Campaign, NativeEvidence::ExactBinary, NativeEvidence::ExactBinary},
    NativeContractRequirement{NativeContract::GrabRelease, "O01-grab-release", NativeScope::Optional, NativeEvidence::ExactSource, NativeEvidence::ExactSource},
    NativeContractRequirement{NativeContract::MenuPresentation, "O02-menu-presentation", NativeScope::Optional, NativeEvidence::ExactBinary, NativeEvidence::Missing},
    NativeContractRequirement{NativeContract::DialogueProgress, "O03-dialogue-progress", NativeScope::Optional, NativeEvidence::ExactBinary, NativeEvidence::Missing},
    NativeContractRequirement{NativeContract::AppearanceDiagnostics, "D01-appearance-diagnostics", NativeScope::Diagnostic, NativeEvidence::ExactBinary, NativeEvidence::Missing},
};

// Missing, duplicate or reordered obligations cannot silently remove a gate.
constexpr bool NativeAdmissionReady(std::span<const NativeContractRequirement> aContracts)
{
    if (aContracts.size() != static_cast<std::size_t>(NativeContract::Count))
        return false;
    for (std::size_t i = 0; i < aContracts.size(); ++i)
    {
        const auto& contract = aContracts[i];
        const auto& required = NativeContracts[i];
        if (contract.Id != required.Id || contract.Scope != required.Scope || contract.Required != required.Required ||
            (required.Mandatory() && !contract.Qualified()))
            return false;
    }
    return true;
}

constexpr bool SupportsQualifiedNativeAccess(std::string_view aVersion, NativeContract aContract)
{
    // Narrow wrappers only. Never use this to admit the entire executable.
    if (aContract != NativeContract::ResetMain && aContract != NativeContract::GrabRelease &&
        aContract != NativeContract::FinalAppearance && aContract != NativeContract::LocalSeating)
        return false;
    return aVersion == "1.6.1170.0" ||
           (aVersion == "1.7.104.0" && NativeContracts[static_cast<std::size_t>(aContract)].Qualified());
}
} // namespace STRE::Runtime
