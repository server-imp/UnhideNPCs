#include "ui.hpp"

#include <map>
#include <set>

#include "hook.hpp"
#include "imgui.h"
#include "unpc.hpp"
#include "backends/imgui_impl_dx11.h"
#include "backends/imgui_impl_win32.h"
#include "dumping.hpp"
#include "MumbleLink.hpp"

#include <shellapi.h>

#include "imgui_internal.h"

float labelOffset = -1;
float fieldWidth  = -1;

std::optional<std::unique_ptr<memory::hooks::D3D11>> ui::d3dHook {};
std::optional<memory::hooks::WndProc>                ui::wndProcHook {};

void ui::tooltip(const char* text)
{
    if (!ImGui::IsItemHovered())
    {
        return;
    }

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(text);
    ImGui::EndTooltip();
}

bool ui::checkbox(const char* label, const char* id, bool& value, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    return ImGui::Checkbox(id, &value);
}

bool ui::combo(const char* label, const char* id, int& value, const char* const* items, int count, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(fieldWidth);
    const bool result = ImGui::Combo(id, &value, items, count);
    ImGui::PopItemWidth();
    return result;
}

bool ui::sliderInt(const char* label, const char* id, int32_t& value, const int32_t min, const int32_t max, const char* fmt, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(fieldWidth);
    const bool result = ImGui::SliderInt(id, &value, min, max, fmt);
    ImGui::PopItemWidth();
    return result;
}

bool ui::sliderFloat(const char* label, const char* id, float& value, const float min, const float max, const char* fmt, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + labelOffset);
    ImGui::PushItemWidth(fieldWidth);
    const bool result = ImGui::SliderFloat(id, &value, min, max, fmt);
    ImGui::PopItemWidth();
    return result;
}

bool ui::button(const char* label)
{
    return ImGui::Button(label, { labelOffset + fieldWidth, 30 });
}

void ui::separatorText(const char* text)
{
    const ImGuiStyle& style = ImGui::GetStyle();

    const float fullWidth = ImGui::GetContentRegionAvail().x;
    const float textWidth = ImGui::CalcTextSize(text).x;

    float lineWidth = (fullWidth - textWidth - style.ItemSpacing.x * 2.0f) * 0.5f;
    if (lineWidth < 1.0f)
    {
        lineWidth = 1.0f;
    }

    const ImVec2 cursor = ImGui::GetCursorScreenPos();
    ImDrawList*  draw   = ImGui::GetWindowDrawList();

    const float y = cursor.y + ImGui::GetTextLineHeight() * 0.5f;

    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Separator);

    // Left line
    draw->AddLine(ImVec2(cursor.x, y), ImVec2(cursor.x + lineWidth, y), color);

    // Text
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + lineWidth + style.ItemSpacing.x);
    ImGui::TextUnformatted(text);

    // Right line
    const ImVec2 textEnd = ImGui::GetItemRectMax();

    draw->AddLine(ImVec2(textEnd.x + style.ItemSpacing.x, y), ImVec2(cursor.x + fullWidth, y), color);
}

bool ui::textLink(const char* label, const bool centered)
{
    const ImVec2 textSize = ImGui::CalcTextSize(label);
    const float  startX   = ImGui::GetCursorPosX();

    if (centered)
    {
        const float avail = ImGui::GetContentRegionAvail().x;
        ImGui::SetCursorPosX(startX + (avail - textSize.x) * 0.5f);
    }

    const ImVec2 pos = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(label, textSize);
    const bool hovered = ImGui::IsItemHovered();
    const bool clicked = ImGui::IsItemClicked();

    const ImU32 color = ImGui::GetColorU32(hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);

    ImGui::GetWindowDrawList()->AddText(pos, color, label);

    if (hovered)
    {
        ImGui::GetWindowDrawList()->AddLine(ImVec2(pos.x, pos.y + textSize.y), ImVec2(pos.x + textSize.x, pos.y + textSize.y), color, 1.0f);
    }

    return clicked;
}

struct CheckboxGroupEntry
{
    const char* label {};
    const char* tooltip {};
    bool*       value {};
    bool*       changed {};

    CheckboxGroupEntry(const char* label, const char* tooltip, bool* value, bool* changed) : label(label), tooltip(tooltip), value(value), changed(changed) {}
};

