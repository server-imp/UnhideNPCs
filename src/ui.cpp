#include "ui.hpp"
#include "imgui.h"
#include "unpc.hpp"

constexpr float LABEL_OFFSET = 200.0f;
constexpr float FIELD_WIDTH  = 256.0f;

void ui::tooltip(const char* text)
{
    if (!ImGui::IsItemHovered())
        return;

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
    ImGui::SetCursorPosX(initialCursorX + LABEL_OFFSET);
    return ImGui::Checkbox(id, &value);
}

bool ui::combo
(const char* label, const char* id, int& value, const char* const* items, const int count, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + LABEL_OFFSET);
    ImGui::PushItemWidth(FIELD_WIDTH);
    const bool result = ImGui::Combo(id, &value, items, count);
    ImGui::PopItemWidth();
    return result;
}

bool ui::sliderInt
(const char* label, const char* id, int& value, const int min, const int max, const char* fmt, const char* tip)
{
    const float initialCursorX = ImGui::GetCursorPosX();
    ImGui::Text("%s", label);
    tooltip(tip);
    ImGui::SameLine();
    ImGui::SetCursorPosX(initialCursorX + LABEL_OFFSET);
    ImGui::PushItemWidth(FIELD_WIDTH);
    const bool result = ImGui::SliderInt(id, &value, min, max, fmt);
    ImGui::PopItemWidth();
    return result;
}

const char* ranks[] = {"Normal", "Veteran", "Elite", "Champion", "Legendary"};
const char* modes[] = {"Both", "Attackable", "Non-Attackable"};

void ui::renderOptions()
{
    if (!unpc::mumbleLink || unpc::mumbleLink->getContext().IsCompetitiveMode())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
        ImGui::Text("Disabled in Competitive");
        ImGui::PopStyleColor();
        return;
    }

    if (auto forceConsole = unpc::settings->getForceConsole(); ui::checkbox
        (
            "Force Console",
            "##ForceConsole",
            forceConsole,
            "Forces the creation of a console window when set to true.\n"
            "Note: If the console window is exited, then the game will exit as well."
        ))
    {
        unpc::settings->setForceConsole(forceConsole);
        if (!unpc::injected)
        {
            unpc::logger->setConsole(forceConsole);
        }
    }

    if (auto playerOwned = unpc::settings->getPlayerOwned(); ui::checkbox
        (
            "Player Owned",
            "##PlayerOwned",
            playerOwned,
            "NPCs that are owned by players (pets, clones, minis etc) will also be unhidden."
        ))
    {
        unpc::settings->setPlayerOwned(playerOwned);
    }

    if (auto minRank = unpc::settings->getMinimumRank(); ui::combo
        (
            "Minimum Rank",
            "##minRank",
            minRank,
            ranks,
            IM_ARRAYSIZE(ranks),
            "Only NPCs that have at least this rank gets unhidden."
        ))
    {
        unpc::settings->setMinimumRank(minRank);
    }

    if (auto attackable = unpc::settings->getAttackable(); ui::combo
        (
            "Attackable",
            "##attackable",
            attackable,
            modes,
            IM_ARRAYSIZE(modes),
            "Only NPCs that match this gets unhidden."
        ))
    {
        unpc::settings->setAttackable(attackable);
    }

    if (auto hidePlayers = unpc::settings->getHidePlayers(); ui::checkbox
        (
            "Hide Players",
            "##HidePlayers",
            hidePlayers,
            "Players will be hidden when this is ticked, useful for boosting performance.\n"
            "Their names are still visible, and you can still target them"
        ))
    {
        unpc::settings->setHidePlayers(hidePlayers);
    }

    if (auto maxDistance = static_cast<int32_t>(unpc::settings->getMaximumDistance()); ui::sliderInt
        (
            "Max Distance",
            "##maxDistance",
            maxDistance,
            0,
            1000,
            "%d meters",
            "NPCs within this distance will be unhidden. (0=no distance check)"
        ))
    {
        unpc::settings->setMaximumDistance(static_cast<float>(maxDistance));
    }
}

void ui::renderWindow(const uint32_t not_charsel_or_loading, const uint32_t hide_if_combat_or_ooc)
{
    if (unpc::exit)
        return;
    if (!unpc::settings || !unpc::settings->loaded())
        return;
    if (!unpc::logger)
        return;

    if (not_charsel_or_loading == 0 || hide_if_combat_or_ooc == 1)
        return;

    bool       open       = unpc::settings->getArcDPS_UIOpen();
    const bool startState = open;

    if (wasComboPressed({VK_LMENU, VK_LSHIFT, 'U'}))
    {
        open = !open;
        unpc::settings->setArcDPS_UIOpen(open);
        return;
    }

    if (!open)
        return;

    ImGui::SetNextWindowSize(ImVec2{0,0}); // Auto size
    if (ImGui::Begin("UnhideNPCs [ ALT+SHIFT+U ]", &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
    {
        renderOptions();
        ImGui::End();
    }

    if (wasComboPressed({VK_LMENU, VK_LSHIFT, 'U'}))
    {
        open = !open;
    }

    if (startState != open)
    {
        unpc::settings->setArcDPS_UIOpen(open);
    }
}

static SHORT lastState[256] = {0};

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

    // All keys except last must be currently down
    for (const auto end = combo.end() - 1; it != end; ++it)
    {
        if (!(GetAsyncKeyState(*it) & 0x8000))
            return false;
    }

    // Last key must have just been pressed
    return wasKeyPressed(*(combo.end() - 1));
}
