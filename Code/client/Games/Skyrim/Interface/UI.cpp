#include <Games/Skyrim/Interface/IMenu.h>
#include <Games/Skyrim/Interface/UI.h>

#include <Misc/BSFixedString.h>
#include <TiltedOnlinePCH.h>
#include "immersive_launcher/stubs/DllBlocklist.h"

#include <World.h>

static bool g_RequestUnpauseAll{false};

namespace
{
using DeclaredMenuMap = decltype(UI::menuMap);

constexpr std::uint32_t kInvalidMenuIndex = 0xFFFFFFFFu;
constexpr std::uint32_t kCrc32Polynomial = 0xEDB88320u;

std::uint32_t GenerateCrc32(
    const void* apData,
    std::size_t aSize,
    std::uint32_t aInitialHash,
    std::uint32_t aFinalXor) noexcept
{
    std::uint32_t hash = aInitialHash;
    if (!apData || aSize == 0)
        return hash ^ aFinalXor;

    const auto* pByte =
        static_cast<const unsigned char*>(apData);
    for (std::size_t index = 0; index < aSize; ++index)
    {
        hash ^= pByte[index];
        for (std::uint32_t bit = 0; bit < 8; ++bit)
        {
            const std::uint32_t lowBitMask =
                0u - (hash & 1u);
            hash =
                (hash >> 1u) ^
                (kCrc32Polynomial & lowBitMask);
        }
    }

    return hash ^ aFinalXor;
}

std::uint32_t GenerateRawCrc32(
    const void* apData,
    std::size_t aSize) noexcept
{
    return GenerateCrc32(apData, aSize, 0u, 0u);
}

std::uint32_t GenerateStandardCrc32(
    const void* apData,
    std::size_t aSize) noexcept
{
    return GenerateCrc32(
        apData,
        aSize,
        0xFFFFFFFFu,
        0xFFFFFFFFu);
}

std::size_t GetMenuNameLength(const char* apName) noexcept
{
    if (!apName)
        return 0;

    std::size_t length = 0;
    while (apName[length] != '\0')
        ++length;

    return length;
}

std::uint32_t GenerateMenuNameRawCrc(
    const char* apName) noexcept
{
    return GenerateRawCrc32(
        apName,
        GetMenuNameLength(apName));
}

std::uint32_t GenerateMenuNameStandardCrc(
    const char* apName) noexcept
{
    return GenerateStandardCrc32(
        apName,
        GetMenuNameLength(apName));
}

std::uint32_t GeneratePointer32RawCrc(
    const void* apPointer) noexcept
{
    const std::uint32_t pointerValue =
        static_cast<std::uint32_t>(
            reinterpret_cast<std::uintptr_t>(apPointer));
    return GenerateRawCrc32(
        &pointerValue,
        sizeof(pointerValue));
}

std::uint32_t GeneratePointer64RawCrc(
    const void* apPointer) noexcept
{
    const std::uintptr_t pointerValue =
        reinterpret_cast<std::uintptr_t>(apPointer);
    return GenerateRawCrc32(
        &pointerValue,
        sizeof(pointerValue));
}

std::uint32_t GeneratePointer64StandardCrc(
    const void* apPointer) noexcept
{
    const std::uintptr_t pointerValue =
        reinterpret_cast<std::uintptr_t>(apPointer);
    return GenerateStandardCrc32(
        &pointerValue,
        sizeof(pointerValue));
}

template <class Key> struct NativeMenuHashPolicy;

template <> struct NativeMenuHashPolicy<BSFixedString>
{
    using hash_type = std::uint32_t;

    static hash_type get_hash(
        const BSFixedString& acKey) noexcept
    {
        return GeneratePointer64RawCrc(acKey.data);
    }

    static bool is_key_equal(
        const BSFixedString& acLeft,
        const BSFixedString& acRight) noexcept
    {
        return acLeft.data == acRight.data;
    }
};

template <class Key, class Value>
struct NativeMenuScatterTable
    : creation::BSTScatterTable<
          Key,
          Value,
          creation::BSTScatterTableDefaultKVStorage,
          NativeMenuHashPolicy,
          creation::BSTScatterTableHeapAllocator,
          8>
{
};

using NativeMenuMap = creation::BSTHashMap<
    BSFixedString,
    UI::UIMenuEntry,
    NativeMenuScatterTable>;
using MenuMap = NativeMenuMap;
using MenuMapEntry = typename MenuMap::entry_type;

static_assert(sizeof(NativeMenuMap) == sizeof(DeclaredMenuMap));

NativeMenuMap& GetNativeMenuMap(UI& aUI) noexcept
{
    return reinterpret_cast<NativeMenuMap&>(aUI.menuMap);
}

const NativeMenuMap& GetNativeMenuMap(const UI& acUI) noexcept
{
    return reinterpret_cast<const NativeMenuMap&>(acUI.menuMap);
}

bool IsValidMenuTable(const MenuMap& acMenuMap) noexcept
{
    const std::uint32_t capacity = acMenuMap.max_size();
    return acMenuMap.m_entries &&
        capacity != 0 &&
        (capacity & (capacity - 1u)) == 0;
}

std::uint32_t FindMenuLinearIndex(
    const MenuMap& acMenuMap,
    const BSFixedString& acName) noexcept
{
    if (!IsValidMenuTable(acMenuMap) || !acName.data)
        return kInvalidMenuIndex;

    for (std::uint32_t index = 0;
         index < acMenuMap.max_size();
         ++index)
    {
        const MenuMapEntry& entry = acMenuMap.m_entries[index];
        if (!entry.empty() && entry.key.data == acName.data)
            return index;
    }

    return kInvalidMenuIndex;
}

bool IsMenuReachableFromHash(
    const MenuMap& acMenuMap,
    const BSFixedString& acName,
    std::uint32_t aHash) noexcept
{
    if (!IsValidMenuTable(acMenuMap) || !acName.data)
        return false;

    MenuMapEntry* const pEntries = acMenuMap.m_entries;
    const std::uint32_t capacity = acMenuMap.max_size();
    MenuMapEntry* pEntry =
        pEntries + (aHash & (capacity - 1u));

    const std::uintptr_t beginAddress =
        reinterpret_cast<std::uintptr_t>(pEntries);
    const std::uintptr_t endAddress =
        beginAddress + sizeof(MenuMapEntry) * capacity;

    for (std::uint32_t hop = 0; hop < capacity; ++hop)
    {
        if (pEntry->empty())
            return false;

        if (pEntry->key.data == acName.data)
            return true;

        MenuMapEntry* const pNext = pEntry->next;
        if (pNext == acMenuMap.m_eolPtr)
            return false;

        const std::uintptr_t nextAddress =
            reinterpret_cast<std::uintptr_t>(pNext);
        if (nextAddress < beginAddress ||
            nextAddress >= endAddress ||
            ((nextAddress - beginAddress) % sizeof(MenuMapEntry)) != 0)
        {
            return false;
        }

        pEntry = pNext;
    }

    return false;
}

void LogMenuHashProbe(
    const UI& acUI,
    const BSFixedString& acName,
    const char* apPhase) noexcept
{
    const MenuMap& menuMap = GetNativeMenuMap(acUI);
    const std::uint32_t capacity = menuMap.max_size();
    const bool validTable = IsValidMenuTable(menuMap);
    const std::uint32_t policyHash =
        menuMap.hash_function()(acName);
    const std::uint32_t contentRawHash =
        GenerateMenuNameRawCrc(acName.data);
    const std::uint32_t contentStandardHash =
        GenerateMenuNameStandardCrc(acName.data);
    const std::uint32_t pointer32RawHash =
        GeneratePointer32RawCrc(acName.data);
    const std::uint32_t pointer64RawHash =
        GeneratePointer64RawCrc(acName.data);
    const std::uint32_t pointer64StandardHash =
        GeneratePointer64StandardCrc(acName.data);
    const auto GetBucket = [validTable, capacity](
                               std::uint32_t aHash) noexcept {
        return validTable
            ? aHash & (capacity - 1u)
            : kInvalidMenuIndex;
    };
    const std::uint32_t linearIndex =
        FindMenuLinearIndex(menuMap, acName);

    spdlog::info(
        "UI menu hash probe phase={} name={} stringPtr={} validTable={} linearPresent={} linearIndex={} policyHash={:08X} policyBucket={} contentRawHash={:08X} contentRawBucket={} contentStandardHash={:08X} contentStandardBucket={} pointer32RawHash={:08X} pointer32RawBucket={} pointer64RawHash={:08X} pointer64RawBucket={} pointer64StandardHash={:08X} pointer64StandardBucket={} reachablePolicy={} reachableContentRaw={} reachableContentStandard={} reachablePointer32Raw={} reachablePointer64Raw={} reachablePointer64Standard={} size={} free={} capacity={}",
        apPhase,
        acName.AsAscii(),
        static_cast<const void*>(acName.data),
        validTable,
        linearIndex != kInvalidMenuIndex,
        linearIndex,
        policyHash,
        GetBucket(policyHash),
        contentRawHash,
        GetBucket(contentRawHash),
        contentStandardHash,
        GetBucket(contentStandardHash),
        pointer32RawHash,
        GetBucket(pointer32RawHash),
        pointer64RawHash,
        GetBucket(pointer64RawHash),
        pointer64StandardHash,
        GetBucket(pointer64StandardHash),
        IsMenuReachableFromHash(menuMap, acName, policyHash),
        IsMenuReachableFromHash(menuMap, acName, contentRawHash),
        IsMenuReachableFromHash(menuMap, acName, contentStandardHash),
        IsMenuReachableFromHash(menuMap, acName, pointer32RawHash),
        IsMenuReachableFromHash(menuMap, acName, pointer64RawHash),
        IsMenuReachableFromHash(
            menuMap,
            acName,
            pointer64StandardHash),
        menuMap.size(),
        menuMap.free_count(),
        capacity);
}
} // namespace