bool checkboxGroup(const char* label, const char* tooltip, const std::vector<CheckboxGroupEntry>& items)
{
    const size_t count = items.size();

    if (count == 0)
    {
        return false;
    }

    char id[32];
    if (count == 1)
    {
        snprintf(id, sizeof(id), "##%p", static_cast<void*>(items[0].value));
        auto result = ui::checkbox(items[0].label, id, *items[0].value, items[0].tooltip);
        if (result)
        {
            *items[0].changed = true;
        }
        return result;
    }

    const ImVec2 initialCursorPos = ImGui::GetCursorPos();
    const ImVec2 spacing          = ImGui::GetStyle().ItemSpacing;
    const float  checkBoxSize     = ImGui::GetFrameHeight();
    const float  rowHeight        = spacing.y + checkBoxSize;

    if (label)
    {
        ImGui::TextUnformatted(label);
        if (tooltip)
        {
            ui::tooltip(tooltip);
        }
    }

    bool result = false;

    for (size_t i = 0; i < count; ++i)
    {
        const bool   isOdd = (i & 1) != 0;
        const size_t row   = i >> 1;

        const auto& item = items[i];

        const float y = initialCursorPos.y + static_cast<float>(row) * rowHeight;
        const float x = initialCursorPos.x + (isOdd ? 2.0f : 1.0f) * labelOffset;

        if (item.label)
        {
            const ImVec2 labelSize = ImGui::CalcTextSize(item.label);
            const float  labelX    = x - spacing.x - labelSize.x;

            ImGui::SetCursorPos(ImVec2(labelX, y + ImGui::GetStyle().FramePadding.y));
            ImGui::TextUnformatted(item.label);
            if (item.tooltip)
            {
                ui::tooltip(item.tooltip);
            }
        }

        ImGui::SetCursorPos(ImVec2(x, y));
        char id[32];
        snprintf(id, sizeof(id), "##%p", static_cast<void*>(item.value));

        if (ImGui::Checkbox(id, item.value))
        {
            *item.changed = true;
            result        = true;
        }
    }

    // Move cursor below entire group
    const size_t totalRows = (count + 1) >> 1;
    ImGui::SetCursorPos(ImVec2(initialCursorPos.x, initialCursorPos.y + static_cast<float>(totalRows) * rowHeight));

    return result;
}

const char* ranks[] = { "Normal", "Veteran", "Elite", "Champion", "Legendary" };
const char* modes[] = { "Both", "Attackable", "Non-Attackable" };

