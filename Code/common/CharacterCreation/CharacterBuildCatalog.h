#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace STRE::CharacterCreation
{
inline constexpr std::uint32_t kCharacterBuildVersion = 4;
inline constexpr std::size_t kMaximumSelectionCount = 32;

struct ItemGrant
{
    const char* PluginName{};
    std::uint32_t LocalFormId{};
    std::int32_t Count{};
};

enum class EquipmentSide : std::uint8_t
{
    Right,
    Left
};

struct EquipmentGrant
{
    const char* PluginName{};
    std::uint32_t LocalFormId{};
    std::int32_t Count{};
    EquipmentSide Side{EquipmentSide::Right};
};


[[nodiscard]] bool IsSupportedClassId(
    std::string_view aClassId) noexcept;

[[nodiscard]] bool IsSupportedLoadoutOption(
    std::string_view aClassId,
    std::string_view aGroupId,
    std::string_view aOptionId) noexcept;

[[nodiscard]] bool IsLoadoutGroupActive(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections,
    std::string_view aGroupId) noexcept;

[[nodiscard]] bool HasCompleteLoadout(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections) noexcept;

[[nodiscard]] bool ValidateSelections(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections) noexcept;

[[nodiscard]] std::vector<ItemGrant> BuildItemGrants(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections);

[[nodiscard]] std::vector<EquipmentGrant> BuildEquipmentGrants(
    std::string_view aClassId,
    const std::map<std::string, std::string>& acSelections);
}
