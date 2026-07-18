#include <TiltedOnlinePCH.h>

#include <Services/ItemPreview/ItemPreviewNativeSession.h>

Inventory3DManager* ItemPreviewNativeSession::GetManager() noexcept
{
    return Inventory3DManager::GetSingleton();
}

bool ItemPreviewNativeSession::Begin(std::uint32_t aLightScheme) noexcept
{
    Inventory3DManager* const pManager = GetManager();
    if (!pManager)
        return false;

    m_lightScheme = aLightScheme != 0 ? aLightScheme : 1;
    pManager->Begin3D(m_lightScheme);
    m_open = true;
    return true;
}

void ItemPreviewNativeSession::End() noexcept
{
    if (!m_open)
        return;

    if (Inventory3DManager* const pManager = GetManager())
        pManager->End3D();

    m_open = false;
}

void* ItemPreviewNativeSession::GetManagerAddress() const noexcept
{
    return static_cast<void*>(GetManager());
}

Inventory3DManagerDebugState
ItemPreviewNativeSession::CaptureDebugState() const noexcept
{
    if (Inventory3DManager* const pManager = GetManager())
        return pManager->CaptureDebugState();

    return {};
}

Inventory3DManagerPreviewModelBounds
ItemPreviewNativeSession::CaptureModelBounds(
    std::uintptr_t aExpectedModel) const noexcept
{
    if (Inventory3DManager* const pManager = GetManager())
        return pManager->CaptureModelBounds(aExpectedModel);

    return {};
}

bool ItemPreviewNativeSession::Clear() noexcept
{
    Inventory3DManager* const pManager = GetManager();
    if (!pManager || !m_open)
        return false;

    pManager->Clear3D();
    return true;
}

bool ItemPreviewNativeSession::Load(
    InventoryEntry& aEntry,
    const ItemPreviewTransform* apTransform) noexcept
{
    Inventory3DManager* const pManager = GetManager();
    if (!pManager || !m_open || !aEntry.pObject)
        return false;

    if (apTransform)
    {
        pManager->SetPreviewTransform(
            apTransform->x,
            apTransform->y,
            apTransform->z,
            apTransform->scale);
    }

    pManager->UpdateItem3D(&aEntry);
    return true;
}

bool ItemPreviewNativeSession::RestartAndLoad(
    InventoryEntry& aEntry,
    const ItemPreviewTransform& aTransform,
    std::uint32_t aLightScheme) noexcept
{
    if (!aEntry.pObject)
        return false;

    const std::uint32_t lightScheme =
        aLightScheme != 0 ? aLightScheme : m_lightScheme;

    End();
    if (!Begin(lightScheme))
        return false;

    return Load(aEntry, &aTransform);
}
