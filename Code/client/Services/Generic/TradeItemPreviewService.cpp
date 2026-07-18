#include <TiltedOnlinePCH.h>

#include <Services/TradeItemPreviewService.h>

#include <Forms/TESBoundObject.h>
#include <Forms/TESForm.h>
#include <Interface/Menus/TradePreviewHostMenu.h>
#include <World.h>

TradeItemPreviewService::TradeItemPreviewService(
    World& aWorld,
    entt::dispatcher&) noexcept
    : m_world(aWorld)
    , m_hostBinding(ItemPreviewHostBridge::Get(), *this)
{
    if (!m_hostBinding.IsBound())
    {
        spdlog::error(
            "Trade preview host bridge bind failed hostBridgeProbe=true reason=clientAlreadyBound");
    }

    (void)TradePreviewHostMenu::Register();
}

TradeItemPreviewService::~TradeItemPreviewService() noexcept
{
    Clear();
}

void TradeItemPreviewService::SelectItem(Trade::ItemId aItemId) noexcept
{
    const GameId serverId{
        static_cast<std::uint32_t>(aItemId >> 32),
        static_cast<std::uint32_t>(aItemId)};
    const std::uint32_t gameId = m_world.GetModSystem().GetGameId(serverId);
    TESForm* const pForm = gameId ? TESForm::GetById(gameId) : nullptr;
    TESBoundObject* const pObject = pForm ? Cast<TESBoundObject>(pForm) : nullptr;

    if (!pObject)
    {
        spdlog::warn(
            "Trade preview rejected item {:016X}: game form {:08X}",
            aItemId,
            gameId);
        return;
    }

    InventoryEntry entry{};
    entry.pObject = pObject;
    entry.pExtraLists = nullptr;
    entry.count = 1;
    m_controller.SetItem(entry, aItemId);
    ExecuteHostCommand(m_hostSession.RequestOpen());

    spdlog::info(
        "Trade preview selected item {:016X} (game form {:08X})",
        aItemId,
        gameId);
}

void TradeItemPreviewService::Clear() noexcept
{
    m_controller.ClearItem();
    ExecuteHostCommand(m_hostSession.RequestClose());
}

void TradeItemPreviewService::SetPreviewRegion(
    float aLeft,
    float aTop,
    float aWidth,
    float aHeight) noexcept
{
    m_controller.SetPreviewRegion(aLeft, aTop, aWidth, aHeight);
}

void TradeItemPreviewService::UpdatePreviewPlacement() noexcept
{
    m_controller.UpdatePreviewPlacement();
}

ItemPreviewRasterCaptureRequest
TradeItemPreviewService::CaptureProjectionTelemetryState() noexcept
{
    return CaptureRasterRequest();
}

ItemPreviewRasterCaptureRequest
TradeItemPreviewService::CaptureRasterRequest() noexcept
{
    return m_controller.CaptureRasterRequest();
}

void TradeItemPreviewService::SubmitProjectionMeasurement(
    const ItemPreviewRasterMeasurement& aMeasurement) noexcept
{
    SubmitRasterMeasurement(aMeasurement);
}

void TradeItemPreviewService::SubmitRasterMeasurement(
    const ItemPreviewRasterMeasurement& aMeasurement) noexcept
{
    m_controller.SubmitRasterMeasurement(aMeasurement);
}

void TradeItemPreviewService::ProcessPendingFitReloadOnUiThread() noexcept
{
    ProcessPendingReloadOnUiThread();
}

void TradeItemPreviewService::ProcessPendingReloadOnUiThread() noexcept
{
    m_controller.ProcessPendingReloadOnUiThread();
}

bool TradeItemPreviewService::BeginNativePreviewSession(
    std::uint32_t aLightScheme) noexcept
{
    return BeginNativeSession(aLightScheme);
}

bool TradeItemPreviewService::BeginNativeSession(
    std::uint32_t aLightScheme) noexcept
{
    return m_controller.BeginSession(aLightScheme);
}

void TradeItemPreviewService::EndNativePreviewSession() noexcept
{
    EndNativeSession();
}

void TradeItemPreviewService::EndNativeSession() noexcept
{
    m_controller.EndSession();
}

void TradeItemPreviewService::OnHostMenuShown(bool aApplySelection) noexcept
{
    OnHostShown(aApplySelection);
}

void TradeItemPreviewService::OnHostShown(bool aApplySelection) noexcept
{
    const auto result = m_hostSession.OnShown();
    ExecuteHostCommand(result.command);

    if (result.applySelection)
        m_controller.OnHostShown(aApplySelection);
}

void TradeItemPreviewService::OnHostMenuHidden() noexcept
{
    OnHostHidden();
}

void TradeItemPreviewService::OnHostHidden() noexcept
{
    m_controller.OnHostHidden();
    ExecuteHostCommand(m_hostSession.OnHidden());
}

void TradeItemPreviewService::ExecuteHostCommand(
    ItemPreviewHostSession::Command aCommand) noexcept
{
    switch (aCommand)
    {
    case ItemPreviewHostSession::Command::Show:
        TradePreviewHostMenu::Show();
        break;
    case ItemPreviewHostSession::Command::Hide:
        TradePreviewHostMenu::Hide();
        break;
    case ItemPreviewHostSession::Command::None:
    default:
        break;
    }
}
