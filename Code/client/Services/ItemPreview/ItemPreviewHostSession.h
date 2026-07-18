#pragma once

#include <atomic>
#include <cstdint>

class ItemPreviewHostSession
{
public:
    enum class Command : std::uint8_t
    {
        None,
        Show,
        Hide
    };

    struct ShownResult
    {
        Command command{Command::None};
        bool applySelection{};
    };

    ItemPreviewHostSession() noexcept = default;

    TP_NOCOPYMOVE(ItemPreviewHostSession);

    [[nodiscard]] Command RequestOpen() noexcept;
    [[nodiscard]] Command RequestClose() noexcept;
    [[nodiscard]] ShownResult OnShown() noexcept;
    [[nodiscard]] Command OnHidden() noexcept;

    [[nodiscard]] bool IsOpen() const noexcept
    {
        return m_open.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool WantsOpen() const noexcept
    {
        return m_wantsOpen.load(std::memory_order_acquire);
    }

private:
    [[nodiscard]] Command QueueShow() noexcept;
    [[nodiscard]] Command QueueHide() noexcept;

    std::atomic_bool m_open{};
    std::atomic_bool m_messagePending{};
    std::atomic_bool m_wantsOpen{};
};