void ui::renderOptions()
{
    if (!unpc::mumbleLink || unpc::mumbleLink->getContext().isCompetitiveMode())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        ImGui::Text("Disabled in Competitive");
        ImGui::PopStyleColor();
        return;
    }

    if (labelOffset < 0)
    {
        const auto fontSize = ImGui::GetFontSize();

        labelOffset = fontSize * 12;

        fieldWidth = labelOffset + ImGui::GetFrameHeight();
    }

    separatorText("Show");
    bool alwaysShowTarget = unpc::settings->getAlwaysShowTarget();
    bool playerOwned      = unpc::settings->getPlayerOwned();
    bool unhideNpcs       = unpc::settings->getUnhideNpcs();
    bool alwaysShowTargetChanged {}, playerOwnedChanged {}, unhideNpcsChanged {};

    if (checkboxGroup(
        "Unhide",
        "The selected will be unhidden",
        std::vector<CheckboxGroupEntry> {
            { "NPCs", unpc::settings->getCommentUnhideNpcs().c_str(), &unhideNpcs, &unhideNpcsChanged },
            { "Player-Owned", unpc::settings->getCommentPlayerOwned().c_str(), &playerOwned, &playerOwnedChanged },
            { "Target", unpc::settings->getCommentAlwaysShowTarget().c_str(), &alwaysShowTarget, &alwaysShowTargetChanged }
        }
    ))
    {
        if (alwaysShowTargetChanged)
        {
            unpc::settings->setAlwaysShowTarget(alwaysShowTarget);
        }
        else if (playerOwnedChanged)
        {
            unpc::settings->setPlayerOwned(playerOwned);
        }
        else if (unhideNpcsChanged)
        {
            unpc::settings->setUnhideNpcs(unhideNpcs);
        }
    }
    ImGui::NewLine();
    if (auto maxDistance = static_cast<int32_t>(unpc::settings->getMaximumDistance()); ui::sliderInt(
        "Max Distance",
        "##maxDistance",
        maxDistance,
        0,
        1000,
        maxDistance == 0 ? "Unlimited" : "%d meters",
        unpc::settings->getCommentMaximumDistance().c_str()
    ))
    {
        unpc::settings->setMaximumDistance(static_cast<float>(maxDistance));
        ++re::forceVisibility;
    }
    if (auto minRank = unpc::settings->getMinimumRank(); ui::combo(
        "Minimum Rank",
        "##minRank",
        minRank,
        ranks,
        IM_ARRAYSIZE(ranks),
        unpc::settings->getCommentMinimumRank().c_str()
    ))
    {
        unpc::settings->setMinimumRank(minRank);
        ++re::forceVisibility;
    }
    if (auto attackable = unpc::settings->getAttackable(); ui::combo(
        "Attackable",
        "##attackable",
        attackable,
        modes,
        IM_ARRAYSIZE(modes),
        unpc::settings->getCommentAttackable().c_str()
    ))
    {
        unpc::settings->setAttackable(attackable);
        ++re::forceVisibility;
    }

    ui::separatorText("Hide");
    bool hideAllPlayers       = unpc::settings->getHidePlayers();
    bool hideBlockedPlayers   = unpc::settings->getHideBlockedPlayers();
    bool hideNonGroupPlayers  = unpc::settings->getHideNonGroupMembers();
    bool hideNonGuildPlayers  = unpc::settings->getHideNonGuildMembers();
    bool hideNonFriendPlayers = unpc::settings->getHideNonFriends();
    bool hideAllPlayersChanged {}, hideBlockedPlayersChanged {}, hideNonGroupPlayersChanged {}, hideNonGuildPlayersChanged {}, hideNonFriendPlayersChanged {};

    if (checkboxGroup(
        "Players",
        "Players that match the selected critera will be hidden",
        std::vector<CheckboxGroupEntry> {
            { "All", unpc::settings->getCommentHidePlayers().c_str(), &hideAllPlayers, &hideAllPlayersChanged },
            { "Blocked", unpc::settings->getCommentHideBlockedPlayers().c_str(), &hideBlockedPlayers, &hideBlockedPlayersChanged },
            { "Non-Group", unpc::settings->getCommentHideNonGroupMembers().c_str(), &hideNonGroupPlayers, &hideNonGroupPlayersChanged },
            { "Non-Guild", unpc::settings->getCommentHideNonGuildMembers().c_str(), &hideNonGuildPlayers, &hideNonGuildPlayersChanged },
            { "Non-Friends", unpc::settings->getCommentHideNonFriends().c_str(), &hideNonFriendPlayers, &hideNonFriendPlayersChanged }
        }
    ))
    {
        if (hideAllPlayersChanged)
        {
            unpc::settings->setHidePlayers(hideAllPlayers);
        }
        else if (hideBlockedPlayersChanged)
        {
            unpc::settings->setHideBlockedPlayers(hideBlockedPlayers);
        }
        else if (hideNonGroupPlayersChanged)
        {
            unpc::settings->setHideNonGroupMembers(hideNonGroupPlayers);
        }
        else if (hideNonGuildPlayersChanged)
        {
            unpc::settings->setHideNonGuildMembers(hideNonGuildPlayers);
        }
        else if (hideNonFriendPlayersChanged)
        {
            unpc::settings->setHideNonFriends(hideNonFriendPlayers);
        }
        ++re::forceVisibility;
    }
    ImGui::NewLine();

    bool hideAllPlayerOwned = unpc::settings->getHidePlayerOwned();
    bool hideBlockedPlayerOwned = unpc::settings->getHideBlockedPlayersOwned();
    bool hideNonGroupPlayerOwned = unpc::settings->getHideNonGroupMembersOwned();
    bool hideNonGuildPlayerOwned = unpc::settings->getHideNonGuildMembersOwned();
    bool hideNonFriendPlayerOwned = unpc::settings->getHideNonFriendsOwned();
    bool hideMyOwned = unpc::settings->getHidePlayerOwnedSelf();
    bool hideAllPlayerOwnedChanged {}, hideBlockedPlayerOwnedChanged {}, hideNonGroupPlayerOwnedChanged {}, hideNonGuildPlayerOwnedChanged {},
         hideNonFriendPlayerOwnedChanged {}, hideMyOwnedChanged;

    if (checkboxGroup(
        "Player-Owned",
        "Characters that are owned by the selected type of players will be hidden",
        std::vector<CheckboxGroupEntry> {
            { "All", unpc::settings->getCommentHidePlayerOwned().c_str(), &hideAllPlayerOwned, &hideAllPlayerOwnedChanged },
            { "Blocked", unpc::settings->getCommentHideBlockedPlayersOwned().c_str(), &hideBlockedPlayerOwned, &hideBlockedPlayerOwnedChanged },
            { "Non-Group", unpc::settings->getCommentHideNonGroupMembersOwned().c_str(), &hideNonGroupPlayerOwned, &hideNonGroupPlayerOwnedChanged },
            { "Non-Guild", unpc::settings->getCommentHideNonGuildMembersOwned().c_str(), &hideNonGuildPlayerOwned, &hideNonGuildPlayerOwnedChanged },
            { "Non-Friends", unpc::settings->getCommentHideNonFriendsOwned().c_str(), &hideNonFriendPlayerOwned, &hideNonFriendPlayerOwnedChanged },
            { "Mine", unpc::settings->getCommentHidePlayerOwnedSelf().c_str(), &hideMyOwned, &hideMyOwnedChanged }
        }
    ))
    {
        if (hideAllPlayerOwnedChanged)
        {
            unpc::settings->setHidePlayerOwned(hideAllPlayerOwned);
        }
        else if (hideBlockedPlayerOwnedChanged)
        {
            unpc::settings->setHideBlockedPlayersOwned(hideBlockedPlayerOwned);
        }
        else if (hideNonGroupPlayerOwnedChanged)
        {
            unpc::settings->setHideNonGroupMembersOwned(hideNonGroupPlayerOwned);
        }
        else if (hideNonGuildPlayerOwnedChanged)
        {
            unpc::settings->setHideNonGuildMembersOwned(hideNonGuildPlayerOwned);
        }
        else if (hideNonFriendPlayerOwnedChanged)
        {
            unpc::settings->setHideNonFriendsOwned(hideNonFriendPlayerOwned);
        }
        else if (hideMyOwnedChanged)
        {
            unpc::settings->setHidePlayerOwnedSelf(hideMyOwned);
        }
        ++re::forceVisibility;
    }
    ImGui::NewLine();

    if (auto maxPlayersVisible = unpc::settings->getMaxPlayersVisible(); ui::sliderInt(
        "Max Players",
        "##MaxPlayersVisible",
        maxPlayersVisible,
        0,
        250,
        maxPlayersVisible == 0 ? "Unlimited" : "%d",
        unpc::settings->getCommentMaxPlayersVisible().c_str()
    ))
    {
        unpc::settings->setMaxPlayersVisible(maxPlayersVisible);
        ++re::forceVisibility;
    }
    if (auto maxPlayerOwnedVisible = unpc::settings->getMaxPlayerOwnedVisible(); ui::sliderInt(
        "Max Player-Owned",
        "##MaxPlayerOwnedVisible",
        maxPlayerOwnedVisible,
        0,
        100,
        maxPlayerOwnedVisible == 0 ? "Unlimited" : "%d",
        unpc::settings->getCommentMaxPlayerOwnedVisible().c_str()
    ))
    {
        unpc::settings->setMaxPlayerOwnedVisible(maxPlayerOwnedVisible);
        ++re::forceVisibility;
    }
    if (auto maxNpcs = unpc::settings->getMaxNpcs(); ui::sliderInt(
        "Max NPCs",
        "##maxNpcs",
        maxNpcs,
        0,
        1000,
        maxNpcs == 0 ? "Unlimited" : "%d",
        unpc::settings->getCommentMaxNpcs().c_str()
    ))
    {
        unpc::settings->setMaxNpcs(maxNpcs);
        ++re::forceVisibility;
    }
    ImGui::NewLine();
    if (auto disableInInstances = unpc::settings->getDisableHidingInInstances(); ui::checkbox(
        "Disable in Instances",
        "##DisableInInstances",
        disableInInstances,
        unpc::settings->getCommentDisableHidingInInstances().c_str()
    ))
    {
        unpc::settings->setDisableHidingInInstances(disableInInstances);
        ++re::forceVisibility;
    }

    separatorText("Misc");
    std::vector<CheckboxGroupEntry> entries {};

    bool forceConsole    = unpc::settings->getForceConsole();
    bool loadScreenBoost = unpc::settings->getLoadScreenBoost();
    bool disableOverlay  = unpc::settings->getDisableOverlay();
    bool closeOnEscape   = unpc::settings->getCloseOnEscape();
    bool forceConsoleChanged {}, loadScreenBoostChanged {}, disableOverlayChanged {}, closeOnEscapeChanged {};

    entries.emplace_back("Console", unpc::settings->getCommentForceConsole().c_str(), &forceConsole, &forceConsoleChanged);
    entries.emplace_back("Loading Boost", unpc::settings->getCommentLoadScreenBoost().c_str(), &loadScreenBoost, &loadScreenBoostChanged);

    if (unpc::mode == unpc::EMode::Nexus)
    {
        goto footer;
    }

    entries.emplace_back("Esc To Close", unpc::settings->getCommentCloseOnEscape().c_str(), &closeOnEscape, &closeOnEscapeChanged);

    if (unpc::mode == unpc::EMode::Proxy || unpc::mode == unpc::EMode::Injected)
    {
        entries.emplace_back("Disable Overlay", unpc::settings->getCommentDisableOverlay().c_str(), &disableOverlay, &disableOverlayChanged);
    }

