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

float ui::labelOffset = -1;
float ui::fieldWidth  = -1;

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

bool ui::sliderInt(
    const char*   label,
    const char*   id,
    int32_t&      value,
    const int32_t min,
    const int32_t max,
    const char*   fmt,
    const char*   tip
)
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

bool ui::sliderFloat(
    const char* label,
    const char* id,
    float&      value,
    const float min,
    const float max,
    const char* fmt,
    const char* tip
)
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

bool ui::textbox(const char* tag, char* buffer, const size_t bufferSize)
{
    ImGui::PushItemWidth(labelOffset + fieldWidth);
    auto result = ImGui::InputText(tag, buffer, bufferSize);
    ImGui::PopItemWidth();
    return result;
}

bool ui::textboxbutton(const char* tag, const char* hint, char* buffer, const size_t bufferSize, const char* buttonText)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    const auto  width          = labelOffset - ImGui::GetStyle().ItemSpacing.x;
    ImGui::PushItemWidth(width);
    ImGui::InputTextWithHint(tag, hint, buffer, bufferSize);
    ImGui::PopItemWidth();
    tooltip(hint);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + width + ImGui::GetStyle().ItemSpacing.x);
    if (ImGui::Button(buttonText, { fieldWidth, 30 }))
    {
        return true;
    }

    return false;
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
        ImGui::GetWindowDrawList()->AddLine(
            ImVec2(pos.x, pos.y + textSize.y),
            ImVec2(pos.x + textSize.x, pos.y + textSize.y),
            color,
            1.0f
        );
    }

    return clicked;
}

struct CheckboxGroupEntry
{
    const char* label {};
    const char* tooltip {};
    bool*       value {};
    bool*       changed {};