UI* UI::Get()
{
    POINTER_SKYRIMSE(UI*, s_instance, 400327);
    return *s_instance.Get();
}


bool UI::HasMenuRegistration(const BSFixedString& acName) const
{
    if (!acName.data)
        return false;

    const NativeMenuMap& nativeMenuMap =
        GetNativeMenuMap(*this);
    const std::uint32_t nativeHash =
        NativeMenuHashPolicy<BSFixedString>::get_hash(acName);
    return IsMenuReachableFromHash(
        nativeMenuMap,
        acName,
        nativeHash);
}

bool UI::RegisterMenu(const BSFixedString& acName, TCreate* apCreate)
{
    if (!acName.data || !apCreate)
        return false;

    static BSFixedString s_inventoryMenuName{"InventoryMenu"};
    LogMenuHashProbe(
        *this,
        s_inventoryMenuName,
        "vanilla-control-before-insert");
    LogMenuHashProbe(
        *this,
        acName,
        "target-before-insert");

    if (HasMenuRegistration(acName))
    {
        spdlog::info(
            "UI menu registration already present name={}",
            acName.AsAscii());
        LogMenuHashProbe(
            *this,
            acName,
            "target-already-present");
        return true;
    }

    NativeMenuMap& nativeMenuMap = GetNativeMenuMap(*this);
    const bool inserted = nativeMenuMap.insert(
        acName,
        UIMenuEntry{nullptr, apCreate});
    const bool present = HasMenuRegistration(acName);

    spdlog::info(
        "UI menu registration result name={} inserted={} present={} size={} free={} capacity={}",
        acName.AsAscii(),
        inserted,
        present,
        nativeMenuMap.size(),
        nativeMenuMap.free_count(),
        nativeMenuMap.max_size());
    LogMenuHashProbe(
        *this,
        acName,
        "target-after-insert");

    return inserted && present;
}