footer:
    if (checkboxGroup(nullptr, nullptr, entries))
    {
        if (forceConsoleChanged)
        {
            unpc::settings->setForceConsole(forceConsole);
            unpc::logger->setConsole(forceConsole);
        }
        else if (loadScreenBoostChanged)
        {
            unpc::settings->setLoadScreenBoost(loadScreenBoost);
        }
        else if (closeOnEscapeChanged)
        {
            unpc::settings->setCloseOnEscape(closeOnEscape);
        }
        else if (disableOverlayChanged)
        {
            unpc::settings->setDisableOverlay(disableOverlay);
            if (disableOverlay)
            {
                unpc::unloadOverlay = true;
            }
        }
    }
    ImGui::NewLine();

    if (unpc::mode == unpc::EMode::Proxy || unpc::mode == unpc::EMode::Injected)
    {
        if (float overlayFontSize = unpc::settings->getOverlayFontSize(); ui::sliderFloat(
            "Font Size",
            "##OverlayFontSize",
            overlayFontSize,
            10,
            20,
            "%.0f",
            unpc::settings->getCommentOverlayFontSize().c_str()
        ))
        {
            unpc::settings->setOverlayFontSize(std::roundf(overlayFontSize));
        }
    }

    if (ImGui::Button("Force Visibility", { labelOffset + fieldWidth, 30 }))
    {
        ++re::forceVisibility;
    }
    tooltip("Forces all characters to be visible for the next frame\n" "Useful for \"resetting\" things after modifying any settings.");

