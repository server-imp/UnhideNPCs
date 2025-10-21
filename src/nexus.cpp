#include "nexus.hpp"
#include "unpc.hpp"
#include "imgui.h"

namespace nexus
{
    AddonDefinition AddonDef{};
    AddonAPI*       APIDefs{};

    bool isNexus()
    {
        if (APIDefs)
            return true;

        if (util::shmExists(fmt::format("DL_NEXUS_LINK_{}", GetCurrentProcessId())))
            return true;

        return false;
    }

    namespace ui
    {
        constexpr float LABEL_OFFSET = 200.0f;
        constexpr float FIELD_WIDTH  = 256.0f;

        void tooltip(const char* text)
        {
            if (!ImGui::IsItemHovered())
                return;

            ImGui::BeginTooltip();
            ImGui::TextUnformatted(text);
            ImGui::EndTooltip();
        }

        bool labeled_checkbox(const char* label, const char* id, bool& value, const char* tip)
        {
            const float initialCursorX = ImGui::GetCursorPosX();
            ImGui::Text("%s", label);
            tooltip(tip);
            ImGui::SameLine();
            ImGui::SetCursorPosX(initialCursorX + LABEL_OFFSET);
            return ImGui::Checkbox(id, &value);
        }

                bool labeled_combo
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

        bool labeled_slider_int
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
    }

    const char* ranks[] = {"Normal", "Veteran", "Elite", "Champion", "Legendary"};
    const char* modes[] = {"Both", "Attackable", "Non-Attackable"};

    void options()
    {
        if (unpc::exit)
            return;

        if (!unpc::settings || !unpc::settings->loaded())
            return;
        if (!unpc::logger)
            return;

        if (auto forceConsole = unpc::settings->getForceConsole(); ui::labeled_checkbox
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

        if (auto playerOwned = unpc::settings->getPlayerOwned(); ui::labeled_checkbox
            (
                "Player Owned",
                "##PlayerOwned",
                playerOwned,
                "NPCs that are owned by players (pets, clones, minis etc) will also be unhidden."
            ))
        {
            unpc::settings->setPlayerOwned(playerOwned);
        }

        if (auto minRank = unpc::settings->getMinimumRank(); ui::labeled_combo
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

        if (auto attackable = unpc::settings->getAttackable(); ui::labeled_combo
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

        if (auto maxDistance = static_cast<int32_t>(unpc::settings->getMaximumDistance()); ui::labeled_slider_int
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

    void onLoad(AddonAPI* aApi)
    {
        APIDefs = aApi;

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(APIDefs->ImguiContext));
        ImGui::SetAllocatorFunctions
            ((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);

        APIDefs->Renderer.Register(ERenderType_OptionsRender, options);

        unpc::start();
    }

    void onUnload()
    {
        APIDefs->Renderer.Deregister(options);

        unpc::stop();

        FreeLibrary(unpc::hModule);
    }
}

nexus::AddonDefinition* GetAddonDef()
{
    nexus::AddonDef.Signature        = unpc::signature;
    nexus::AddonDef.APIVersion       = NEXUS_API_VERSION;
    nexus::AddonDef.Name             = "UnhideNPCs";
    nexus::AddonDef.Version.Major    = unpc::version::year;
    nexus::AddonDef.Version.Minor    = unpc::version::month;
    nexus::AddonDef.Version.Build    = unpc::version::day;
    nexus::AddonDef.Version.Revision = unpc::version::build;
    nexus::AddonDef.Author           = "server-imp";
    nexus::AddonDef.Description      = "Stops the game from hiding NPCs/Monsters etc";
    nexus::AddonDef.Load             = nexus::onLoad;
    nexus::AddonDef.Unload           = nexus::onUnload;
    nexus::AddonDef.Flags            = nexus::EAddonFlags::EAddonFlags_IsVolatile;

    nexus::AddonDef.Provider   = nexus::EUpdateProvider_GitHub;
    nexus::AddonDef.UpdateLink = "https://github.com/server-imp/UnhideNPCs";

    return &nexus::AddonDef;
}
