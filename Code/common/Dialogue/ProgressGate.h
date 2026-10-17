#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>

namespace STRE::Dialogue
{
inline constexpr std::string_view cPlugin = "STRE_AlternateStart.esp";

struct ResponseKey
{
    std::uintptr_t Menu{}, Movie{}, Info{}, Response{};
    std::uint32_t FormId{}, Speaker{};
    std::uint64_t Session{};
    bool operator==(const ResponseKey&) const = default;
};

struct Observation
{
    ResponseKey Key;
    std::string_view Plugin;
    bool MenuOpen{};
    std::optional<bool> AllowProgress;
};

enum class GateAction
{
    None,
    Arm,
    Enable
};

// Pure permission gate: never emits input or a dialogue progression command.
class ProgressGate
{
public:
    using Clock = std::chrono::steady_clock;
    static constexpr auto cDelay = std::chrono::milliseconds(750);

    GateAction Observe(const Observation& aSample, Clock::time_point aNow) noexcept
    {
        if (!aSample.MenuOpen || !aSample.Key.Menu || !aSample.Key.Movie || !aSample.Key.Info || !aSample.Key.FormId || aSample.Plugin != cPlugin ||
            !aSample.AllowProgress.has_value())
        {
            Reset();
            return GateAction::None;
        }
        if (m_key != aSample.Key)
        {
            Reset();
            m_key = aSample.Key;
        }
        if (*aSample.AllowProgress)
        {
            m_armed = m_attempted = false;
            return GateAction::None;
        }
        if (m_attempted)
            return GateAction::None;
        if (!m_armed || aNow < m_started)
        {
            m_started = aNow;
            m_armed = true;
            return GateAction::Arm;
        }
        if (aNow - m_started < cDelay)
            return GateAction::None;
        m_attempted = true;
        m_armed = false;
        return GateAction::Enable;
    }

    // A confirmed true value permits a new false edge even when vanilla skips
    // between two frame observations. Failed writes are not retried per frame.
    void ConfirmEnabled() noexcept { m_armed = m_attempted = false; }
    void Reset() noexcept
    {
        m_key = {};
        m_armed = m_attempted = false;
        m_started = {};
    }

private:
    ResponseKey m_key{};
    Clock::time_point m_started{};
    bool m_armed{}, m_attempted{};
};
} // namespace STRE::Dialogue
