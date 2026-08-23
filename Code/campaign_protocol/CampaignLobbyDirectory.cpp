#include <CampaignLobbyDirectory.h>

#include <algorithm>
#include <array>
#include <random>
#include <utility>

namespace STRE::Campaign
{
CampaignLobbyDirectory::CampaignLobbyDirectory(
    CodeGenerator aCodeGenerator,
    std::size_t aMaximumAllocationAttempts) noexcept
    : m_codeGenerator(std::move(aCodeGenerator))
    , m_maximumAllocationAttempts(aMaximumAllocationAttempts)
{
    if (!m_codeGenerator)
        m_codeGenerator = GenerateRandomCode;
}

std::string CampaignLobbyDirectory::GenerateRandomCode()
{
    static thread_local std::mt19937 generator{std::random_device{}()};
    std::uniform_int_distribution<std::size_t> distribution(
        0, kCampaignJoinCodeAlphabet.size() - 1);

    std::string result;
    result.reserve(kCampaignJoinCodeLength);
    for (std::size_t index = 0; index < kCampaignJoinCodeLength; ++index)
        result.push_back(kCampaignJoinCodeAlphabet[distribution(generator)]);
    return result;
}

std::optional<std::string> CampaignLobbyDirectory::Allocate(
    std::string aCampaignId,
    std::uint32_t aPartyId) noexcept
{
    try
    {
        if (aCampaignId.empty())
            return std::nullopt;

        if (CampaignLobbyAlias* const pExisting = FindByCampaign(aCampaignId))
        {
            pExisting->PartyId = aPartyId;
            return pExisting->JoinCode;
        }

        for (std::size_t attempt = 0;
             attempt < m_maximumAllocationAttempts;
             ++attempt)
        {
            TiltedPhoques::String normalized;
            if (!NormalizeCampaignJoinCode(m_codeGenerator(), normalized))
                continue;

            const std::string code = normalized.c_str();
            if (m_byCode.contains(code))
                continue;

            CampaignLobbyAlias lobby;
            lobby.JoinCode = code;
            lobby.CampaignId = std::move(aCampaignId);
            lobby.PartyId = aPartyId;
            m_codeByCampaign.emplace(lobby.CampaignId, code);
            m_byCode.emplace(code, std::move(lobby));
            return code;
        }
    }
    catch (...)
    {
    }
    return std::nullopt;
}

const CampaignLobbyAlias* CampaignLobbyDirectory::Resolve(
    std::string_view acJoinCode) const noexcept
{
    try
    {
        TiltedPhoques::String normalized;
        if (!NormalizeCampaignJoinCode(acJoinCode, normalized))
            return nullptr;
        const auto it = m_byCode.find(normalized.c_str());
        return it == m_byCode.end() ? nullptr : &it->second;
    }
    catch (...)
    {
        return nullptr;
    }
}

const CampaignLobbyAlias* CampaignLobbyDirectory::FindByCampaign(
    std::string_view acCampaignId) const noexcept
{
    try
    {
        const auto code = m_codeByCampaign.find(std::string(acCampaignId));
        if (code == m_codeByCampaign.end())
            return nullptr;
        const auto lobby = m_byCode.find(code->second);
        return lobby == m_byCode.end() ? nullptr : &lobby->second;
    }
    catch (...)
    {
        return nullptr;
    }
}

CampaignLobbyAlias* CampaignLobbyDirectory::FindByCampaign(
    std::string_view acCampaignId) noexcept
{
    return const_cast<CampaignLobbyAlias*>(
        static_cast<const CampaignLobbyDirectory&>(*this)
            .FindByCampaign(acCampaignId));
}

void CampaignLobbyDirectory::RememberDisplayName(
    std::string_view acCampaignId,
    std::string aPlayerId,
    std::string aDisplayName) noexcept
{
    try
    {
        CampaignLobbyAlias* const pLobby = FindByCampaign(acCampaignId);
        TiltedPhoques::String normalized;
        if (!pLobby || aPlayerId.empty() ||
            !NormalizeCampaignLobbyDisplayName(aDisplayName, normalized))
        {
            return;
        }
        pLobby->DisplayNames[std::move(aPlayerId)] = normalized.c_str();
    }
    catch (...)
    {
    }
}

void CampaignLobbyDirectory::ForgetDisplayName(
    std::string_view acCampaignId,
    std::string_view acPlayerId) noexcept
{
    try
    {
        CampaignLobbyAlias* const pLobby = FindByCampaign(acCampaignId);
        if (pLobby)
            pLobby->DisplayNames.erase(std::string(acPlayerId));
    }
    catch (...)
    {
    }
}

void CampaignLobbyDirectory::Invalidate(
    std::string_view acCampaignId) noexcept
{
    try
    {
        const auto code = m_codeByCampaign.find(std::string(acCampaignId));
        if (code == m_codeByCampaign.end())
            return;
        m_byCode.erase(code->second);
        m_codeByCampaign.erase(code);
    }
    catch (...)
    {
    }
}
}
