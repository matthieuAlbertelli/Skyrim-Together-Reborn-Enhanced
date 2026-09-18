#include <TiltedOnlinePCH.h>
#include <SexChangeProbe.h>
#include <CharacterCreation/SexProbeWindow.h>
#include <Actor.h>
#include <PlayerCharacter.h>
#include <Forms/TESNPC.h>
#include <Forms/TESRace.h>
#include <Forms/BGSHeadPart.h>
#include <Forms/BGSColorForm.h>
#include <Systems/FaceGenSystem.h>
#include <Interface/UI.h>
#include <VersionDb.h>
#include <array>
#include <mutex>
#include <bit>

namespace
{
using STRE::CharacterCreation::SexProbeWindow;
struct Sample
{
    uint32_t Sex{};
    uintptr_t Root{}, Face{}, Head{};
    std::string Text;
};
struct Watch
{
    uint32_t Id{};
    uintptr_t Token{};
    uint64_t Revision{};
    SexProbeWindow Window;
    Sample Previous;
};
std::mutex Mutex;
std::array<Watch, 16> Watches{};
uint64_t Tick{};
size_t Next{};
uint64_t Hash(std::string_view text)
{
    uint64_t hash = 14695981039346656037ull;
    for (const unsigned char c : text)
        hash = (hash ^ c) * 1099511628211ull;
    return hash;
}
std::string Model(const TESRace* race, size_t sex)
{
    const char* path = race ? race->skeletonModels[sex].name.AsAscii() : nullptr;
    const size_t size = path ? strnlen(path, 513) : 0;
    if (!path || size > 512)
        return "unavailable";
    std::string value(path, size);
    for (auto& c : value)
    {
        if (c == '/')
            c = '\\';
        else if (c >= 'A' && c <= 'Z')
            c += ('a' - 'A');
    }
    return value;
}
Sample Read(Actor* actor, bool detailed)
{
    Sample s;
    auto* base = Cast<TESNPC>(actor->baseForm);
    if (!base)
    {
        s.Text = "baseUnavailable=true";
        return s;
    }
    s.Sex = base->actorData.IsFemale();
    s.Root = reinterpret_cast<uintptr_t>(actor->GetNiNode());
    s.Face = reinterpret_cast<uintptr_t>(actor->GetFaceGenNiNode());
    s.Head = reinterpret_cast<uintptr_t>(FaceGenSystem::GetHeadGeometry(actor));
    s.Text = fmt::format(
        "actor={:X} actorToken={:X} base={:X} baseToken={:X} isPlayerRef={} sex={} runtimeRace={:X} baseRace={:X} overlayRace={:X} weight={} thirdPerson3D={:X} faceNode={:X} "
        "head={:X} actorStateFlags1={:X} actorStateFlags2={:X}",
        actor->formID, reinterpret_cast<uintptr_t>(actor), base->formID, reinterpret_cast<uintptr_t>(base), actor == PlayerCharacter::Get(), s.Sex,
        actor->race ? actor->race->formID : 0, base->raceForm.race ? base->raceForm.race->formID : 0, base->overlayRace ? base->overlayRace->formID : 0, base->weight, s.Root,
        s.Face, s.Head, actor->actorState.flags1, actor->actorState.flags2);
    if (!detailed)
        return s;
    String bytes;
    const auto flags = base->GetChangeFlags();
    base->Serialize(&bytes);
    uint64_t parts = 14695981039346656037ull;
    if (base->headparts && base->headpartsCount <= 255)
        for (uint32_t i = 0; i < base->headpartsCount; ++i)
            parts = (parts ^ (base->headparts[i] ? base->headparts[i]->formID : 0)) * 1099511628211ull;
    const auto* hair = base->headData ? base->headData->hairColor : nullptr;
    const auto inventory = actor->GetActorInventory();
    int64_t count{};
    size_t worn{};
    for (const auto& e : inventory.Entries)
    {
        count += e.Count;
        worn += e.IsWorn() ? 1 : 0;
    }
    std::string tintBytes;
    size_t tintCount{};
    bool tintComplete = false;
    if (actor == PlayerCharacter::Get())
    {
        const auto& tints = PlayerCharacter::Get()->GetTints();
        tintCount = tints.length;
        tintComplete = tintCount <= 255 && tintCount <= tints.capacity && (!tintCount || tints.data);
        if (tintComplete)
            for (uint32_t i = 0; i < tints.length; ++i)
            {
                const auto* tint = tints[i];
                if (!tint)
                {
                    tintComplete = false;
                    break;
                }
                const char* name = tint->texture ? tint->texture->name.AsAscii() : nullptr;
                size_t n = name ? strnlen(name, 1025) : 0;
                if (n > 1024)
                {
                    tintComplete = false;
                    break;
                }
                tintBytes += fmt::format("{}:{}:{}:{}:", tint->type, tint->color, std::bit_cast<uint32_t>(tint->alpha), n);
                if (name)
                    tintBytes.append(name, n);
            }
    }
    const auto male = Model(base->raceForm.race, 0), female = Model(base->raceForm.race, 1);
    s.Text += fmt::format(
        " headpartCount={} headpartHash={:X} hairColorForm={:X} hairColorABGR={:X} bodyColor={},{},{} inventoryEntries={} inventoryCount={} equippedEntries={} appearanceBytes={} "
        "appearanceHash={:X} changeFlags={:X} tintCount={} tintHash={:X} tintsComplete={} descriptorRaceLocal={:X} descriptorSex={} descriptorWeight={} maleModel={} "
        "femaleModel={} sameModels={}",
        base->headpartsCount, parts, hair ? hair->formID : 0, hair ? hair->abgr : 0, base->color.red, base->color.green, base->color.blue, inventory.Entries.size(), count, worn,
        bytes.size(), Hash({bytes.data(), bytes.size()}), flags, tintCount, Hash(tintBytes), tintComplete, base->raceForm.race ? base->raceForm.race->formID : 0, s.Sex,
        base->weight, male, female, male != "unavailable" && male == female);
    return s;
}
Watch& Find(Actor* actor)
{
    const auto token = reinterpret_cast<uintptr_t>(actor);
    for (auto& w : Watches)
        if (w.Id == actor->formID && w.Token == token)
            return w;
    auto& w = Watches[Next++ % Watches.size()];
    w = {};
    w.Id = actor->formID;
    w.Token = token;
    return w;
}
} // namespace
SexChangeProbeTiming ReadSexChangeProbeTiming(Actor* actor) noexcept
{
    try
    {
        std::lock_guard lock(Mutex);
        SexChangeProbeTiming result{Tick, 0, false};
        if (actor)
            for (const auto& w : Watches)
                if (w.Id == actor->formID && w.Token == reinterpret_cast<uintptr_t>(actor))
                {
                    result.SexTick = w.Window.SexTick;
                    result.Known = w.Window.SexObserved;
                    break;
                }
        return result;
    }
    catch (...) { return {}; }
}
void SignalSexChangeProbe(Actor* actor, const char* phase) noexcept
{
    if (!actor || !STRE::CharacterCreation::SexProbeRuntime(VersionDb::Get().GetLoadedVersionString()))
        return;
    const auto error = GetLastError();
    try
    {
        const auto sample = Read(actor, false);
        uint64_t tick{}, delta{}, sexDelta{};
        bool associated{}, sexObserved{};
        SexProbeWindow::Change change{};
        {
            std::lock_guard lock(Mutex);
            auto& w = Find(actor);
            tick = Tick;
            ++w.Revision;
            change = w.Window.Observe(tick, sample.Sex, Hash(sample.Text));
            w.Previous = sample;
            associated = w.Window.Active(tick);
            delta = associated ? tick - w.Window.Start : 0;
            sexObserved = w.Window.SexObserved;
            sexDelta = sexObserved ? tick - w.Window.SexTick : 0;
            if (std::string_view(phase) != "completion-event")
                w.Window.Arm(tick);
        }
        if (change.SexChanged)
            spdlog::info("[STRE][SexChangeProbe] phase=sex-flag-change source=native-observation tick={} from={} to={} {}", tick, change.From, sample.Sex, sample.Text);
        spdlog::info(
            "[STRE][SexChangeProbe] phase={} tick={} tickDelta={} associated={} sexObserved={} sexTickDelta={} thread={} {}", phase, tick, delta, associated, sexObserved, sexDelta,
            GetCurrentThreadId(), sample.Text);
    }
    catch (...)
    {
    }
    SetLastError(error);
}
void TickSexChangeProbe() noexcept
{
    if (!STRE::CharacterCreation::SexProbeRuntime(VersionDb::Get().GetLoadedVersionString()))
        return;
    try
    {
        static BSFixedString menu("RaceSex Menu");
        auto* ui = UI::Get();
        auto* player = PlayerCharacter::Get();
        const bool open = ui && ui->GetMenuOpen(menu);
        std::array<Watch, 16> work;
        uint64_t tick;
        {
            std::lock_guard lock(Mutex);
            tick = ++Tick;
            for (auto& w : Watches)
                if (w.Id && !w.Window.Active(tick) && !(open && player && w.Token == reinterpret_cast<uintptr_t>(player)))
                    w = {};
            if (open && player)
                Find(player);
            work = Watches;
        }
        for (auto w : work)
        {
            if (!w.Id)
                continue;
            auto* actor = Cast<Actor>(TESForm::GetById(w.Id));
            if (!actor || reinterpret_cast<uintptr_t>(actor) != w.Token)
                continue;
            if (!(open && actor == player) && !w.Window.Active(tick))
                continue;
            const auto now = Read(actor, true);
            const auto change = w.Window.Observe(tick, now.Sex, Hash(now.Text));
            if (change.Significant)
            {
                spdlog::info(
                    "[STRE][SexChangeProbe] phase={} tick={} tickDelta={} from={} to={} {}",
                    change.SexChanged ? "sex-flag-change"
                    : change.Baseline ? "baseline"
                                      : "state-change",
                    tick, w.Window.Active(tick) ? tick - w.Window.Start : 0, change.From, now.Sex, now.Text);
                if (!change.Baseline && (now.Root != w.Previous.Root || now.Face != w.Previous.Face || now.Head != w.Previous.Head))
                    spdlog::info(
                        "[STRE][SexChangeProbe] phase=geometry-transition tick={} tickDelta={} actor={:X} rootOld={:X} rootNew={:X} faceOld={:X} faceNew={:X} headOld={:X} "
                        "headNew={:X}",
                        tick, w.Window.Active(tick) ? tick - w.Window.Start : 0, w.Id, w.Previous.Root, now.Root, w.Previous.Face, now.Face, w.Previous.Head, now.Head);
            }
            w.Previous = now;
            {
                std::lock_guard lock(Mutex);
                for (auto& current : Watches)
                    if (current.Id == w.Id && current.Token == w.Token)
                    {
                        // A concurrent hook/event wins over a stale service sample.
                        if (current.Revision == w.Revision)
                            current = std::move(w);
                        break;
                    }
            }
        }
    }
    catch (...)
    {
    }
}
