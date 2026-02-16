#include "ui.hpp"

#include "hook.hpp"
#include "imgui.h"
#include "unpc.hpp"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"

constexpr auto LABEL_OFFSET = 272.0f;
constexpr auto FIELD_WIDTH  = 230.0f;

std::optional<std::unique_ptr<memory::hooks::d3d11>> ui::d3dHook {};
std::optional<memory::hooks::wndproc>                ui::wndProcHook {};

void ui::tooltip(const char* text)
{
    if (!ImGui::IsItemHovered())
        return;

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
}

bool ui::checkbox(
    const char* label,
    const char* id,
    bool&       value,
    const char* tip,
    const float labelOffset = LABEL_OFFSET
)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    return ImGui::Checkbox(id, &value);
}

bool ui::combo(
    const char*        label,
    const char*        id,
    int&               value,
    const char* const* items,
    const int          count,
    const char*        tip,
    const float        labelOffset = LABEL_OFFSET
)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(FIELD_WIDTH);
    const bool result = ImGui::Combo(id, &value, items, count);
    ImGui::PopItemWidth();
    return result;
}

bool ui::sliderInt(
    const char*   label,
    const char*   id,
    int32_t&      value,
    const int32_t min,
    const int32_t max,
    const char*   fmt,
    const char*   tip,
    const float   labelOffset = LABEL_OFFSET
)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(FIELD_WIDTH);
    const bool result = ImGui::SliderInt(id, &value, min, max, fmt);
    ImGui::PopItemWidth();
    return result;
}

bool ui::sliderFloat(
    const char* label,
    const char* id,
    float&      value,
    const float min,
    const float max,
    const char* fmt,
    const char* tip,
    const float labelOffset = LABEL_OFFSET
)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(FIELD_WIDTH);
    const bool result = ImGui::SliderFloat(id, &value, min, max, fmt);
    ImGui::PopItemWidth();
    return result;
}

const char* ranks[] = { "Normal", "Veteran", "Elite", "Champion", "Legendary" };
const char* modes[] = { "Both", "Attackable", "Non-Attackable" };

