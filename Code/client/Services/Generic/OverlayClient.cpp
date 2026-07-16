#include <TiltedOnlinePCH.h>

#include <OverlayRenderHandler.hpp>

#include <Services/OverlayClient.h>
#include <Services/TransportService.h>
#include <Services/TradeMenuService.h>
#include <Services/TradeItemPreviewService.h>
#include <Services/UiSurfaceService.h>

#include <Messages/SendChatMessageRequest.h>
#include <Messages/TeleportRequest.h>

#include <Events/SetTimeCommandEvent.h>

#include <World.h>

#include <charconv>
#include <optional>
#include <string_view>

namespace
{
std::optional<std::uint64_t> ParseUnsigned64(
    const std::string& acValue) noexcept
{
    std::uint64_t result{};
    const char* const pBegin = acValue.data();
    const char* const pEnd = pBegin + acValue.size();

    const auto parseResult =
        std::from_chars(pBegin, pEnd, result);

    if (parseResult.ec != std::errc{} ||
        parseResult.ptr != pEnd)
    {
        return std::nullopt;
    }

    return result;
}
}

OverlayClient::OverlayClient(TransportService& aTransport, TiltedPhoques::OverlayRenderHandler* apHandler)
    : TiltedPhoques::OverlayClient(apHandler)
    , m_transport(aTransport)
{
}

OverlayClient::~OverlayClient() noexcept
{
}

bool OverlayClient::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message)
{
    if (message->GetName() == "ui-event")
    {
        auto pArguments = message->GetArgumentList();

        auto eventName = pArguments->GetString(0).ToString();
        auto eventArgs = pArguments->GetList(1);

#ifndef PUBLIC_BUILD
        LOG(INFO) << "event=ui_event name=" << eventName;
#endif

        if (eventName == "connect")
            ProcessConnectMessage(eventArgs);
        else if (eventName == "disconnect")
            ProcessDisconnectMessage();
        else if (eventName == "revealPlayers")
            ProcessRevealPlayersMessage();
        else if (eventName == "sendMessage")
            ProcessChatMessage(eventArgs);
        else if (eventName == "setTime")
            ProcessSetTimeCommand(eventArgs);
        else if (eventName == "launchParty")
            World::Get().GetPartyService().CreateParty();
        else if (eventName == "leaveParty")
            World::Get().GetPartyService().LeaveParty();
        else if (eventName == "createPartyInvite")
        {
            uint32_t aPlayerId = eventArgs->GetInt(0);
            World::Get().GetPartyService().CreateInvite(aPlayerId);
        }
        else if (eventName == "acceptPartyInvite")
        {
            uint32_t aInviterId = eventArgs->GetInt(0);
            // push to main thread because the party service has to check validity of invite thread safely
            World::Get().GetRunner().Queue([aInviterId]() { World::Get().GetPartyService().AcceptInvite(aInviterId); });
        }
        else if (eventName == "kickPartyMember")
        {
            uint32_t aPlayerId = eventArgs->GetInt(0);
            World::Get().GetPartyService().KickPartyMember(aPlayerId);
        }
        else if (eventName == "changePartyLeader")
        {
            uint32_t aPlayerId = eventArgs->GetInt(0);
            World::Get().GetPartyService().ChangePartyLeader(aPlayerId);
        }
        else if (eventName == "teleportToPlayer")
            ProcessTeleportMessage(eventArgs);
        else if (eventName == "toggleDebugUI")
        {
            const bool isTradeAction =
                eventArgs &&
                eventArgs->GetSize() >= 2 &&
                eventArgs->GetString(0).ToString() == "__trade__";

            if (!isTradeAction)
            {
                // The legacy web debug toggle is intentionally disabled for
                // the production trade UI.
                return true;
            }

            const std::string action =
                eventArgs->GetString(1).ToString();

            if (action == "accept")
            {
                const auto sessionId = ParseUnsigned64(
                    eventArgs->GetString(2).ToString());

                if (sessionId)
                {
                    World::Get().GetRunner().Queue(
                        [sessionId = *sessionId]()
                        {
                            World::Get().ctx().at<TradeMenuService>().AcceptInvite(sessionId);
                        });
                }
            }
            else if (action == "reject")
            {
                const auto sessionId = ParseUnsigned64(
                    eventArgs->GetString(2).ToString());

                if (sessionId)
                {
                    World::Get().GetRunner().Queue(
                        [sessionId = *sessionId]()
                        {
                            World::Get().ctx().at<TradeMenuService>().RejectInvite(sessionId);
                        });
                }
            }
            else if (action == "setOffer")
            {
                const auto itemId = ParseUnsigned64(
                    eventArgs->GetString(2).ToString());
                const int quantity = eventArgs->GetInt(3);

                if (itemId && quantity >= 0)
                {
                    World::Get().GetRunner().Queue(
                        [itemId = *itemId,
                         quantity = static_cast<std::uint32_t>(quantity)]()
                        {
                            World::Get().ctx().at<TradeMenuService>().SetOfferedQuantity(
                                itemId,
                                quantity);
                        });
                }
            }
            else if (action == "confirm")
            {
                World::Get().GetRunner().Queue(
                    []()
                    {
                        World::Get().ctx().at<TradeMenuService>().ConfirmTrade();
                    });
            }
            else if (action == "cancel")
            {
                World::Get().GetRunner().Queue(
                    []()
                    {
                        World::Get().ctx().at<TradeMenuService>().CancelTrade();
                    });
            }
            else if (action == "dismiss")
            {
                World::Get().GetRunner().Queue(
                    []()
                    {
                        World::Get().ctx().at<TradeMenuService>().DismissTerminalState();
                    });
            }
            else if (action == "dismissOutgoing")
            {
                World::Get().GetRunner().Queue(
                    []()
                    {
                        World::Get().ctx().at<TradeMenuService>().DismissOutgoingInvite();
                    });
            }
            else if (action == "preview")
            {
                const auto itemId = ParseUnsigned64(
                    eventArgs->GetString(2).ToString());

                if (itemId)
                {
                    World::Get().GetRunner().Queue(
                        [itemId = *itemId]()
                        {
                            World::Get().ctx().at<TradeItemPreviewService>().SelectItem(itemId);
                        });
                }
            }
            else if (action == "clearPreview")
            {
                World::Get().GetRunner().Queue(
                    []()
                    {
                        World::Get().ctx().at<TradeItemPreviewService>().Clear();
                    });
            }
        }

        return true;
    }

    return false;
}