    CheckboxGroupEntry(const char* label, const char* tooltip, bool* value, bool* changed) : label(label),
        tooltip(tooltip), value(value), changed(changed) {}
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
        const float x = initialCursorPos.x + (isOdd ? 2.0f : 1.0f) * ui::labelOffset;

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

const char* instanceModes[] = { "Always On", "Disabled in instances", "Instances only" };

std::vector<const char*> profileOptions {};

void renderProfileOptions()
{
    auto& settings = *unpc::settings;
    auto& profile  = settings.profile();

    static char profileNameBuffer[64] { 0 };
    int         selected = unpc::settings->ActiveProfile.get();

    if (profileOptions.empty())
    {
        for (const auto& child : unpc::settings->children())
        {
            profileOptions.push_back(child->name().c_str());
        }

        strcpy_s(profileNameBuffer, sizeof(profileNameBuffer), unpc::settings->children()[selected]->name().c_str());
    }

    if (ui::combo("Profile", "##profile", selected, profileOptions.data(), profileOptions.size(), "The profile to use"))
    {
        unpc::settings->ActiveProfile.set(selected);
        profileOptions.clear();
        ++re::forceVisibility;
    }

    if (ui::textboxbutton(
        "##profileName",
        "New Profile Name",
        profileNameBuffer,
        std::size(profileNameBuffer),
        "Rename Profile"
    ))
    {
        if (unpc::settings->ActiveProfile.get() > 0 && profileNameBuffer[0])
        {
            unpc::settings->renameProfile(
                unpc::settings->children()[unpc::settings->ActiveProfile.get()]->name(),
                profileNameBuffer
            );
            profileOptions.clear();
            ++re::forceVisibility;
        }
    }

    if (ui::button("New Profile"))
    {
        const char* newName = profileNameBuffer[0] ? profileNameBuffer : "Profile";
        int         i       = 0;
        auto        name    = fmt::format("{}", newName);
        while (unpc::settings->profileExists(name))
        {
            ++i;
            name = fmt::format("{} {}", newName, i);
        }
        unpc::settings->addProfile(name);
        unpc::settings->ActiveProfile.set(unpc::settings->children().size() - 1);
        profileOptions.clear();
        ++re::forceVisibility;
    }

    if (ui::button("Remove Profile"))
    {
        int i = unpc::settings->ActiveProfile.get();
        if (i > 0)
        {
            unpc::settings->ActiveProfile.set(i - 1);
            unpc::settings->removeProfile(unpc::settings->children()[i]->name());
            profileOptions.clear();
            ++re::forceVisibility;
        }
    }
    ui::tooltip("Remove current profile");
}

void ui::renderOptions()
{
    if (!unpc::settings || !unpc::settings->loaded())
    {
        return;
    }

    auto& settings = *unpc::settings;
    auto& profile  = settings.profile();

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

    renderProfileOptions();

    separatorText("Show");

    bool unhideNpcsChanged {}, unhidePlayersChanged {}, playerOwnedChanged {}, alwaysShowTargetChanged {};
    bool unhideNpcs       = profile.UnhideNPCs.get();
    bool unhidePlayers    = profile.UnhidePlayers.get();
    bool playerOwned      = profile.PlayerOwned.get();
    bool alwaysShowTarget = profile.AlwaysShowTarget.get();

    if (checkboxGroup(
        "Unhide",
        "The selected will be unhidden",
        std::vector<CheckboxGroupEntry> {
            { "NPCs", profile.UnhideNPCs.description().c_str(), &unhideNpcs, &unhideNpcsChanged },
            { "Players", profile.UnhidePlayers.description().c_str(), &unhidePlayers, &unhidePlayersChanged },
            { "Player-Owned", profile.PlayerOwned.description().c_str(), &playerOwned, &playerOwnedChanged },
            { "Target", profile.AlwaysShowTarget.description().c_str(), &alwaysShowTarget, &alwaysShowTargetChanged }
        }
    ))
    {
        if (unhideNpcsChanged)
        {
            profile.UnhideNPCs.set(unhideNpcs);
        }
        if (unhidePlayersChanged)
        {
            profile.UnhidePlayers.set(unhidePlayers);
        }
        if (playerOwnedChanged)
        {
            profile.PlayerOwned.set(playerOwned);
        }
        if (alwaysShowTargetChanged)
        {
            profile.AlwaysShowTarget.set(alwaysShowTarget);
        }
    }

    bool unhideLowQuality = profile.UnhideLowQuality.get();
    ImGui::NewLine();
    if (ui::checkbox(
        "Low Quality Models",
        "##unhideLowQualityModels",
        unhideLowQuality,
        profile.UnhideLowQuality.description().c_str()
    ))
    {
        profile.UnhideLowQuality.set(unhideLowQuality);
    }
    if (auto maxDistance = static_cast<int32_t>(profile.MaximumDistance.get()); ui::sliderInt(
        "Max Distance",
        "##maxDistance",
        maxDistance,
        0,
        1000,
        maxDistance == 0 ? "Unlimited" : "%d meters",
        profile.MaximumDistance.description().c_str()
    ))
    {
        profile.MaximumDistance.set(static_cast<float>(maxDistance));
    }

    int minimumRank = profile.MinimumRank.get();
    if (ui::combo(
        "Minimum Rank",
        "##minRank",
        minimumRank,
        ranks,
        IM_ARRAYSIZE(ranks),
        profile.MinimumRank.description().c_str()
    ))
    {
        profile.MinimumRank.set(minimumRank);
    }

    int attackable = profile.Attackable.get();
    if (ui::combo(
        "Attackable",
        "##attackable",
        attackable,
        modes,
        IM_ARRAYSIZE(modes),
        profile.Attackable.description().c_str()
    ))
    {
        profile.Attackable.set(attackable);
    }

    ui::separatorText("Hide");
    bool hideAllPlayersChanged {}, hideBlockedPlayersChanged {}, hideNonGroupPlayersChanged {}, hideStrangersChanged {},
         hideNonGuildPlayersChanged {}, hideNonFriendPlayersChanged {};
    bool hidePlayers = profile.HidePlayers.get();
    bool hideBlockedPlayers = profile.HideBlockedPlayers.get();
    bool hideNonGroupMembers = profile.HideNonGroupMembers.get();
    bool hideStrangers = profile.HideStrangers.get();
    bool hideNonGuildMembers = profile.HideNonGuildMembers.get();
    bool hideNonFriends = profile.HideNonFriends.get();

    if (checkboxGroup(
        "Players",
        "Players that match the selected critera will be hidden",
        std::vector<CheckboxGroupEntry> {
            { "All", profile.HidePlayers.description().c_str(), &hidePlayers, &hideAllPlayersChanged },
            {
                "Blocked",
                profile.HideBlockedPlayers.description().c_str(),
                &hideBlockedPlayers,
                &hideBlockedPlayersChanged
            },
            {
                "Non-Group",
                profile.HideNonGroupMembers.description().c_str(),
                &hideNonGroupMembers,
                &hideNonGroupPlayersChanged
            },
            { "Strangers", profile.HideStrangers.description().c_str(), &hideStrangers, &hideStrangersChanged },
            {
                "Non-Guild",
                profile.HideNonGuildMembers.description().c_str(),
                &hideNonGuildMembers,
                &hideNonGuildPlayersChanged
            },
            {
                "Non-Friends",
                profile.HideNonFriends.description().c_str(),
                &hideNonFriends,
                &hideNonFriendPlayersChanged
            }
        }
    ))
    {
        if (hideAllPlayersChanged)
        {
            profile.HidePlayers.set(hidePlayers);
        }
        if (hideBlockedPlayersChanged)
        {
            profile.HideBlockedPlayers.set(hideBlockedPlayers);
        }
        if (hideNonGroupPlayersChanged)
        {
            profile.HideNonGroupMembers.set(hideNonGroupMembers);
        }
        if (hideStrangersChanged)
        {
            profile.HideStrangers.set(hideStrangers);
        }
        if (hideNonGuildPlayersChanged)
        {
            profile.HideNonGuildMembers.set(hideNonGuildMembers);
        }
        if (hideNonFriendPlayersChanged)
        {
            profile.HideNonFriends.set(hideNonFriends);
        }
    }
    ImGui::NewLine();

    bool hideAllPlayerOwnedChanged {}, hideBlockedPlayerOwnedChanged {}, hideNonGroupPlayerOwnedChanged {},
         hideStrangersOwnedChanged {}, hideNonGuildPlayerOwnedChanged {}, hideNonFriendPlayerOwnedChanged {},
         hideMyOwnedChanged {};
    bool hidePlayerOwned          = profile.HidePlayerOwned.get();
    bool hideBlockedPlayersOwned  = profile.HideBlockedPlayersOwned.get();
    bool hideNonGroupMembersOwned = profile.HideNonGroupMembersOwned.get();
    bool hideStrangersOwned       = profile.HideStrangersOwned.get();
    bool hideNonGuildMembersOwned = profile.HideNonGuildMembersOwned.get();
    bool hideNonFriendsOwned      = profile.HideNonFriendsOwned.get();
    bool hidePlayerOwnedSelf      = profile.HidePlayerOwnedSelf.get();

    if (checkboxGroup(
        "Player-Owned",
        "Characters that are owned by the selected type of players will be hidden",
        std::vector<CheckboxGroupEntry> {
            { "All", profile.HidePlayerOwned.description().c_str(), &hidePlayerOwned, &hideAllPlayerOwnedChanged },
            {
                "Blocked",
                profile.HideBlockedPlayersOwned.description().c_str(),
                &hideBlockedPlayersOwned,
                &hideBlockedPlayerOwnedChanged
            },
            {
                "Non-Group",
                profile.HideNonGroupMembersOwned.description().c_str(),
                &hideNonGroupMembersOwned,
                &hideNonGroupPlayerOwnedChanged
            },
            {
                "Strangers",
                profile.HideStrangersOwned.description().c_str(),
                &hideStrangersOwned,
                &hideStrangersOwnedChanged
            },
            {
                "Non-Guild",
                profile.HideNonGuildMembersOwned.description().c_str(),
                &hideNonGuildMembersOwned,
                &hideNonGuildPlayerOwnedChanged
            },
            {
                "Non-Friends",
                profile.HideNonFriendsOwned.description().c_str(),
                &hideNonFriendsOwned,
                &hideNonFriendPlayerOwnedChanged
            },
            { "Mine", profile.HidePlayerOwnedSelf.description().c_str(), &hidePlayerOwnedSelf, &hideMyOwnedChanged }
        }
    ))
    {
        if (hideAllPlayerOwnedChanged)
        {
            profile.HidePlayerOwned.set(hidePlayerOwned);
        }
        if (hideBlockedPlayerOwnedChanged)
        {
            profile.HideBlockedPlayersOwned.set(hideBlockedPlayersOwned);
        }
        if (hideNonGroupPlayerOwnedChanged)
        {
            profile.HideNonGroupMembersOwned.set(hideNonGroupMembersOwned);
        }
        if (hideStrangersOwnedChanged)
        {
            profile.HideStrangersOwned.set(hideStrangersOwned);
        }
        if (hideNonGuildPlayerOwnedChanged)
        {
            profile.HideNonGuildMembersOwned.set(hideNonGuildMembersOwned);
        }
        if (hideNonFriendPlayerOwnedChanged)
        {
            profile.HideNonFriendsOwned.set(hideNonFriendsOwned);
        }
        if (hideMyOwnedChanged)
        {
            profile.HidePlayerOwnedSelf.set(hidePlayerOwnedSelf);
        }
    }
    ImGui::NewLine();

    bool hidePlayersInCombatChanged {}, hidePlayerOwnedInCombatChanged {};
    bool hidePlayersInCombat     = profile.HidePlayersInCombat.get();
    bool hidePlayerOwnedInCombat = profile.HidePlayerOwnedInCombat.get();
    if (checkboxGroup(
        "In Combat",
        "When in combat, hide the following",
        std::vector<CheckboxGroupEntry> {
            {
                "Players",
                profile.HidePlayersInCombat.description().c_str(),
                &hidePlayersInCombat,
                &hidePlayersInCombatChanged
            },
            {
                "Player-Owned",
                profile.HidePlayerOwnedInCombat.description().c_str(),
                &hidePlayerOwnedInCombat,
                &hidePlayerOwnedInCombatChanged
            }
        }
    ))
    {
        if (hidePlayersInCombatChanged)
        {
            profile.HidePlayersInCombat.set(hidePlayersInCombat);
        }
        if (hidePlayerOwnedInCombatChanged)
        {
            profile.HidePlayerOwnedInCombat.set(hidePlayerOwnedInCombat);
        }
    }

    ImGui::NewLine();
    int32_t maxPlayersVisible = profile.MaxPlayersVisible.get();
    if (ui::sliderInt(
        "Max Players",
        "##MaxPlayersVisible",
        maxPlayersVisible,
        0,
        250,
        maxPlayersVisible == 0 ? "Unlimited" : "%d",
        profile.MaxPlayersVisible.description().c_str()
    ))
    {
        profile.MaxPlayersVisible.set(maxPlayersVisible);
    }

    int32_t maxPlayerOwnedVisible = profile.MaxPlayerOwnedVisible.get();
    if (ui::sliderInt(
        "Max Player-Owned",
        "##MaxPlayerOwnedVisible",
        maxPlayerOwnedVisible,
        0,
        100,
        maxPlayerOwnedVisible == 0 ? "Unlimited" : "%d",
        profile.MaxPlayerOwnedVisible.description().c_str()
    ))
    {
        profile.MaxPlayerOwnedVisible.set(maxPlayerOwnedVisible);
    }

    int32_t maxNpcs = profile.MaxNpcs.get();
    if (ui::sliderInt(
        "Max NPCs",
        "##maxNpcs",
        maxNpcs,
        0,
        1000,
        maxNpcs == 0 ? "Unlimited" : "%d",
        profile.MaxNpcs.description().c_str()
    ))
    {
        profile.MaxNpcs.set(maxNpcs);
    }
    ImGui::NewLine();

    int instanceBehaviour = profile.InstanceBehaviour.get();
    if (ui::combo(
        "Instance Behaviour",
        "##instances",
        instanceBehaviour,
        instanceModes,
        IM_ARRAYSIZE(instanceModes),
        profile.InstanceBehaviour.description().c_str()
    ))
    {
        profile.InstanceBehaviour.set(instanceBehaviour);
    }
    separatorText("Misc");
    std::vector<CheckboxGroupEntry> entries {};
    bool forceConsoleChanged {}, loadScreenBoostChanged {}, disableOverlayChanged {}, closeOnEscapeChanged {};
    bool forceConsole = settings.ForceConsole.get();
    bool loadScreenBoost = settings.LoadScreenBoost.get();
    bool closeOnEscape = settings.CloseOnEscape.get();

    entries.emplace_back("Console", settings.ForceConsole.description().c_str(), &forceConsole, &forceConsoleChanged);
    entries.emplace_back(
        "Loading Boost",
        settings.LoadScreenBoost.description().c_str(),
        &loadScreenBoost,
        &loadScreenBoostChanged
    );
    entries.emplace_back(
        "Esc To Close",
        settings.CloseOnEscape.description().c_str(),
        &closeOnEscape,
        &closeOnEscapeChanged
    );

    if (unpc::mode == unpc::EMode::Nexus)
    {
        goto footer;
    }

footer:
    if (checkboxGroup(nullptr, nullptr, entries))
    {
        if (forceConsoleChanged)
        {
            settings.ForceConsole.set(forceConsole);
            unpc::logger->setConsole(forceConsole);
        }
        if (loadScreenBoostChanged)
        {
            settings.LoadScreenBoost.set(loadScreenBoost);
        }
        if (closeOnEscapeChanged)
        {
            settings.CloseOnEscape.set(closeOnEscape);
        }
    }
    ImGui::NewLine();

    if (unpc::mode == unpc::EMode::Proxy || unpc::mode == unpc::EMode::Injected)
    {
        float overlayFontSize = settings.OverlayFontSize.get();
        if (ui::sliderFloat(
            "Font Size",
            "##OverlayFontSize",
            overlayFontSize,
            10,
            20,
            "%.0f",
            settings.OverlayFontSize.description().c_str()
        ))
        {
            settings.OverlayFontSize.set(std::roundf(overlayFontSize));
        }
    }

    if (ImGui::Button("Force Visibility", { labelOffset + fieldWidth, 30 }))
    {
        ++re::forceVisibility;
    }
    tooltip(
        "Forces all characters to be visible for the next frame\n"
        "Useful for \"resetting\" things after modifying any settings."
    );

    if (ImGui::TreeNodeEx("Hotkeys", ImGuiTreeNodeFlags_NoTreePushOnOpen))
    {
        unpc::hotkeyManager.renderHotkeys();
        ImGui::TreePop();
    }
    else
    {
        unpc::hotkeyManager.stopCapturing();
    }

#ifndef BUILDING_ON_GITHUB
    ui::separatorText("Debug");
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

    bool       open       = unpc::settings->OverlayOpen.get();
    const bool startState = open;

    if (wasComboPressed({ VK_LMENU, VK_LSHIFT, 'U' }))
    {
        open = !open;
        unpc::settings->OverlayOpen.set(open);
        if (!open)
        {
            unpc::hotkeyManager.stopCapturing();
        }
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
        unpc::settings->OverlayOpen.set(open);
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

    if (!unpc::settings)
    {
        return msg;
    }

    if (!ImGui::GetCurrentContext())
    {
        return msg;
    }

    if (!unpc::hotkeyManager.onWndProc(hWnd, msg, wParam, lParam))
    {
        return 0;
    }

    if (msg == WM_KEYDOWN && wParam == VK_ESCAPE)
    {
        if (unpc::settings->OverlayOpen.get() && unpc::settings->CloseOnEscape.get())
        {
            unpc::settings->OverlayOpen.set(false);
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

u32 ui::onWndProcNexus(HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
    return onWndProc(hWnd, msg, wParam, lParam);
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
    cfg.SizePixels = unpc::settings->OverlayFontSize.get();
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
