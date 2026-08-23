#pragma once

#include <Structs/Campaign.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace STRE::Campaign
{
struct CampaignLobbyAlias
{
    std::string JoinCode;
    std::string CampaignId;
    std::uint32_t PartyId{};
    std::unordered_map<std::string, std::string> DisplayNames;
};

class CampaignLobbyDirectory final
{
public:
    using CodeGenerator = std::function<std::string()>;

    explicit CampaignLobbyDirectory(
        CodeGenerator aCodeGenerator = {},
        std::size_t aMaximumAllocationAttempts = 32) noexcept;

    [[nodiscard]] std::optional<std::string> Allocate(
        std::string aCampaignId,
        std::uint32_t aPartyId) noexcept;
    [[nodiscard]] const CampaignLobbyAlias* Resolve(
        std::string_view acJoinCode) const noexcept;
    [[nodiscard]] const CampaignLobbyAlias* FindByCampaign(
        std::string_view acCampaignId) const noexcept;
    [[nodiscard]] CampaignLobbyAlias* FindByCampaign(
        std::string_view acCampaignId) noexcept;

    void RememberDisplayName(
        std::string_view acCampaignId,
        std::string aPlayerId,
        std::string aDisplayName) noexcept;
    void ForgetDisplayName(
        std::string_view acCampaignId,
        std::string_view acPlayerId) noexcept;
    void Invalidate(std::string_view acCampaignId) noexcept;

private:
    [[nodiscard]] static std::string GenerateRandomCode();

    CodeGenerator m_codeGenerator;
    std::size_t m_maximumAllocationAttempts{};
    std::unordered_map<std::string, CampaignLobbyAlias> m_byCode;
    std::unordered_map<std::string, std::string> m_codeByCampaign;
};
}