void OverlayClient::ProcessConnectMessage(CefRefPtr<CefListValue> aEventArgs)
{
    std::string baseIp = aEventArgs->GetString(0);
    if (baseIp == "localhost")
    {
        baseIp = "127.0.0.1";
    }

    uint16_t port = aEventArgs->GetInt(1) ? aEventArgs->GetInt(1) : 10578;
    World::Get().GetTransport().SetServerPassword(aEventArgs->GetString(2));

    std::string endpoint = baseIp + ":" + std::to_string(port);

    World::Get().GetRunner().Queue([endpoint] { World::Get().GetTransport().Connect(endpoint); });
}

void OverlayClient::ProcessDisconnectMessage()
{
    World::Get().GetRunner().Queue([]() { World::Get().GetTransport().Close(); });
}

void OverlayClient::ProcessRevealPlayersMessage()
{
    SetUIVisible(false);
    World::Get().GetMagicService().StartRevealingOtherPlayers();
}

void OverlayClient::ProcessChatMessage(CefRefPtr<CefListValue> aEventArgs)
{
    std::string contents = aEventArgs->GetString(1).ToString();
    if (!contents.empty())
    {
        SendChatMessageRequest messageRequest;
        messageRequest.MessageType = static_cast<ChatMessageType>(aEventArgs->GetInt(0));
        messageRequest.ChatMessage = contents;

        spdlog::info(L"Send chat message of type {}: '{}' ", messageRequest.MessageType, aEventArgs->GetString(1).ToWString());

        m_transport.Send(messageRequest);
    }
}

void OverlayClient::ProcessSetTimeCommand(CefRefPtr<CefListValue> aEventArgs)
{
    const uint8_t hours = static_cast<uint8_t>(aEventArgs->GetInt(0));
    const uint8_t minutes = static_cast<uint8_t>(aEventArgs->GetInt(1));
    const uint32_t senderId = m_transport.GetLocalPlayerId();
    World::Get().GetDispatcher().trigger(SetTimeCommandEvent(hours, minutes, senderId));
}

void OverlayClient::ProcessTeleportMessage(CefRefPtr<CefListValue> aEventArgs)
{
    TeleportRequest request{};
    request.PlayerId = aEventArgs->GetInt(0);

    m_transport.Send(request);
}

void OverlayClient::ProcessToggleDebugUI()
{
    World::Get().GetDebugService().m_showDebugStuff = !World::Get().GetDebugService().m_showDebugStuff;
}

void OverlayClient::SetUIVisible(bool aVisible) noexcept
{
    auto& uiSurface = World::Get().ctx().at<UiSurfaceService>();
    uiSurface.SetSurface(
        aVisible
            ? UiSurface::SkyrimTogether
            : UiSurface::None);
}