void UI::QueueMessage(
    const BSFixedString& acName,
    UIMessage::UI_MESSAGE_TYPE aType,
    void* apData)
{
    using TAddMessage = void(void*, const BSFixedString*, std::uint32_t, void*);

    POINTER_SKYRIMSE(void*, queueSingleton, 400445);
    POINTER_SKYRIMSE(TAddMessage, addMessage, 13631);

    void** const ppQueue = queueSingleton.Get();
    auto* const pFunction = addMessage.Get();
    if (!ppQueue || !*ppQueue || !pFunction)
    {
        spdlog::warn(
            "Could not queue UI message {} for {}",
            static_cast<std::uint32_t>(aType),
            acName.AsAscii());
        return;
    }

    spdlog::info(
        "Queueing UI message name={} type={}",
        acName.AsAscii(),
        static_cast<std::uint32_t>(aType));

    pFunction(
        *ppQueue,
        &acName,
        static_cast<std::uint32_t>(aType),
        apData);
}

bool UI::GetMenuOpen(const BSFixedString& acName) const
{
    if (acName.data == nullptr)
        return false;

    TP_THIS_FUNCTION(TMenuSystem_IsOpen, bool, const UI, const BSFixedString&);
    POINTER_SKYRIMSE(TMenuSystem_IsOpen, s_isMenuOpen, 82074);

    return TiltedPhoques::ThisCall(s_isMenuOpen.Get(), this, acName);
}

