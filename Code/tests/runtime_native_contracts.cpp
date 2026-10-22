#include <catch2/catch.hpp>
#include <Runtime/Compatibility.h>

using namespace STRE::Runtime;

TEST_CASE("Native admission requires the complete evidence set, not relocation presence", "[runtime-port][native-contracts]")
{
    REQUIRE(CampaignAbiVerified);
    REQUIRE(CampaignAbiVerified == NativeAdmissionReady(NativeContracts));
    REQUIRE_FALSE(NativeAdmissionReady({}));

    // Synthetic evidence exercises policy only. It is never supplied to startup.
    auto evidence = NativeContracts;
    for (auto& contract : evidence)
        if (contract.Mandatory())
            contract.Available = contract.Required;
    REQUIRE(NativeAdmissionReady(evidence));
    REQUIRE_FALSE(NativeAdmissionReady(std::span(evidence).first(evidence.size() - 1)));

    for (std::size_t i = 0; i < evidence.size(); ++i)
    {
        INFO(evidence[i].Key);
        auto missing = evidence;
        missing[i].Available = NativeEvidence::Missing;
        REQUIRE(NativeAdmissionReady(missing) == !evidence[i].Mandatory());
        if (evidence[i].Mandatory())
        {
            auto weakened = missing;
            weakened[i].Scope = NativeScope::Diagnostic;
            REQUIRE_FALSE(NativeAdmissionReady(weakened));
            if (evidence[i].Required == NativeEvidence::ExactBinary)
            {
                missing[i].Available = NativeEvidence::ExactSource;
                REQUIRE_FALSE(NativeAdmissionReady(missing));
                missing[i].Required = NativeEvidence::ExactSource;
                REQUIRE_FALSE(NativeAdmissionReady(missing));
            }
        }
        auto duplicated = evidence;
        duplicated[i].Id = evidence[(i + 1) % evidence.size()].Id;
        REQUIRE_FALSE(NativeAdmissionReady(duplicated));
    }
}

TEST_CASE("Qualified native wrappers remain exact and cannot open other paths", "[runtime-port][native-contracts]")
{
    for (const auto version : {"1.6.1170.0", "1.7.104.0"})
    {
        REQUIRE(SupportsQualifiedNativeAccess(version, NativeContract::ResetMain));
        REQUIRE(SupportsQualifiedNativeAccess(version, NativeContract::GrabRelease));
        REQUIRE(SupportsQualifiedNativeAccess(version, NativeContract::FinalAppearance));
        REQUIRE(SupportsQualifiedNativeAccess(version, NativeContract::LocalSeating));
        for (const auto& contract : NativeContracts)
            if (contract.Id != NativeContract::ResetMain && contract.Id != NativeContract::GrabRelease &&
                contract.Id != NativeContract::FinalAppearance && contract.Id != NativeContract::LocalSeating)
                REQUIRE_FALSE(SupportsQualifiedNativeAccess(version, contract.Id));
    }
    for (const auto version : {"", "1.6.1179.0", "1.7.99.0", "1.7.104", "1.7.104.1", "1.7.105.0", "1.7.104.0-gog"})
        for (const auto contract : {NativeContract::ResetMain, NativeContract::GrabRelease,
                                   NativeContract::FinalAppearance, NativeContract::LocalSeating, NativeContract::Count})
            REQUIRE_FALSE(SupportsQualifiedNativeAccess(version, contract));
}
