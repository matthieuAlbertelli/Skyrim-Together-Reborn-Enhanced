#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewHostSession.h>

ItemPreviewHostSession::Command ItemPreviewHostSession::RequestOpen() noexcept
{
    m_wantsOpen.store(true, std::memory_order_release);
    return QueueShow();
}

ItemPreviewHostSession::Command ItemPreviewHostSession::RequestClose() noexcept
{
    m_wantsOpen.store(false, std::memory_order_release);

    if (!m_open.load(std::memory_order_acquire) &&
        !m_messagePending.load(std::memory_order_acquire))
    {
        return Command::None;
    }

    return QueueHide();
}

ItemPreviewHostSession::ShownResult ItemPreviewHostSession::OnShown() noexcept
{
    m_open.store(true, std::memory_order_release);
    m_messagePending.store(false, std::memory_order_release);

    if (!m_wantsOpen.load(std::memory_order_acquire))
        return {QueueHide(), false};

    return {Command::None, true};
}

ItemPreviewHostSession::Command ItemPreviewHostSession::OnHidden() noexcept
{
    m_open.store(false, std::memory_order_release);
    m_messagePending.store(false, std::memory_order_release);

    if (!m_wantsOpen.load(std::memory_order_acquire))
        return Command::None;

    return QueueShow();
}

ItemPreviewHostSession::Command ItemPreviewHostSession::QueueShow() noexcept
{
    if (m_open.load(std::memory_order_acquire))
        return Command::None;

    bool expected = false;
    if (!m_messagePending.compare_exchange_strong(
            expected,
            true,
            std::memory_order_acq_rel))
    {
        return Command::None;
    }

    return Command::Show;
}

ItemPreviewHostSession::Command ItemPreviewHostSession::QueueHide() noexcept
{
    bool expected = false;
    (void)m_messagePending.compare_exchange_strong(
        expected,
        true,
        std::memory_order_acq_rel);

    // Preserve the previous host behavior: a hide request is forwarded even
    // when another UI message is already pending, so a stale show cannot keep
    // the native preview visible after its owner has released it.
    return Command::Hide;
}
