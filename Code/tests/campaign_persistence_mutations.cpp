#include <campaign_persistence_test_helpers.h>

#include <catch2/catch.hpp>

using namespace STRE::Campaign;
using namespace STRE::Campaign::Test;

TEST_CASE("Optimistic revisions idempotency journal and outbox are atomic", "[campaign.persistence][transaction]")
{
    TemporaryDatabase database;
    std::optional<TransactionStage> injectedStage;
    SqliteCampaignStoreOptions options;
    options.FaultInjector = [&](TransactionStage aStage)
    {
        return injectedStage == aStage;
    };
    auto store = OpenStore(database, options);
    REQUIRE(store->CreateCampaign(MakeCampaign()).Succeeded());

    CampaignMutationRequest mutation;
    mutation.Campaign = CampaignId{"campaign-1"};
    mutation.ExpectedRevision = 1;
    mutation.Mutation = MutationId{"mutation-update-1"};
    mutation.Kind = "UpdateCampaignState";
    mutation.MutationPayload = {0x60};
    mutation.CoreStateCodecVersion = 3;
    mutation.CoreStatePayload = Bytes{0x61, 0x62};
    mutation.Outbox = {{2, {0x63}}};
    MutationResult applied = store->ApplyMutation(mutation);
    REQUIRE(applied.Succeeded());
    REQUIRE(applied.Revision == 2);

    MutationResult duplicate = store->ApplyMutation(mutation);
    REQUIRE(duplicate.Succeeded());
    REQUIRE(duplicate.IdempotentReplay);
    REQUIRE(duplicate.Revision == 2);

    CampaignMutationRequest conflicting = mutation;
    conflicting.MutationPayload = {0xFF};
    REQUIRE(store->ApplyMutation(conflicting).Error == StoreError::IdempotencyConflict);

    CampaignMutationRequest stale = mutation;
    stale.Mutation = MutationId{"mutation-stale"};
    stale.MutationPayload = {0x64};
    REQUIRE(store->ApplyMutation(stale).Error == StoreError::StaleRevision);
    auto afterStale = store->LoadCampaign(CampaignId{"campaign-1"});
    REQUIRE(afterStale.Value.CurrentRevision == 2);
    REQUIRE(afterStale.Value.CoreStatePayload == Bytes{0x61, 0x62});

    injectedStage = TransactionStage::AfterJournal;
    CampaignMutationRequest faulted = mutation;
    faulted.ExpectedRevision = 2;
    faulted.Mutation = MutationId{"mutation-faulted"};
    faulted.MutationPayload = {0x70};
    faulted.CoreStatePayload = Bytes{0x71};
    REQUIRE(store->ApplyMutation(faulted).Error == StoreError::FaultInjected);
    injectedStage.reset();

    auto afterFault = store->LoadCampaign(CampaignId{"campaign-1"});
    REQUIRE(afterFault.Value.CurrentRevision == 2);
    REQUIRE(afterFault.Value.CoreStatePayload == Bytes{0x61, 0x62});
    auto journal = store->LoadJournal(CampaignId{"campaign-1"});
    REQUIRE(journal.Succeeded());
    REQUIRE(journal.Value.size() == 2);
    REQUIRE(journal.Value[0].ResultingRevision == 1);
    REQUIRE(journal.Value[1].ResultingRevision == 2);
    auto outbox = store->LoadPendingOutbox(CampaignId{"campaign-1"});
    REQUIRE(outbox.Succeeded());
    REQUIRE(outbox.Value.size() == 2);
    REQUIRE_FALSE(ExecuteRaw(
        database.Path,
        "UPDATE campaign_journal SET mutation_kind='rewritten' "
        "WHERE campaign_id='campaign-1';"));
    REQUIRE(store->LoadJournal(CampaignId{"campaign-1"}).Value.size() == 2);
    REQUIRE(store->MarkOutboxDelivered(outbox.Value.front().Id).Succeeded());
    REQUIRE(store->MarkOutboxDelivered(outbox.Value.front().Id).Succeeded());
    REQUIRE(store->LoadPendingOutbox(CampaignId{"campaign-1"}).Value.size() == 1);
}