void ui::renderOptions()
{
    if (!unpc::mumbleLink || unpc::mumbleLink->getContext().IsCompetitiveMode())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        ImGui::Text("Disabled in Competitive");
        ImGui::PopStyleColor();
        return;
    }

    ImGui::Text("Unhide:");
    ImGui::Indent();
    if (auto alwaysShowTarget = unpc::settings->getAlwaysShowTarget(); ui::checkbox(
        "Target",
        "##AlwaysShowTarget",
        alwaysShowTarget,
        "Always show the targeted character, even if it would be hidden."
    ))
    {
        unpc::settings->setAlwaysShowTarget(alwaysShowTarget);
        re::forceVisibility++;
    }
    if (auto playerOwned = unpc::settings->getPlayerOwned(); ui::checkbox(
        "Player Owned",
        "##PlayerOwned",
        playerOwned,
        "NPCs that are owned by players (pets, clones, minis etc) will also be unhidden."
    ))
    {
        unpc::settings->setPlayerOwned(playerOwned);
        re::forceVisibility++;
    }
    if (auto minRank = unpc::settings->getMinimumRank(); ui::combo(
        "Minimum Rank",
        "##minRank",
        minRank,
        ranks,
        IM_ARRAYSIZE(ranks),
        "Only NPCs that have at least this rank gets unhidden."
    ))
    {
        unpc::settings->setMinimumRank(minRank);
        re::forceVisibility++;
    }
    if (auto attackable = unpc::settings->getAttackable(); ui::combo(
        "Attackable",
        "##attackable",
        attackable,
        modes,
        IM_ARRAYSIZE(modes),
        "Only NPCs that match this gets unhidden."
    ))
    {
        unpc::settings->setAttackable(attackable);
        re::forceVisibility++;
    }
    if (auto maxDistance = static_cast<int32_t>(unpc::settings->getMaximumDistance()); ui::sliderInt(
        "Max Distance",
        "##maxDistance",
        maxDistance,
        0,
        1000,
        maxDistance == 0 ? "Off" : "%d meters",
        "NPCs within this distance will be unhidden. (0=no distance check)"
    ))
    {
        unpc::settings->setMaximumDistance(static_cast<float>(maxDistance));
        re::forceVisibility++;
    }
    ImGui::Unindent();

    ImGui::Text("Hide:");
    ImGui::Indent();
    if (auto hidePlayers = unpc::settings->getHidePlayers(); ui::checkbox(
        "Players",
        "##HidePlayers",
        hidePlayers,
        "Players will be hidden when this is ticked, useful for boosting performance.\n"
        "Their names are still visible, and you can still target them"
    ))
    {
        unpc::settings->setHidePlayers(hidePlayers);
        re::forceVisibility++;
    }
    if (auto maxPlayersVisible = unpc::settings->getMaxPlayersVisible(); ui::sliderInt(
        "Max Players",
        "##MaxPlayersVisible",
        maxPlayersVisible,
        0,
        50,
        maxPlayersVisible == 0 ? "Off" : "%d",
        "Maximum number of visible players. (0=no limit)"
    ))
    {
        unpc::settings->setMaxPlayersVisible(maxPlayersVisible);
        re::forceVisibility++;
    }
    if (auto maxPlayerOwnedVisible = unpc::settings->getMaxPlayerOwnedVisible(); ui::sliderInt(
        "Max Player Owned",
        "##MaxPlayerOwnedVisible",
        maxPlayerOwnedVisible,
        0,
        50,
        maxPlayerOwnedVisible == 0 ? "Off" : "%d",
        "Maximum number of visible player owned NPCs. (0=no limit)"
    ))
    {
        unpc::settings->setMaxPlayerOwnedVisible(maxPlayerOwnedVisible);
        re::forceVisibility++;
    }
    auto hidePlayerOwned = unpc::settings->getHidePlayerOwned();
    if (ui::checkbox(
        "Player Owned",
        "##HidePlayerOwned",
        hidePlayerOwned,
        "NPCs that are owned by players (pets, clones, minis etc) will be hidden."
    ))
    {
        unpc::settings->setHidePlayerOwned(hidePlayerOwned);
        re::forceVisibility++;
    }
    if (hidePlayerOwned)
    {
        ImGui::SameLine();
        const float offset = LABEL_OFFSET + ImGui::GetFontSize() * 1.5f;
        ImGui::Indent(offset);
        ImGui::Text("Mine Too");
        tooltip("Also hide NPCs that are owned by you.");
        ImGui::SameLine();
        auto hidePlayerOwnedSelf = unpc::settings->getHidePlayerOwnedSelf();
        if (ImGui::Checkbox("##HidePlayerOwnedSelf", &hidePlayerOwnedSelf))
        {
            unpc::settings->setHidePlayerOwnedSelf(hidePlayerOwnedSelf);
            re::forceVisibility++;
        }
        ImGui::Unindent(offset);
    }
    if (auto disableInInstances = unpc::settings->getDisableHidingInInstances(); ui::checkbox(
        "Disable in Instances",
        "##DisableInInstances",
        disableInInstances,
        "Disables the hiding options while in an instance (Fractals, Dungeons etc.)"
    ))
    {
        unpc::settings->setDisableHidingInInstances(disableInInstances);
        re::forceVisibility++;
    }
    ImGui::Unindent();

    ImGui::Text("Misc:");
    ImGui::Indent();
    if (auto forceConsole = unpc::settings->getForceConsole(); ui::checkbox(
        "Force Console",
        "##ForceConsole",
        forceConsole,
        "Forces the creation of a console window when set to true.\n"
        "Note: If the console window is exited, then the game will exit as well."
    ))
    {
        unpc::settings->setForceConsole(forceConsole);
        unpc::logger->setConsole(forceConsole);
    }

    if (auto loadScreenBoost = unpc::settings->getLoadScreenBoost(); ui::checkbox(
        "Loading Screen Boost",
        "##LoadScreenBoost",
        loadScreenBoost,
        "Speed up loading screens by temporarily limiting number of characters to 0 when one is triggered.\n"
        "Note: This will cause characters to start loading after the loading screen is finished,\n"
        "meaning there will be invisible characters for a bit after loading."
    ))
    {
        unpc::settings->setLoadScreenBoost(loadScreenBoost);
    }

    if (ImGui::Button("Force Visibility", { LABEL_OFFSET + FIELD_WIDTH, 30 }))
    {
        re::forceVisibility++;
    }
    tooltip(
        "Forces all characters to be visible for the next frame\n"
        "Useful for \"resetting\" things after modifying any settings."
    );
#ifndef BUILDING_ON_GITHUB
    if (ImGui::Button("Print Target Info", { LABEL_OFFSET + FIELD_WIDTH, 30 }))
    {
        re::doPrintTargetInformation = true;
    }
