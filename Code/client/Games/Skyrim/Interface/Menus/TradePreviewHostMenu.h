#pragma once

#include <Interface/IMenu.h>
#include <Interface/UI.h>
#include <Misc/BSFixedString.h>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11Query;

class TradePreviewHostMenu final : public IMenu
{
public:
    static const BSFixedString& GetName() noexcept;
    static bool Register() noexcept;
    static void Show() noexcept;
    static void Hide() noexcept;

    TradePreviewHostMenu() noexcept;
    ~TradePreviewHostMenu() override;

    void Accept(CallbackProcessor* apProcessor) override;
    void PostCreate() override;
    void Unk_03() override;
    UI_MESSAGE_RESULTS ProcessMessage(UIMessage& aMessage) override;
    void AdvanceMovie(float aInterval, std::uint32_t aCurrentTime) override;
    void PostDisplay() override;
    void PreDisplay() override;
    void RefreshPlatform() override;

private:
    static IMenu* Create(UIMessage* apMessage);
    void StartPreview() noexcept;
    void StopPreview() noexcept;
    void ResetRenderQueries() noexcept;
    void PollRenderQueries(ID3D11DeviceContext* apContext) noexcept;
    void BeginRenderQueries(
        ID3D11Device* apDevice,
        ID3D11DeviceContext* apContext) noexcept;
    void EndRenderQueries(ID3D11DeviceContext* apContext) noexcept;

    bool m_started{};
    bool m_postDisplayLogged{};
    bool m_updateMessageLogged{};
    bool m_begin3DInvoked{};
    bool m_firstRenderLogged{};
    bool m_renderUnavailableLogged{};
    bool m_hostMovieLoaded{};
    bool m_hostMovieDisplayLogged{};
    bool m_hostMovieDisplayReturnedLogged{};
    bool m_hostMovieUnavailableLogged{};
    bool m_hostMoviePrePassLogged{};
    bool m_renderQueriesIssued{};
    bool m_pipelineStatsLogged{};
    bool m_occlusionLogged{};
    bool m_pipelineQueryFailed{};
    bool m_occlusionQueryFailed{};
    std::uint32_t m_updateMessageCount{};
    std::uint32_t m_renderCallCount{};
    std::uint32_t m_renderNonZeroCount{};
    std::uint32_t m_firstRenderResult{};
    std::uint32_t m_hostMovieDisplayCalls{};
    std::uint32_t m_queryPollCount{};
    ID3D11Query* m_pipelineQuery{};
    ID3D11Query* m_occlusionQuery{};
};
