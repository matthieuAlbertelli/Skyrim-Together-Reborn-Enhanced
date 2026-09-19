#include <TiltedCore/Stl.hpp>
#include <TiltedCore/Buffer.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <Animation/BindingActionReplay.h>
#include <Animation/AnimationEventLists.h>
#include <catch2/catch.hpp>

namespace
{
ActionEvent Action(const char* name, uint64_t tick = 1)
{
    ActionEvent a;
    a.EventName = TiltedPhoques::String{name};
    a.TargetEventName = TiltedPhoques::String{name};
    a.Tick = tick;
    a.ActionId = 100;
    a.IdleId = 200; // Deliberately unrelated to any runtime chair FormID.
    a.State1 = 11;
    a.State2 = 12;
    a.TargetId = 13;
    a.Type = 2;
    return a;
}
struct Observer
{
    BindingActionReplay History;
    TiltedPhoques::List<ActionEvent> Pending;
    uint32_t ReplayCount{};
    bool Reset{};
    AnimationBinding Old{1000, 10000}, New{2000, 20000};
    AnimationBindingChange Change{1, 1, 3, 19, Old, New};
    Observer() { History.ObserveBinding(Old, Pending, ReplayCount, Reset); }
    void Consume()
    {
        REQUIRE_FALSE(Pending.empty());
        History.Consumed(Pending.front());
        if (ReplayCount) --ReplayCount;
        Pending.pop_front();
    }
    auto Rebind() { return History.Rebind(Change, Pending, ReplayCount, Reset); }
};
}

TEST_CASE("Consumed chair entry is reconstructed before pending actions on the new binding", "[animation-replay]")
{
    Observer o;
    const auto enter = Action("IdleChairRightEnter", 10);
    o.Pending.push_back(enter);
    o.Consume();
    o.Pending.push_back(Action("moveStart", 20));
    o.Pending.push_back(Action("TurnRight", 20)); // Equal ticks retain source order.
    const auto result = o.Rebind();
    REQUIRE(result.Accepted);
    REQUIRE(result.SourceCount == 1);
    REQUIRE(result.Chain.Actions.size() == 1);
    REQUIRE(o.Pending.size() == 3);
    REQUIRE(o.Pending.front().EventName == "IdleChairEnterInstant");
    REQUIRE(o.Pending.front().TargetEventName == "IdleChairEnterInstant");
    REQUIRE(o.Pending.front().ActionId == 0);
    REQUIRE(o.Pending.front().IdleId == 0);
    REQUIRE(o.Pending.front().Tick == enter.Tick);
    REQUIRE(o.Pending.front().State1 == enter.State1);
    REQUIRE(o.Pending.front().State2 == enter.State2);
    REQUIRE(o.Pending.front().TargetId == enter.TargetId);
    REQUIRE(o.History.Change().New == o.New);
    o.History.ObserveBinding(o.New, o.Pending, o.ReplayCount, o.Reset);
    o.Consume();
    REQUIRE(o.Pending.front().EventName == "moveStart");
    REQUIRE(o.History.Cache().GetActions().size() == 1); // Injected history not duplicated.
    o.Consume();
    REQUIRE(o.Pending.front().EventName == "TurnRight");
}

TEST_CASE("Observer refinement uses the exact STR server instant and exit rules", "[animation-replay]")
{
    for (const auto& [event, instant] : AnimationEventLists::kIdleToInstant)
    {
        if (instant.empty()) continue;
        Observer o;
        const auto name = std::string(event);
        const auto enter = Action(name.c_str());
        o.History.Consumed(enter);
        ActionReplayCache server;
        server.AppendAll({enter});
        const auto expected = server.FormRefinedReplayChain();
        const auto result = o.Rebind();
        REQUIRE(result.Accepted);
        REQUIRE(result.Chain == expected);
        REQUIRE(result.Chain.Actions.size() == 1);
        REQUIRE(result.Chain.Actions.front().EventName == instant);
    }
}

TEST_CASE("Pending chair entry is never duplicated by reconstruction", "[animation-replay]")
{
    Observer o;
    const auto pending = Action("IdleChairLeftEnter");
    o.Pending.push_back(pending);
    REQUIRE(o.Rebind().Accepted);
    REQUIRE(o.Pending.size() == 1);
    REQUIRE(o.Pending.front() == pending);
    REQUIRE(o.ReplayCount == 0);
    o.Consume();
    REQUIRE(o.History.Cache().GetActions().size() == 1);
}

TEST_CASE("Consumed or pending furniture exit prevents resurrecting a previous entry", "[animation-replay]")
{
    for (const char* exit : {"IdleChairExitToStand", "IdleChairRightExit", "ForceFurnExit"})
    {
        Observer o;
        o.History.Consumed(Action("IdleChairRightEnter", 1));
        SECTION("Consumed exit uses the shared refined chain")
        {
            o.History.Consumed(Action(exit, 2));
            const auto result = o.Rebind();
            REQUIRE(result.Accepted);
            REQUIRE(result.Chain.ResetAnimationGraph);
            REQUIRE(result.Chain.Actions.empty());
            REQUIRE(o.Pending.empty());
        }
        SECTION("Exit already received but still pending invalidates history")
        {
            o.Pending.push_back(Action("moveStop", 2));
            o.Pending.push_back(Action(exit, 3));
            o.Pending.push_back(Action("IdleChairLeftEnter", 4));
            const auto before = o.Pending;
            const auto result = o.Rebind();
            REQUIRE(result.PendingExit);
            REQUIRE(result.Chain.Actions.empty());
            REQUIRE(o.Pending == before);
            REQUIRE(o.History.Cache().GetActions().empty());
        }
    }
}

