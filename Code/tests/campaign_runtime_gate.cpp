#include <Campaign/CampaignRuntimeGate.h>

#include <catch2/catch.hpp>

using STRE::Campaign::CampaignRuntimeGate;
using STRE::Campaign::CampaignRuntimeGateState;

TEST_CASE("Campaign runtime gate follows explicit load transitions", "[campaign.gate]")
{
    CampaignRuntimeGate gate;

    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.IsNextLoadArmed());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::ArmedDuringLoad);

    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.Release());
    REQUIRE(gate.GetState() == CampaignRuntimeGateState::Released);
}

TEST_CASE("Campaign runtime gate arm and release are idempotent", "[campaign.gate]")
{
    CampaignRuntimeGate gate;

    REQUIRE(gate.ArmNextLoad());
    REQUIRE_FALSE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    REQUIRE_FALSE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());
    REQUIRE_FALSE(gate.OnPostLoad());
    REQUIRE(gate.Release());
    REQUIRE_FALSE(gate.Release());
}

TEST_CASE("Normal and failed loads do not leave the campaign gate locked", "[campaign.gate]")
{
    CampaignRuntimeGate normal;
    REQUIRE_FALSE(normal.OnNativeLoadEnter());
    normal.OnNativeLoadReturn(true);
    REQUIRE_FALSE(normal.OnPostLoad());
    REQUIRE_FALSE(normal.IsLocked());

    CampaignRuntimeGate failed;
    REQUIRE(failed.ArmNextLoad());
    REQUIRE(failed.OnNativeLoadEnter());
    failed.OnNativeLoadReturn(false);
    REQUIRE(failed.GetState() == CampaignRuntimeGateState::Open);
    REQUIRE_FALSE(failed.OnPostLoad());
}

TEST_CASE("Presentation observations cannot release the campaign gate", "[campaign.gate]")
{
    CampaignRuntimeGate gate;
    REQUIRE(gate.ArmNextLoad());
    REQUIRE(gate.OnNativeLoadEnter());
    gate.OnNativeLoadReturn(true);
    REQUIRE(gate.OnPostLoad());

    gate.ObserveGuardMenu(true);
    gate.ObserveCefPresentation(true);
    REQUIRE(gate.IsLocked());
    REQUIRE(gate.IsGuardMenuObserved());
    REQUIRE(gate.IsCefPresentationObserved());

    gate.ObserveGuardMenu(false);
    gate.ObserveCefPresentation(false);
    REQUIRE(gate.IsLocked());
    REQUIRE_FALSE(gate.IsGuardMenuObserved());
    REQUIRE_FALSE(gate.IsCefPresentationObserved());
}
