#ifndef UNHIDENPCS_UI_HPP
#define UNHIDENPCS_UI_HPP
#pragma once
#include "fw/memory/hooks/d3d11.hpp"
#include "fw/memory/hooks/wndproc.hpp"

namespace ui
{
    extern float labelOffset;
    extern float fieldWidth;

    extern std::optional<std::unique_ptr<memory::hooks::D3D11>> d3dHook;
    extern std::optional<memory::hooks::WndProc>                wndProcHook;

    void tooltip(const char* text);

    bool checkbox(const char* label, const char* id, bool& value, const char* tip);

    bool combo(
        const char*        label,
        const char*        id,
        int&               value,
        const char* const* items,
        int                count,
        const char*        tip
    );

    bool sliderInt(
        const char* label,
        const char* id,
        int32_t&    value,
        int32_t     min,
        int32_t     max,
        const char* fmt,
        const char* tip
    );

    bool sliderFloat(
        const char* label,
        const char* id,
        float&      value,
        float       min,
        float       max,
        const char* fmt,
        const char* tip
    );

    bool button(const char* label);

    void separatorText(const char* text);

    bool textLink(const char* label, bool centered = false);

    void renderOptions();

    void renderWindow();

    bool wasKeyPressed(int vKey);

    bool wasComboPressed(const std::initializer_list<int>& combo);

    uintptr_t onWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    uint32_t onWndProcNexus(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void onD3DPresent();

    void onD3DResizeBuffers(const memory::hooks::D3D11* hk, bool pre);

    bool onD3DStarted(const memory::hooks::D3D11* hk);

    void onD3DShutdown(const memory::hooks::D3D11* hk);
}

#endif //UNHIDENPCS_UI_HPP