TEST_CASE("New actions follow the replay prefix and existing pending order", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairFrontEnter", 1));
    o.Pending.push_back(Action("moveStart", 2));
    REQUIRE(o.Rebind().Accepted);
    o.Pending.push_back(Action("turnStop", 3));
    o.Pending.push_back(Action("IdleChairExitToStand", 4));
    for (const char* event : {"IdleChairEnterInstant", "moveStart", "turnStop", "IdleChairExitToStand"})
    {
        REQUIRE(o.Pending.front().EventName == event);
        o.Consume();
    }
    auto cache = o.History.Cache();
    REQUIRE(cache.FormRefinedReplayChain().Actions.empty());
}

TEST_CASE("Repeated notification or update does not inject another replay", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairRightEnter"));
    REQUIRE(o.Rebind().Accepted);
    const auto before = o.Pending;
    REQUIRE_FALSE(o.Rebind().Accepted);
    o.History.ObserveBinding(o.New, o.Pending, o.ReplayCount, o.Reset);
    o.History.ObserveBinding(o.New, o.Pending, o.ReplayCount, o.Reset);
    REQUIRE(o.Pending == before);
    o.Consume();
    REQUIRE_FALSE(o.Rebind().Accepted);
    REQUIRE(o.Pending.empty());
    REQUIRE(o.ReplayCount == 0);
}

TEST_CASE("Exit arriving during graph wait cancels synthetic entry but preserves live actions", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairRightEnter"));
    o.Pending.push_back(Action("moveStart", 2));
    REQUIRE(o.Rebind().Accepted);
    REQUIRE(o.History.LogWaitOnce());
    REQUIRE_FALSE(o.History.LogWaitOnce());
    o.Pending.push_back(Action("IdleChairExitToStand", 3));
    REQUIRE(o.History.CancelForPendingExit(o.Pending, o.ReplayCount, o.Reset));
    REQUIRE(o.Pending.size() == 2);
    REQUIRE(o.Pending.front().EventName == "moveStart");
    REQUIRE(o.Pending.back().EventName == "IdleChairExitToStand");
    REQUIRE(o.History.Remaining() == 0);
    REQUIRE(o.ReplayCount == 0);
    REQUIRE_FALSE(o.Rebind().Accepted);
}

TEST_CASE("Binding without replayable history preserves the live queue", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("TurnRight"));
    o.History.Consumed(Action(""));
    o.Pending.push_back(Action("moveStart", 2));
    const auto before = o.Pending;
    const auto result = o.Rebind();
    REQUIRE(result.Accepted);
    REQUIRE(result.Chain.Actions.empty());
    REQUIRE(o.Pending == before);
    REQUIRE_FALSE(o.Reset);
}

TEST_CASE("Replay cache and reconstruction remain bounded under long action streams", "[animation-replay]")
{
    Observer o;
    for (uint64_t i = 0; i < 1000; ++i)
        o.History.Consumed(Action("IdleChairRightEnter", i));
    REQUIRE(o.History.Cache().GetActions().size() == ActionReplayCache::kReplayCacheMaxSize);
    const auto result = o.Rebind();
    REQUIRE(result.Chain.Actions.size() == ActionReplayCache::kReplayCacheMaxSize);
    REQUIRE(o.Pending.front().Tick == 968);
    REQUIRE(o.Pending.back().Tick == 999);
}

TEST_CASE("Context invalidation removes only injected history and rejects stale reconstruction", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairRightEnter"));
    o.Pending.push_back(Action("moveStart", 2));
    REQUIRE(o.Rebind().Accepted);
    o.History.Invalidate(o.Pending, o.ReplayCount, o.Reset);
    REQUIRE(o.Pending.size() == 1);
    REQUIRE(o.Pending.front().EventName == "moveStart");
    REQUIRE(o.ReplayCount == 0);
    REQUIRE(o.History.Cache().GetActions().empty());
    REQUIRE_FALSE(o.Rebind().Accepted);
    o.Consume(); // Normal STR actions cannot repopulate invalidated session history.
    REQUIRE(o.History.Cache().GetActions().empty());
    Observer fresh;
    fresh.Change.Session = 2;
    REQUIRE(fresh.Rebind().Accepted);
    REQUIRE(fresh.Pending.empty());
}

TEST_CASE("Unexpected actor token invalidates replay rather than targeting an alias", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairRightEnter"));
    REQUIRE(o.Rebind().Accepted);
    o.History.ObserveBinding({o.New.FormId, o.New.Token + 1}, o.Pending, o.ReplayCount, o.Reset);
    REQUIRE(o.Pending.empty());
    REQUIRE_FALSE(o.History.Enabled());
    REQUIRE_FALSE(o.Rebind().Accepted);
}

TEST_CASE("Subsequent generation rebuilds once without duplicating unconsumed injected history", "[animation-replay]")
{
    Observer o;
    o.History.Consumed(Action("IdleChairRightEnter"));
    o.Pending.push_back(Action("moveStart", 2));
    REQUIRE(o.Rebind().Accepted);
    const auto stale = o.Change;
    o.Change.Old = o.New;
    o.Change.New = {3000, 30000};
    o.Change.Generation = 2;
    REQUIRE(o.Rebind().Accepted);
    REQUIRE(o.Pending.size() == 2);
    REQUIRE(o.ReplayCount == 1);
    REQUIRE(o.Pending.front().EventName == "IdleChairEnterInstant");
    REQUIRE_FALSE(o.History.Rebind(stale, o.Pending, o.ReplayCount, o.Reset).Accepted);
}