#ifndef BUILDING_ON_GITHUB
    separatorText("Debug");
    re::debugMenu();
#endif

    ImGui::NewLine();
    ImGui::TextUnformatted("Report issues on");
    ImGui::SameLine();
    if (textLink("GitHub"))
    {
        ShellExecuteA(nullptr, "open", "https://github.com/server-imp/UnhideNPCs/", nullptr, nullptr, SW_SHOWNORMAL);
    }
    ImGui::SameLine();

    const std::string version   = fmt::format("v{}", unpc::version::STRING);
    const float       textWidth = ImGui::CalcTextSize(version.c_str()).x;
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - textWidth - (2 * ImGui::GetStyle().ItemSpacing.x));
    ImGui::TextUnformatted(version.c_str());
}

void ui::renderWindow()
{
    if (unpc::exit)
    {
        return;
    }
    if (!unpc::settings || !unpc::settings->loaded())
    {
        return;
    }
    if (!unpc::logger)
    {
        return;
    }

    bool       open       = unpc::settings->getOverlayOpen();
    const bool startState = open;

    if (wasComboPressed({ VK_LMENU, VK_LSHIFT, 'U' }))
    {
        open = !open;
        unpc::settings->setOverlayOpen(open);
        return;
    }

    if (!open)
    {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2 { 0, 0 }); // Auto size
    if (ImGui::Begin("UnhideNPCs [ ALT+SHIFT+U ]", &open, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
    {
        {
            static ImGuiWindow* tmp = nullptr;
            if (tmp)
            {
                ImGui::GetCurrentContext()->MovingWindow = tmp;
                tmp                                      = nullptr;
            }

            auto& io     = ImGui::GetIO();
            auto  window = ImGui::GetCurrentWindow();

            ImVec2 min = ImVec2(0.0f, 0.0f);
            ImVec2 max = io.DisplaySize;

            ImVec2 pos  = window->Pos;
            ImVec2 size = window->Size;

            bool clamped = false;

            if (pos.x < min.x)
            {
                pos.x   = min.x;
                clamped = true;
            }
            if (pos.y < min.y)
            {
                pos.y   = min.y;
                clamped = true;
            }
            if (pos.x + size.x > max.x)
            {
                pos.x   = max.x - size.x;
                clamped = true;
            }
            if (pos.y + size.y > max.y)
            {
                pos.y   = max.y - size.y;
                clamped = true;
            }

            if (clamped && !ImGui::GetCurrentContext()->MovingWindow)
            {
                ImGui::GetCurrentWindow()->Pos = pos;
            }
        }

        renderOptions();
    }
    ImGui::End();

    if (startState != open)
    {
        unpc::settings->setOverlayOpen(open);
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
    {
        return false;
    }
    auto it = combo.begin();

    // All keys except the last must be currently down
    for (const auto end = combo.end() - 1; it != end; ++it)
    {
        if (!(GetAsyncKeyState(*it) & 0x8000))
        {
            return false;
        }
    }

    // The last key must have just been pressed
    return wasKeyPressed(*(combo.end() - 1));
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

uintptr_t ui::onWndProc(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    if (unpc::exit)
    {
        return msg;
    }

    if (!unpc::settings || !unpc::settings->loaded())
    {
        return msg;
    }

    if (!ImGui::GetCurrentContext())
    {
        return msg;
    }

    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE)
    {
        if (unpc::settings->getOverlayOpen() && unpc::settings->getCloseOnEscape())
        {
            unpc::settings->setOverlayOpen(false);
            return 0;
        }
    }

    if (unpc::mode == unpc::EMode::Proxy || unpc::mode == unpc::EMode::Injected)
    {
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
                return 0;
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
                {
                    return 0;
                }
                break;
            default: break;
            }
        }

        if (msg == WM_KEYUP || msg == WM_SYSKEYUP)
        {
            ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
        }
    }

    return msg;
}

void ui::onD3DPresent()
{
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    renderWindow();

    ImGui::Render();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ui::onD3DResizeBuffers(const memory::hooks::D3D11* hk, const bool pre)
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

void ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowPadding     = ImVec2(12, 10);
    style.FramePadding      = ImVec2(6, 4);
    style.CellPadding       = ImVec2(6, 4);
    style.ItemSpacing       = ImVec2(8, 6);
    style.ItemInnerSpacing  = ImVec2(6, 4);
    style.TouchExtraPadding = ImVec2(0, 0);

    style.IndentSpacing = 20;
    style.ScrollbarSize = 14;
    style.GrabMinSize   = 10;

    style.WindowRounding    = 8;
    style.ChildRounding     = 6;
    style.FrameRounding     = 6;
    style.PopupRounding     = 6;
    style.ScrollbarRounding = 6;
    style.GrabRounding      = 6;
    style.TabRounding       = 6;

    style.WindowBorderSize = 1;
    style.ChildBorderSize  = 1;
    style.PopupBorderSize  = 1;
    style.FrameBorderSize  = 0;
    style.TabBorderSize    = 0;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign  = ImVec2(0.5f, 0.5f);

    const auto emeraldBright = ImVec4(0.20f, 0.85f, 0.65f, 1.00f);
    const auto emerald       = ImVec4(0.10f, 0.65f, 0.50f, 1.00f);
    const auto emeraldDim    = ImVec4(0.08f, 0.40f, 0.32f, 1.00f);
    const auto cyanMagic     = ImVec4(0.20f, 0.90f, 0.80f, 1.00f);
    const auto gold          = ImVec4(0.85f, 0.70f, 0.25f, 1.00f);
    const auto goldDim       = ImVec4(0.55f, 0.45f, 0.18f, 1.00f);
    const auto bgDark        = ImVec4(0.07f, 0.08f, 0.10f, 0.85f);
    const auto bgMid         = ImVec4(0.11f * 2, 0.13f * 2, 0.16f * 2, 0.85f);
    const auto bgLight       = ImVec4(0.16f * 2, 0.18f * 2, 0.22f * 2, 0.85f);
    const auto text          = ImVec4(0.85f, 0.90f, 0.88f, 1.00f);
    const auto textDim       = ImVec4(0.45f, 0.55f, 0.52f, 1.00f);

    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]         = text;
    colors[ImGuiCol_TextDisabled] = textDim;

    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg]  = bgDark;
    colors[ImGuiCol_PopupBg]  = bgMid;

    colors[ImGuiCol_Border]       = emeraldDim;
    colors[ImGuiCol_BorderShadow] = ImVec4(0, 0, 0, 0);

    colors[ImGuiCol_FrameBg]        = bgMid;
    colors[ImGuiCol_FrameBgHovered] = emeraldDim;
    colors[ImGuiCol_FrameBgActive]  = emerald;

    colors[ImGuiCol_TitleBg]          = bgDark;
    colors[ImGuiCol_TitleBgActive]    = emeraldDim;
    colors[ImGuiCol_TitleBgCollapsed] = bgDark;

    colors[ImGuiCol_MenuBarBg] = bgMid;

    colors[ImGuiCol_ScrollbarBg]          = bgDark;
    colors[ImGuiCol_ScrollbarGrab]        = emeraldDim;
    colors[ImGuiCol_ScrollbarGrabHovered] = emerald;
    colors[ImGuiCol_ScrollbarGrabActive]  = emeraldBright;

    colors[ImGuiCol_CheckMark] = cyanMagic;

    colors[ImGuiCol_SliderGrab]       = emerald;
    colors[ImGuiCol_SliderGrabActive] = cyanMagic;

    colors[ImGuiCol_Button]        = emeraldDim;
    colors[ImGuiCol_ButtonHovered] = emerald;
    colors[ImGuiCol_ButtonActive]  = emeraldBright;

    colors[ImGuiCol_Header]        = emeraldDim;
    colors[ImGuiCol_HeaderHovered] = emerald;
    colors[ImGuiCol_HeaderActive]  = emeraldBright;

    colors[ImGuiCol_Tab]                = bgMid;
    colors[ImGuiCol_TabHovered]         = emerald;
    colors[ImGuiCol_TabActive]          = emeraldDim;
    colors[ImGuiCol_TabUnfocused]       = bgMid;
    colors[ImGuiCol_TabUnfocusedActive] = emeraldDim;

    colors[ImGuiCol_ResizeGrip]        = emeraldDim;
    colors[ImGuiCol_ResizeGripHovered] = emerald;
    colors[ImGuiCol_ResizeGripActive]  = emeraldBright;

    colors[ImGuiCol_Separator]        = goldDim;
    colors[ImGuiCol_SeparatorHovered] = gold;
    colors[ImGuiCol_SeparatorActive]  = gold;

    colors[ImGuiCol_TableHeaderBg]     = emeraldDim;
    colors[ImGuiCol_TableBorderStrong] = emeraldDim;
    colors[ImGuiCol_TableBorderLight]  = bgLight;
    colors[ImGuiCol_TableRowBg]        = ImVec4(0, 0, 0, 0);
    colors[ImGuiCol_TableRowBgAlt]     = ImVec4(1, 1, 1, 0.03f);

    colors[ImGuiCol_TextSelectedBg] = emeraldDim;

    colors[ImGuiCol_DragDropTarget] = cyanMagic;

    colors[ImGuiCol_NavHighlight]          = emeraldBright;
    colors[ImGuiCol_NavWindowingHighlight] = cyanMagic;
    colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0, 0, 0, 0.5f);
    colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0, 0, 0, 0.6f);
}

bool ui::onD3DStarted(const memory::hooks::D3D11* hk)
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

    io.IniFilename = "./addons/UnhideNPCs/imgui.ini";
    io.LogFilename = "./addons/UnhideNPCs/imgui.log";

    io.Fonts->Clear();
    ImFontConfig cfg {};
    cfg.SizePixels = unpc::settings->getOverlayFontSize();
    io.FontDefault = io.Fonts->AddFontDefault(&cfg);
    io.Fonts->Build();

    ApplyTheme();

    wndProcHook.emplace(hk->hWnd());
    wndProcHook->addCallback(onWndProc);
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

void ui::onD3DShutdown(const memory::hooks::D3D11* hk)
{
    LOG_DBG("OnD3DShutdown");

    if (wndProcHook)
    {
        wndProcHook.reset();
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}