#endif
    ImGui::Unindent();

    ImGui::Text("Overlay:");
    ImGui::Indent();
    if (auto disableOverlay = unpc::settings->getDisableOverlay(); ui::checkbox(
        "Disable Overlay",
        "##DisableOverlay",
        disableOverlay,
        "Disable the built in overlay\n" "Note: This immediately unloads this overlay,\n"
        "if you want to re-enable this overlay you have to \n" "edit config.cfg and set DisableOverlay to false."
    ))
    {
        unpc::settings->setDisableOverlay(disableOverlay);
        if (disableOverlay)
        {
            unpc::unloadOverlay = true;
        }
    }
    if (float overlayFontSize = unpc::settings->getOverlayFontSize(); ui::sliderFloat(
        "Font Size",
        "##OverlayFontSize",
        overlayFontSize,
        10,
        24,
        "%.0f",
        "The font size used for the overlay\nRequires a restart/reload to reflect changes"
    ))
    {
        unpc::settings->setOverlayFontSize(std::roundf(overlayFontSize));
    }
    ImGui::Unindent();
}

void ui::renderWindow()
{
    if (unpc::exit)
        return;
    if (!unpc::settings || !unpc::settings->loaded())
        return;
    if (!unpc::logger)
        return;

    bool       open       = unpc::settings->getArcDPS_UIOpen();
    const bool startState = open;

    if (wasComboPressed({ VK_LMENU, VK_LSHIFT, 'U' }))
    {
        open = !open;
        unpc::settings->setArcDPS_UIOpen(open);
        return;
    }

    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2 { 0, 0 }); // Auto size
    if (ImGui::Begin("UnhideNPCs [ ALT+SHIFT+U ]", &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        renderOptions();
    }
    ImGui::End();

    if (wasComboPressed({ VK_LMENU, VK_LSHIFT, 'U' }))
    {
        open = !open;
    }

    if (startState != open)
    {
        unpc::settings->setArcDPS_UIOpen(open);
    }
}

static SHORT lastState[256] = { 0 };

bool ui::wasKeyPressed(const int vKey)
{
    const SHORT current = GetAsyncKeyState(vKey);
    const bool  pressed = (current & 0x8000) && !(lastState[vKey] & 0x8000);
    lastState[vKey]     = current;
    return pressed;
}

bool ui::wasComboPressed(const std::initializer_list<int>& combo)
{
    if (combo.size() < 1)
        return false;
    auto it = combo.begin();

    // All keys except the last must be currently down
    for (const auto end = combo.end() - 1; it != end; ++it)
    {
        if (!(GetAsyncKeyState(*it) & 0x8000))
            return false;
    }

    // The last key must have just been pressed
    return wasKeyPressed(*(combo.end() - 1));
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

bool ui::OnWndProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    if (unpc::exit)
        return false;

    auto& io = ImGui::GetIO();

    if (msg == WM_MOUSEMOVE)
    {
        io.MousePos = ImVec2 { static_cast<float>(LOWORD(lParam)), static_cast<float>(HIWORD(lParam)) };
    }

    if (io.WantCaptureMouse)
    {
        switch (msg)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONDBLCLK:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_SETCURSOR: ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
            return true;
        default: break;
        }
    }

    if (io.WantCaptureKeyboard || io.WantTextInput)
    {
        switch (msg)
        {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_CHAR:
        case WM_SYSCHAR: ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
            if (io.WantTextInput)
                return true;
            break;
        default: break;
        }
    }

    if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
    {
        ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
    }

    return false;
}

void ui::OnD3DPresent()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    renderWindow();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ui::OnD3DResizeBuffers(const memory::hooks::d3d11* hk, const bool pre)
{
    LOG_DBG("OnD3DResizeBuffers");

    if (pre)
    {
        ImGui_ImplDX11_InvalidateDeviceObjects();
    }
    else
    {
        ImGui_ImplDX11_Init(hk->device(), hk->context());
    }
}

bool ui::OnD3DStarted(const memory::hooks::d3d11* hk)
{
    LOG_DBG("OnD3DStarted");

    ImGui::CreateContext();
    ImGui_ImplWin32_Init(hk->hWnd());

    if (!ImGui_ImplDX11_Init(hk->device(), hk->context()))
    {
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig cfg {};
    cfg.SizePixels = unpc::settings->getOverlayFontSize();
    io.FontDefault = io.Fonts->AddFontDefault(&cfg);
    io.Fonts->Build();

    wndProcHook.emplace(hk->hWnd());
    wndProcHook->addCallback(OnWndProc);
    if (!wndProcHook->enable())
    {
        wndProcHook.reset();
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        return false;
    }

    return true;
}

void ui::OnD3DShutdown(const memory::hooks::d3d11* hk)
{
    LOG_DBG("OnD3DShutdown");

    if (wndProcHook)
        wndProcHook.reset();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
