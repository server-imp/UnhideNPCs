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

    void tooltip(const char* text)
    {
        if (!ImGui::IsItemHovered())
            return;

        ImGui::BeginTooltip();
        ImGui::TextUnformatted(text);
        ImGui::EndTooltip();
    }

    void options()
    {
        bool forceConsole = unpc::settings->getForceConsole();
        if (ImGui::Checkbox("Force Console", &forceConsole))
        {
            unpc::settings->setForceConsole(forceConsole);
            if (!unpc::injected)
            {
                unpc::logger->setConsole(forceConsole);
            }
        }
        tooltip
        (
            "Forces the creation of a console window when set to true.\n"
            "Note: If the console window is exited, then the game will exit as well."
        );

        auto maxDistance = static_cast<int32_t>(unpc::settings->getMaximumDistance());
        ImGui::Text("Max Distance");
        ImGui::SameLine();
        ImGui::PushItemWidth(256);
        if (ImGui::SliderInt("##maxDistance", &maxDistance, 0, 1000, "%d meters"))
        {
            unpc::settings->setMaximumDistance(static_cast<float>(maxDistance));
        }
        ImGui::PopItemWidth();
        tooltip
        (
            "The maximum distance (in meters) at which NPCs will be unhidden.\n"
            "Set to 0 or below for no distance check."
        );
    }

    void onLoad(AddonAPI* aApi)
    {
        APIDefs = aApi;

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(APIDefs->ImguiContext));
        ImGui::SetAllocatorFunctions
            ((void* (*)(size_t, void*))APIDefs->ImguiMalloc, (void(*)(void*, void*))APIDefs->ImguiFree);

        APIDefs->Renderer.Register(ERenderType_OptionsRender, options);
        unpc::entrypoint();
    }

    void onUnload()
    {
        APIDefs->Renderer.Deregister(options);
        unpc::exit = true;
        Sleep(100);
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