void UI::CloseAllMenus()
{
    TP_THIS_FUNCTION(TUI_CloseAll, void, const UI);
    POINTER_SKYRIMSE(TUI_CloseAll, s_CloseAll, 82088);

    TiltedPhoques::ThisCall(s_CloseAll.Get(), this);
}

BSFixedString* UI::LookupMenuNameByInstance(IMenu* apMenu)
{
    for (auto& it : menuMap)
    {
        if (it.value.spMenu == apMenu)
            return &it.key;
    }
    return nullptr;
}

IMenu* UI::FindMenuByName(const BSFixedString& acName)
{
    for (const auto& it : menuMap)
    {
        if (it.key == acName)
            return it.value.spMenu;
    }
    return nullptr;
}

void UI::DebugLogAllMenus()
{
    for (auto& e : menuStack)
    {
        spdlog::info("Menu {}", e->uiMenuFlags);
    }
}

static void UnfreezeMenu(IMenu* apEntry)
{
    if (apEntry->PausesGame())
        apEntry->ClearFlag(IMenu::kPausesGame);

    if (apEntry->FreezesBackground())
        apEntry->ClearFlag(IMenu::kFreezeFrameBackground);

    if (apEntry->FreezesFramePause())
        apEntry->ClearFlag(IMenu::kFreezeFramePause);
}

static constexpr const char* kAllowList[] = {
    "TweenMenu",     "MagicMenu",     "StatsMenu",     "InventoryMenu", "MessageBoxMenu",
    "ContainerMenu", "FavoritesMenu", "Tutorial Menu", "Console"
    //"MapMenu", // MapMenu is disabled till we find a proper fix for first person.
    //"Journal Menu", // Journal menu, aka pause menu, is disabled until we find a fix for manual save crashing while unpaused.
};

static void* (*UI_AddToActiveQueue)(UI*, IMenu*, void*);

static void* UI_AddToActiveQueue_Hook(UI* apSelf, IMenu* apMenu, void* apFoundItem /*In reality a reference*/)
{
    // If the menu is empty we let the real function handle it.
    if (!apMenu || !World::Get().GetTransport().IsConnected() || stubs::g_IsSoulsREActive)
        return UI_AddToActiveQueue(apSelf, apMenu, apFoundItem);

    // NOTE(Force): could also compare by RTTI later on...
    for (const char* item : kAllowList)
    {
        if (auto* pMenu = apSelf->FindMenuByName(item))
        {
            if (pMenu == apMenu)
                UnfreezeMenu(apMenu);
        }
    }

    return UI_AddToActiveQueue(apSelf, apMenu, apFoundItem);
}

using TCallback = void(void*, const BSFixedString*, uint32_t, void*);
static TCallback* UIMessageQueue__AddMessage_Real;

// Useful for debugging UI related issues.
void UIMessageQueue__AddMessage(void* a1, const BSFixedString* a2, UIMessage::UI_MESSAGE_TYPE a3, void* a4)
{
    spdlog::info("Adding Message {} with prio {} from {}", a2->AsAscii(), a3, fmt::ptr(_ReturnAddress()));
    UIMessageQueue__AddMessage_Real(a1, a2, a3, a4);
}

static TiltedPhoques::Initializer s_s(
    []()
    {
        // pray that this doesnt fail!
        VersionDbPtr<uint8_t> ProcessHook(82082);
        TiltedPhoques::SwapCall(ProcessHook.Get() + 0x682, UI_AddToActiveQueue, &UI_AddToActiveQueue_Hook);

        // Ignore startup movie
        // TODO: Move me later.
        VersionDbPtr<uint8_t> MainInit(36548);
        TiltedPhoques::Put<uint8_t>(MainInit.Get() + 0xFE, 0xEB);

        // Credits to Skyrim Souls RE for this fix.
        // Allows the favorites menu to be numbered during connect.
        VersionDbPtr<uint8_t> FavoritesCanProcess(51538);
        TiltedPhoques::Put<uint16_t>(FavoritesCanProcess.Get() + 0x15, 0x9090);

        // Some experiments:
        // POINTER_SKYRIMSE(TCallback, s_start, 13631);
        // UIMessageQueue__AddMessage_Real = s_start.Get();
        // TP_HOOK(&UIMessageQueue__AddMessage_Real, UIMessageQueue__AddMessage);

        // This kills the loading spinner
        // TiltedPhoques::Put<uint8_t>(0x1405D51C1, 0xEB);
        // TiltedPhoques::Nop(0x1405D51A2, 5);

        // use 8 threads by default!
        // TiltedPhoques::Put<uint8_t>(0x141E45770, 8);
    });
