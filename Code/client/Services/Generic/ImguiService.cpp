#include <TiltedOnlinePCH.h>

#include <Services/ImguiService.h>
#include <Systems/RenderSystemD3D11.h>
#include <d3d11.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui.h>
#include <imgui_internal.h>

// According to imgui documentation we have to do it this way in order to avoid link conflicts with windows.h
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ImguiService::ImguiService()
    : OnDraw(m_drawSignal)
{
}

ImguiService::~ImguiService() noexcept
{
}

void ImguiService::Create(RenderSystemD3D11* apRenderSystem, HWND aHwnd)
{
    m_imDriver.Initialize(static_cast<void*>(aHwnd));

    // init platform
    if (!ImGui_ImplWin32_Init(aHwnd))
    {
        spdlog::error("Failed to initialize Imgui-Win32");
        return;
    }

    m_ready = ImGui_ImplDX11_Init(apRenderSystem->GetDevice(), apRenderSystem->GetDeviceContext());
}

void ImguiService::Render() const
{
    if (!m_ready)
        return;
    ImGui_ImplDX11_NewFrame();

    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    m_drawSignal.publish();
    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

bool ImguiService::RenderMainMenuTexture(
    ID3D11ShaderResourceView* apTexture, unsigned aWidth, unsigned aHeight, ID3D11DeviceContext* apContext, std::string_view aHint, float aHintAlpha) const
{
    if (!m_ready || !apContext || !ImGui::GetCurrentContext())
        return false;
    D3D11_VIEWPORT viewport{};
    UINT count = 1;
    apContext->RSGetViewports(&count, &viewport);
    if (!count || viewport.Width <= 0 || viewport.Height <= 0)
        return false;
    ID3D11RenderTargetView* target{};
    apContext->OMGetRenderTargets(1, &target, nullptr);
    if (!target)
        return false;
    if (!apTexture)
    {
        const float black[4]{0, 0, 0, 1};
        apContext->ClearRenderTargetView(target, black);
    }
    target->Release();
    if (apTexture && (!aWidth || !aHeight))
        return false;
    if (!apTexture && aHint.empty())
        return true;
    ImGui_ImplDX11_NewFrame();
    const ImVec2 origin(viewport.TopLeftX, viewport.TopLeftY);
    const ImVec2 size(viewport.Width, viewport.Height);
    ImDrawList list(ImGui::GetDrawListSharedData());
    list._ResetForNewFrame();
    list.PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y));
    if (apTexture)
    {
        const float scale = std::max(size.x / aWidth, size.y / aHeight);
        const ImVec2 imageSize(aWidth * scale, aHeight * scale);
        const ImVec2 imageMin(origin.x + (size.x - imageSize.x) * 0.5f, origin.y + (size.y - imageSize.y) * 0.5f);
        list.AddImage(apTexture, imageMin, ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y));
    }
    if (!aHint.empty() && aHintAlpha > 0.0f)
    {
        // Text is resolved by the feature's localization catalog, never here.
        // Explicit font/atlas also works before ImGui's first full UI frame.
        const auto& io = ImGui::GetIO();
        auto* font = io.FontDefault ? io.FontDefault : (io.Fonts->Fonts.empty() ? nullptr : io.Fonts->Fonts[0]);
        if (font && font->IsLoaded())
        {
            float fontSize = std::clamp(size.y / 54.0f, 14.0f, 32.0f);
            const auto* begin = aHint.data();
            const auto* end = begin + aHint.size();
            auto textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, begin, end);
            if (textSize.x > size.x * 0.8f)
            {
                fontSize *= size.x * 0.8f / textSize.x;
                textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, begin, end);
            }
            const float margin = std::clamp(size.y / 30.0f, 16.0f, 64.0f);
            const ImVec2 position(origin.x + size.x - margin - textSize.x, origin.y + size.y - margin - textSize.y);
            const auto alpha = static_cast<int>(std::clamp(aHintAlpha, 0.0f, 1.0f) * 220.0f);
            list.PushTextureID(font->ContainerAtlas->TexID);
            list.AddText(font, fontSize, ImVec2(position.x + 1, position.y + 1), IM_COL32(0, 0, 0, alpha), begin, end);
            list.AddText(font, fontSize, position, IM_COL32(240, 240, 240, alpha), begin, end);
            list.PopTextureID();
        }
    }
    list.PopClipRect();
    ImDrawList* lists[]{&list};
    ImDrawData data;
    data.Valid = true;
    data.CmdListsCount = 1;
    data.TotalIdxCount = list.IdxBuffer.Size;
    data.TotalVtxCount = list.VtxBuffer.Size;
    data.CmdLists = lists;
    data.DisplayPos = origin;
    data.DisplaySize = size;
    data.FramebufferScale = ImVec2(1, 1);
    // The existing backend restores the engine's D3D state after this pass.
    ImGui_ImplDX11_RenderDrawData(&data);
    return true;
}

void ImguiService::Reset() const
{
    // TODO: idk how imgui handles this
}

LRESULT ImguiService::WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
}

void ImguiService::RawInputHandler(RAWINPUT& aRawinput)
{
    if (ImGui::GetCurrentContext() == NULL)
        return;

    ImGuiIO& io = ImGui::GetIO();
    if (aRawinput.header.dwType == RIM_TYPEMOUSE)
    {
        const auto mouse = aRawinput.data.mouse;

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_DOWN)
        {
            io.MouseDown[0] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_LEFT_BUTTON_UP)
        {
            io.MouseDown[0] = false;
        }

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_DOWN)
        {
            io.MouseDown[1] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_RIGHT_BUTTON_UP)
        {
            io.MouseDown[1] = false;
        }

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_DOWN)
        {
            io.MouseDown[2] = true;
        }

        if (mouse.usButtonFlags & RI_MOUSE_MIDDLE_BUTTON_UP)
        {
            io.MouseDown[2] = false;
        }
    }
}
