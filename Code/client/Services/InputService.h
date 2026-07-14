#pragma once

struct UiSurfaceService;

/**
 * @brief Handles input handling for the UI.
 */
struct InputService
{
    explicit InputService(UiSurfaceService& aUiSurfaceService) noexcept;
    ~InputService() noexcept;

    static LRESULT WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    TP_NOCOPYMOVE(InputService);
};
