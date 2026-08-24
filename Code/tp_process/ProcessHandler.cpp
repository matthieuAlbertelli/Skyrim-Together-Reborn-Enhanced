
#include "ProcessHandler.h"

#include <CampaignBootstrapBridge.h>
#include <CampaignNativeLoadBridge.h>

#include <string>

ProcessHandler::ProcessHandler() noexcept
    : OverlayRenderProcessHandler("skyrimtogether")
{
}

void ProcessHandler::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    OverlayRenderProcessHandler::OnContextCreated(browser, frame, context);

    m_pCoreObject->SetValue("on", CefV8Value::CreateFunction("on", m_pEventsHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("off", CefV8Value::CreateFunction("off", m_pEventsHandler), V8_PROPERTY_ATTRIBUTE_NONE);

    m_pCoreObject->SetValue("connect", CefV8Value::CreateFunction("connect", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("disconnect", CefV8Value::CreateFunction("disconnect", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("revealPlayers", CefV8Value::CreateFunction("revealPlayers", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("sendMessage", CefV8Value::CreateFunction("sendMessage", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("setTime", CefV8Value::CreateFunction("setTime", m_pOverlayHandler),V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("deactivate", CefV8Value::CreateFunction("deactivate", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("launchParty", CefV8Value::CreateFunction("launchParty", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("leaveParty", CefV8Value::CreateFunction("leaveParty", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("createPartyInvite", CefV8Value::CreateFunction("createPartyInvite", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("acceptPartyInvite", CefV8Value::CreateFunction("acceptPartyInvite", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("kickPartyMember", CefV8Value::CreateFunction("kickPartyMember", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("changePartyLeader", CefV8Value::CreateFunction("changePartyLeader", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("teleportToPlayer", CefV8Value::CreateFunction("teleportToPlayer", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("toggleDebugUI", CefV8Value::CreateFunction("toggleDebugUI", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    m_pCoreObject->SetValue("characterCreationAction", CefV8Value::CreateFunction("characterCreationAction", m_pOverlayHandler), V8_PROPERTY_ATTRIBUTE_NONE);
    for (const std::string_view functionName :
         STRE::Campaign::kCampaignBootstrapCefFunctions)
    {
        const std::string name{functionName};
        m_pCoreObject->SetValue(
            name,
            CefV8Value::CreateFunction(name, m_pOverlayHandler),
            V8_PROPERTY_ATTRIBUTE_NONE);
        LOG(INFO) << "[STRE][CampaignBootstrapBridge] registered JS function="
                  << name;
    }
    for (const std::string_view functionName :
         STRE::Campaign::kCampaignNativeLoadCefFunctions)
    {
        const std::string name{functionName};
        m_pCoreObject->SetValue(
            name,
            CefV8Value::CreateFunction(name, m_pOverlayHandler),
            V8_PROPERTY_ATTRIBUTE_NONE);
        LOG(INFO) << "[STRE][CampaignNativeLoadBridge] registered JS function="
                  << name;
    }
}

void ProcessHandler::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context)
{
    OverlayRenderProcessHandler::OnContextReleased(browser, frame, context);
}
