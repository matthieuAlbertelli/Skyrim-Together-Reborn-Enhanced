#include <CampaignAdmissionService.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <random>
#include <utility>

namespace STRE::Campaign
{
namespace
{
bool IsLowerHex(char aValue) noexcept
{
    return (aValue >= '0' && aValue <= '9') ||
        (aValue >= 'a' && aValue <= 'f');
}

bool IsValidPlayerId(const std::string& acPlayerId) noexcept
{
    return acPlayerId.size() == 64 &&
        std::all_of(acPlayerId.begin(), acPlayerId.end(), IsLowerHex);
}

bool IsValidProtocolId(const std::string& acValue) noexcept
{
    if (acValue.empty() || acValue.size() > kCampaignWireMaximumIdLength)
        return false;
    return std::all_of(
        acValue.begin(), acValue.end(), [](char aValue)
        {
            return (aValue >= 'a' && aValue <= 'z') ||
                (aValue >= 'A' && aValue <= 'Z') ||
                (aValue >= '0' && aValue <= '9') ||
                aValue == '-' || aValue == '_';
        });
}

std::string DefaultGenerateId(std::string_view acPrefix)
{
    static constexpr std::array<char, 16> cHex{
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    std::random_device random;
    std::string result(acPrefix);
    result.reserve(acPrefix.size() + 32);
    for (std::size_t index = 0; index < 16; ++index)
    {
        const auto value = static_cast<unsigned char>(random());
        result.push_back(cHex[value >> 4]);
        result.push_back(cHex[value & 0x0F]);
    }
    return result;
}

CampaignProtocolResult MapCommandResult(
    const CampaignCommandResult& acResult) noexcept
{
    if (acResult.Succeeded())
    {
        if (acResult.IdempotentReplay)
            return CampaignProtocolResult::IdempotentReplay;
        return acResult.Applied ? CampaignProtocolResult::Applied
                                : CampaignProtocolResult::AcceptedNoOp;
    }
    if (acResult.PersistenceError == StoreError::IdempotencyConflict)
        return CampaignProtocolResult::IdempotencyConflict;
    if (acResult.PersistenceError == StoreError::NotFound)
        return CampaignProtocolResult::CampaignNotFound;
    switch (acResult.Error)
    {
    case CampaignError::StaleRevision:
        return CampaignProtocolResult::StaleRevision;
    case CampaignError::RosterSealed:
        return CampaignProtocolResult::RosterSealed;
    case CampaignError::RosterNotSealed:
        return CampaignProtocolResult::RosterNotSealed;
    case CampaignError::UnauthorizedActor:
        return CampaignProtocolResult::Unauthorized;
    case CampaignError::NotCampaignMember:
        return CampaignProtocolResult::IdentityMismatch;
    case CampaignError::RosterIncomplete:
        return CampaignProtocolResult::RosterIncomplete;
    case CampaignError::InvalidIdentity:
    case CampaignError::InvalidRoster:
    case CampaignError::RosterLimitExceeded:
    case CampaignError::DuplicateSlot:
    case CampaignError::DuplicatePlayer:
    case CampaignError::DuplicateCharacterBinding:
    case CampaignError::SlotNotFound:
    case CampaignError::InvalidPhase:
    case CampaignError::InvalidSessionManager:
        return CampaignProtocolResult::InvalidRequest;
    default: return CampaignProtocolResult::PersistenceFailure;
    }
}

CampaignProtocolCommandResult Failure(
    CampaignProtocolOperation aOperation,
    CampaignProtocolResult aResult,
    std::string aCampaignId = {})
{
    CampaignProtocolCommandResult result;
    result.Operation = aOperation;
    result.Result = aResult;
    result.CampaignId = std::move(aCampaignId);
    return result;
}

CampaignProtocolCommandResult FromCommand(
    CampaignProtocolOperation aOperation,
    const std::string& acCampaignId,
    const CampaignCommandResult& acCommand)
{
    CampaignProtocolCommandResult result;
    result.Operation = aOperation;
    result.Result = MapCommandResult(acCommand);
    result.CampaignId = acCampaignId;
    result.Version = acCommand.Version;
    return result;
}

CampaignProtocolResult MapLoadFailure(const CampaignLoadResult& acLoad) noexcept
{
    if (acLoad.PersistenceError == StoreError::NotFound)
        return CampaignProtocolResult::CampaignNotFound;
    return CampaignProtocolResult::PersistenceFailure;
}
}

CampaignAdmissionService::CampaignAdmissionService(
    CampaignRuntimeService& aRuntime,
    IdGenerator aIdGenerator) noexcept
    : m_runtime(aRuntime)
    , m_idGenerator(std::move(aIdGenerator))
{
    if (!m_idGenerator)
        m_idGenerator = DefaultGenerateId;
}

std::string CampaignAdmissionService::GenerateId(std::string_view acPrefix)
{
    return m_idGenerator(acPrefix);
}

CampaignConnectionRegistration CampaignAdmissionService::RegisterConnection(
    CampaignConnectionHandle aConnection,
    std::string aPlayerId) noexcept
{
    try
    {
        if (!IsValidPlayerId(aPlayerId))
            return CampaignConnectionRegistration::InvalidPlayerId;
        const auto duplicate = std::find_if(
            m_connections.begin(), m_connections.end(),
            [&](const CampaignAdmissionRecord& acRecord)
            {
                return acRecord.Connection != aConnection &&
                    acRecord.Player.Value == aPlayerId;
            });
        if (duplicate != m_connections.end())
            return CampaignConnectionRegistration::DuplicateActivePlayerId;
        if (CampaignAdmissionRecord* const pExisting = FindConnection(aConnection))
        {
            if (pExisting->Player.Value == aPlayerId)
                return CampaignConnectionRegistration::Accepted;
            return CampaignConnectionRegistration::DuplicateActivePlayerId;
        }
        m_connections.push_back(
            {aConnection, PlayerId{std::move(aPlayerId)}, std::nullopt,
             std::nullopt});
        return CampaignConnectionRegistration::Accepted;
    }
    catch (...)
    {
        return CampaignConnectionRegistration::InvalidPlayerId;
    }
}

CampaignAdmissionRecord* CampaignAdmissionService::FindConnection(
    CampaignConnectionHandle aConnection) noexcept
{
    const auto it = std::find_if(
        m_connections.begin(), m_connections.end(),
        [aConnection](const CampaignAdmissionRecord& acRecord)
        {
            return acRecord.Connection == aConnection;
        });
    return it == m_connections.end() ? nullptr : &*it;
}

const CampaignAdmissionRecord* CampaignAdmissionService::FindConnection(
    CampaignConnectionHandle aConnection) const noexcept
{
    const auto it = std::find_if(
        m_connections.begin(), m_connections.end(),
        [aConnection](const CampaignAdmissionRecord& acRecord)
        {
            return acRecord.Connection == aConnection;
        });
    return it == m_connections.end() ? nullptr : &*it;
}

std::vector<CampaignMemberPresence> CampaignAdmissionService::BuildPresence(
    const CampaignId& acCampaign) const
{
    std::vector<CampaignMemberPresence> result;
    for (const CampaignAdmissionRecord& connection : m_connections)
    {
        if (!connection.AdmittedIdentity ||
            connection.AdmittedIdentity->Campaign != acCampaign)
        {
            continue;
        }
        result.push_back({*connection.AdmittedIdentity, true, true});
    }
    return result;
}

std::vector<CampaignConnectionHandle>
CampaignAdmissionService::GetAdmittedConnections(
    const CampaignId& acCampaign) const
{
    std::vector<CampaignConnectionHandle> result;
    for (const CampaignAdmissionRecord& connection : m_connections)
    {
        if (connection.AdmittedIdentity &&
            connection.AdmittedIdentity->Campaign == acCampaign)
        {
            result.push_back(connection.Connection);
        }
    }
    return result;
}

std::optional<CampaignSnapshotData> CampaignAdmissionService::BuildSnapshot(
    const CampaignId& acCampaign) noexcept try
{
    CampaignLoadResult loaded = m_runtime.LoadCampaign(
        acCampaign, BuildPresence(acCampaign));
    if (!loaded)
        return std::nullopt;

    CampaignSnapshotData snapshot;
    snapshot.CampaignId = loaded.Campaign.Id.Value.c_str();
    snapshot.StateVersion = loaded.Campaign.Version;
    snapshot.Phase = static_cast<std::uint8_t>(loaded.Campaign.Phase);
    snapshot.RuntimeState = static_cast<std::uint8_t>(loaded.RuntimeState);
    snapshot.RosterSealed = loaded.Campaign.RosterSealed;
    if (loaded.Campaign.SessionManager)
    {
        snapshot.SessionManagerPlayerId =
            loaded.Campaign.SessionManager->Value.c_str();
    }

    const std::vector<CampaignMemberPresence> presence =
        BuildPresence(acCampaign);
    for (const CampaignSlotState& slot : loaded.Campaign.Roster)
    {
        const bool present = std::any_of(
            presence.begin(), presence.end(),
            [&](const CampaignMemberPresence& acPresence)
            {
                return acPresence.Identity.Slot == slot.Slot &&
                    acPresence.Identity.Player == slot.Player &&
                    acPresence.Identity.CharacterBinding ==
                        slot.CharacterBinding;
            });
        CampaignPublicSlotData projected;
        projected.SlotId = slot.Slot.Value.c_str();
        projected.PlayerId = slot.Player.Value.c_str();
        projected.Ready = slot.Ready;
        projected.Present = present;
        snapshot.Roster.push_back(std::move(projected));
    }
    return snapshot;
}
catch (...)
{
    return std::nullopt;
}

std::optional<CampaignSnapshotData> CampaignAdmissionService::Disconnect(
    CampaignConnectionHandle aConnection) noexcept
{
    try
    {
        CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
        if (!pRecord)
            return std::nullopt;
        std::optional<CampaignId> campaign;
        if (pRecord->AdmittedIdentity)
            campaign = pRecord->AdmittedIdentity->Campaign;
        m_connections.erase(std::remove_if(
            m_connections.begin(), m_connections.end(),
            [aConnection](const CampaignAdmissionRecord& acRecord)
            {
                return acRecord.Connection == aConnection;
            }), m_connections.end());
        return campaign ? BuildSnapshot(*campaign) : std::nullopt;
    }
    catch (...)
    {
        return std::nullopt;
    }
}

CampaignProtocolCommandResult CampaignAdmissionService::CreateCampaign(
    CampaignConnectionHandle aConnection,
    const std::string& acMutationId,
    bool aPartyLeader) noexcept
{
    try
    {
        CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
        if (!pRecord || !IsValidProtocolId(acMutationId))
            return Failure(CampaignProtocolOperation::Create,
                CampaignProtocolResult::InvalidRequest);
        CampaignCreationLookupResult creation =
            m_runtime.FindCampaignCreation(
                pRecord->Player, MutationId{acMutationId});
        if (!creation)
        {
            return Failure(CampaignProtocolOperation::Create,
                CampaignProtocolResult::PersistenceFailure);
        }
        if (creation.Identity)
        {
            if (pRecord->AdmittedIdentity &&
                pRecord->AdmittedIdentity->Campaign !=
                    creation.Identity->Campaign)
            {
                return Failure(
                    CampaignProtocolOperation::Create,
                    CampaignProtocolResult::NotAdmitted,
                    creation.Identity->Campaign.Value);
            }
            CampaignLoadResult current = m_runtime.LoadCampaign(
                creation.Identity->Campaign);
            if (!current)
            {
                return Failure(
                    CampaignProtocolOperation::Create,
                    MapLoadFailure(current),
                    creation.Identity->Campaign.Value);
            }
            const auto currentSlot = std::find_if(
                current.Campaign.Roster.begin(),
                current.Campaign.Roster.end(),
                [&](const CampaignSlotState& acSlot)
                {
                    return acSlot.Player == creation.Identity->Player;
                });
            if (currentSlot == current.Campaign.Roster.end() ||
                currentSlot->Slot != creation.Identity->Slot)
            {
                return Failure(
                    CampaignProtocolOperation::Create,
                    CampaignProtocolResult::IdentityMismatch,
                    creation.Identity->Campaign.Value);
            }
            if (currentSlot->CharacterBinding !=
                creation.Identity->CharacterBinding)
            {
                return Failure(
                    CampaignProtocolOperation::Create,
                    CampaignProtocolResult::BindingMismatch,
                    creation.Identity->Campaign.Value);
            }
            CampaignProtocolCommandResult replay;
            replay.Operation = CampaignProtocolOperation::Create;
            replay.Result = CampaignProtocolResult::IdempotentReplay;
            replay.CampaignId = creation.Identity->Campaign.Value;
            replay.Version = creation.Version;
            replay.CampaignSlotId = creation.Identity->Slot.Value;
            replay.CharacterBindingId =
                creation.Identity->CharacterBinding.Value;
            pRecord->AdmittedIdentity = *creation.Identity;
            replay.Snapshot = BuildSnapshot(creation.Identity->Campaign);
            return replay;
        }
        if (pRecord->AdmittedIdentity)
            return Failure(CampaignProtocolOperation::Create,
                CampaignProtocolResult::InvalidRequest);
        if (!aPartyLeader)
            return Failure(CampaignProtocolOperation::Create,
                CampaignProtocolResult::Unauthorized);

        const CampaignId campaign{GenerateId("campaign-")};
        const CampaignSlotRecord hostSlot{
            CampaignSlotId{"slot-01"}, pRecord->Player,
            CharacterBindingId{GenerateId("binding-")}};
        CampaignCommandResult command = m_runtime.CreateLobbyCampaign(
            {campaign, MutationId{acMutationId}, {hostSlot}});
        CampaignProtocolCommandResult result = FromCommand(
            CampaignProtocolOperation::Create, campaign.Value, command);
        if (!result.Succeeded())
            return result;

        pRecord->AdmittedIdentity = CampaignMemberIdentity{
            campaign, hostSlot.Slot, hostSlot.Player,
            hostSlot.CharacterBinding};
        result.CampaignSlotId = hostSlot.Slot.Value;
        result.CharacterBindingId = hostSlot.CharacterBinding.Value;
        result.Snapshot = BuildSnapshot(campaign);
        return result;
    }
    catch (...)
    {
        return Failure(CampaignProtocolOperation::Create,
            CampaignProtocolResult::PersistenceFailure);
    }
}

CampaignProtocolCommandResult CampaignAdmissionService::JoinCampaign(
    CampaignConnectionHandle aConnection,
    const std::string& acCampaignId,
    const std::string& acMutationId,
    StateVersion aExpectedRevision,
    bool aSameLiveSession) noexcept
{
    try
    {
        CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
        if (!pRecord || !IsValidProtocolId(acCampaignId) ||
            !IsValidProtocolId(acMutationId))
        {
            return Failure(CampaignProtocolOperation::Join,
                CampaignProtocolResult::InvalidRequest, acCampaignId);
        }
        if (!aSameLiveSession)
            return Failure(CampaignProtocolOperation::Join,
                CampaignProtocolResult::SessionMismatch, acCampaignId);
        if (pRecord->AdmittedIdentity &&
            pRecord->AdmittedIdentity->Campaign.Value != acCampaignId)
        {
            return Failure(CampaignProtocolOperation::Join,
                CampaignProtocolResult::NotAdmitted, acCampaignId);
        }

        const CampaignId campaign{acCampaignId};
        CampaignLoadResult loaded = m_runtime.LoadCampaign(campaign);
        if (!loaded)
            return Failure(CampaignProtocolOperation::Join,
                MapLoadFailure(loaded), acCampaignId);
        if (loaded.Campaign.RosterSealed)
            return Failure(CampaignProtocolOperation::Join,
                CampaignProtocolResult::RosterSealed, acCampaignId);

        const auto existing = std::find_if(
            loaded.Campaign.Roster.begin(), loaded.Campaign.Roster.end(),
            [&](const CampaignSlotState& acSlot)
            {
                return acSlot.Player == pRecord->Player;
            });
        CampaignSlotRecord slot;
        if (existing != loaded.Campaign.Roster.end())
        {
            slot = {existing->Slot, existing->Player,
                    existing->CharacterBinding};
        }
        else
        {
            std::size_t index = 1;
            for (; index <= kMaximumCampaignRosterSize; ++index)
            {
                char slotName[16]{};
                std::snprintf(slotName, sizeof(slotName), "slot-%02zu", index);
                const bool used = std::any_of(
                    loaded.Campaign.Roster.begin(),
                    loaded.Campaign.Roster.end(),
                    [&](const CampaignSlotState& acSlot)
                    {
                        return acSlot.Slot.Value == slotName;
                    });
                if (!used)
                {
                    slot.Slot.Value = slotName;
                    break;
                }
            }
            if (slot.Slot.Value.empty())
                return Failure(CampaignProtocolOperation::Join,
                    CampaignProtocolResult::InvalidRequest, acCampaignId);
            slot.Player = pRecord->Player;
            slot.CharacterBinding = CharacterBindingId{
                GenerateId("binding-")};
        }

        CampaignCommandResult command = m_runtime.AddRosterSlot(
            {campaign, aExpectedRevision, MutationId{acMutationId}, slot});
        CampaignProtocolCommandResult result = FromCommand(
            CampaignProtocolOperation::Join, acCampaignId, command);
        if (existing != loaded.Campaign.Roster.end() &&
            !command.Succeeded() &&
            (command.Error == CampaignError::DuplicateSlot ||
             command.Error == CampaignError::DuplicatePlayer ||
             command.Error == CampaignError::DuplicateCharacterBinding))
        {
            result.Result =
                CampaignProtocolResult::ExistingMembershipRequiresResume;
            result.Version = loaded.Campaign.Version;
        }
        if (!result.Succeeded())
            return result;

        pRecord->AdmittedIdentity = CampaignMemberIdentity{
            campaign, slot.Slot, slot.Player, slot.CharacterBinding};
        result.CampaignSlotId = slot.Slot.Value;
        result.CharacterBindingId = slot.CharacterBinding.Value;
        result.Snapshot = BuildSnapshot(campaign);
        return result;
    }
    catch (...)
    {
        return Failure(CampaignProtocolOperation::Join,
            CampaignProtocolResult::PersistenceFailure, acCampaignId);
    }
}

CampaignProtocolCommandResult CampaignAdmissionService::ResumeCampaign(
    CampaignConnectionHandle aConnection,
    const std::string& acCampaignId,
    const std::string& acCharacterBindingId) noexcept
{
    try
    {
        CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
        if (!pRecord || !IsValidProtocolId(acCampaignId) ||
            !IsValidProtocolId(acCharacterBindingId))
        {
            return Failure(CampaignProtocolOperation::Resume,
                CampaignProtocolResult::InvalidRequest, acCampaignId);
        }
        if (pRecord->AdmittedIdentity &&
            pRecord->AdmittedIdentity->Campaign.Value != acCampaignId)
        {
            return Failure(CampaignProtocolOperation::Resume,
                CampaignProtocolResult::NotAdmitted, acCampaignId);
        }

        const CampaignId campaign{acCampaignId};
        CampaignLoadResult loaded = m_runtime.LoadCampaign(campaign);
        if (!loaded)
            return Failure(CampaignProtocolOperation::Resume,
                MapLoadFailure(loaded), acCampaignId);
        const auto slot = std::find_if(
            loaded.Campaign.Roster.begin(), loaded.Campaign.Roster.end(),
            [&](const CampaignSlotState& acSlot)
            {
                return acSlot.Player == pRecord->Player;
            });
        if (slot == loaded.Campaign.Roster.end())
            return Failure(CampaignProtocolOperation::Resume,
                CampaignProtocolResult::IdentityMismatch, acCampaignId);
        if (slot->CharacterBinding.Value != acCharacterBindingId)
            return Failure(CampaignProtocolOperation::Resume,
                CampaignProtocolResult::BindingMismatch, acCampaignId);

        const bool alreadyAdmitted = pRecord->AdmittedIdentity.has_value();
        pRecord->AdmittedIdentity = CampaignMemberIdentity{
            campaign, slot->Slot, slot->Player, slot->CharacterBinding};
        CampaignProtocolCommandResult result;
        result.Operation = CampaignProtocolOperation::Resume;
        result.Result = alreadyAdmitted
            ? CampaignProtocolResult::AcceptedNoOp
            : CampaignProtocolResult::Applied;
        result.CampaignId = acCampaignId;
        result.Version = loaded.Campaign.Version;
        result.CampaignSlotId = slot->Slot.Value;
        result.CharacterBindingId = slot->CharacterBinding.Value;
        result.Snapshot = BuildSnapshot(campaign);
        return result;
    }
    catch (...)
    {
        return Failure(CampaignProtocolOperation::Resume,
            CampaignProtocolResult::PersistenceFailure, acCampaignId);
    }
}

CampaignProtocolCommandResult CampaignAdmissionService::StartCampaign(
    CampaignConnectionHandle aConnection,
    const std::string& acCampaignId,
    const std::string& acMutationId,
    StateVersion aExpectedRevision,
    bool aPartyLeader,
    bool aSameLiveSession) noexcept try
{
    CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
    if (!pRecord || !pRecord->AdmittedIdentity ||
        pRecord->AdmittedIdentity->Campaign.Value != acCampaignId)
    {
        return Failure(CampaignProtocolOperation::Start,
            CampaignProtocolResult::NotAdmitted, acCampaignId);
    }
    if (!IsValidProtocolId(acMutationId))
        return Failure(CampaignProtocolOperation::Start,
            CampaignProtocolResult::InvalidRequest, acCampaignId);
    if (!aPartyLeader)
        return Failure(CampaignProtocolOperation::Start,
            CampaignProtocolResult::Unauthorized, acCampaignId);
    if (!aSameLiveSession)
        return Failure(CampaignProtocolOperation::Start,
            CampaignProtocolResult::SessionMismatch, acCampaignId);

    const CampaignId campaign{acCampaignId};
    CampaignCommandResult command = m_runtime.CommitCampaignStart(
        {campaign, aExpectedRevision, MutationId{acMutationId},
         pRecord->Player});
    CampaignProtocolCommandResult result = FromCommand(
        CampaignProtocolOperation::Start, acCampaignId, command);
    if (result.Succeeded())
        result.Snapshot = BuildSnapshot(campaign);
    return result;
}
catch (...)
{
    return Failure(CampaignProtocolOperation::Start,
        CampaignProtocolResult::PersistenceFailure, acCampaignId);
}

CampaignProtocolCommandResult CampaignAdmissionService::SetReady(
    CampaignConnectionHandle aConnection,
    const std::string& acCampaignId,
    const std::string& acMutationId,
    StateVersion aExpectedRevision,
    bool aReady) noexcept try
{
    CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
    if (!pRecord || !pRecord->AdmittedIdentity ||
        pRecord->AdmittedIdentity->Campaign.Value != acCampaignId)
    {
        return Failure(CampaignProtocolOperation::SetReady,
            CampaignProtocolResult::NotAdmitted, acCampaignId);
    }
    if (!IsValidProtocolId(acMutationId))
        return Failure(CampaignProtocolOperation::SetReady,
            CampaignProtocolResult::InvalidRequest, acCampaignId);

    const CampaignId campaign{acCampaignId};
    CampaignCommandResult command = m_runtime.SetReady(
        {campaign, aExpectedRevision, MutationId{acMutationId},
         *pRecord->AdmittedIdentity, aReady});
    CampaignProtocolCommandResult result = FromCommand(
        CampaignProtocolOperation::SetReady, acCampaignId, command);
    if (result.Succeeded())
        result.Snapshot = BuildSnapshot(campaign);
    return result;
}
catch (...)
{
    return Failure(CampaignProtocolOperation::SetReady,
        CampaignProtocolResult::PersistenceFailure, acCampaignId);
}

CampaignProtocolCommandResult CampaignAdmissionService::LeaveCampaign(
    CampaignConnectionHandle aConnection,
    const std::string& acCampaignId,
    const std::string& acMutationId,
    StateVersion aExpectedRevision) noexcept try
{
    CampaignAdmissionRecord* const pRecord = FindConnection(aConnection);
    if (!pRecord || !IsValidProtocolId(acCampaignId) ||
        !IsValidProtocolId(acMutationId))
    {
        return Failure(CampaignProtocolOperation::Leave,
            CampaignProtocolResult::InvalidRequest, acCampaignId);
    }
    const CampaignMemberIdentity* pIdentity = nullptr;
    if (pRecord->AdmittedIdentity &&
        pRecord->AdmittedIdentity->Campaign.Value == acCampaignId)
    {
        pIdentity = &*pRecord->AdmittedIdentity;
    }
    else if (pRecord->PreviousAdmission &&
        pRecord->PreviousAdmission->Campaign.Value == acCampaignId)
    {
        pIdentity = &*pRecord->PreviousAdmission;
    }
    if (!pIdentity)
        return Failure(CampaignProtocolOperation::Leave,
            CampaignProtocolResult::NotAdmitted, acCampaignId);

    const CampaignId campaign{acCampaignId};
    CampaignCommandResult command = m_runtime.RemoveRosterSlot(
        {campaign, aExpectedRevision, MutationId{acMutationId},
         pIdentity->Slot});
    CampaignProtocolCommandResult result = FromCommand(
        CampaignProtocolOperation::Leave, acCampaignId, command);
    if (!result.Succeeded())
        return result;
    if (pRecord->AdmittedIdentity)
    {
        pRecord->PreviousAdmission = pRecord->AdmittedIdentity;
        pRecord->AdmittedIdentity.reset();
    }
    result.Snapshot = BuildSnapshot(campaign);
    return result;
}
catch (...)
{
    return Failure(CampaignProtocolOperation::Leave,
        CampaignProtocolResult::PersistenceFailure, acCampaignId);
}
}
