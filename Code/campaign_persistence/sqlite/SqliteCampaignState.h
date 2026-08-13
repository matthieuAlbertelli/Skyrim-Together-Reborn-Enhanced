#pragma once

#include <CampaignTypes.h>

#include <string_view>

struct sqlite3;

namespace STRE::Campaign::Sqlite
{
StoreResult InsertSlot(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotRecord& acSlot);
StoreResult VerifySlotBinding(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CampaignSlotId& acSlot,
    const CharacterBindingId& acBinding);
StoreResult UpsertCharacterBuild(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const CharacterBuildState& acBuild,
    StateVersion aRevision);
StoreResult UpsertAdapterState(
    sqlite3* apDatabase,
    const CampaignId& acCampaign,
    const AdapterState& acState,
    StateVersion aRevision);
StoreResult DeleteByTextKey(
    sqlite3* apDatabase,
    const char* apSql,
    const CampaignId& acCampaign,
    std::string_view acKey,
    std::string_view acContext);
}
