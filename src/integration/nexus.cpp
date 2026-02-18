#include "nexus.hpp"
#include "../unpc.hpp"
#include "../ui.hpp"
#include "imgui.h"

namespace nexus
{
    AddonDefinition AddonDef {};
    AddonAPI*       APIDefs {};

    bool isNexus()
    {
        if (APIDefs)
        {
            return true;
        }

        if (util::shmExists(fmt::format("DL_NEXUS_LINK_{}", GetCurrentProcessId())))
        {
            return true;
        }

        return false;
    }

    MumbleLink* getMumbleLink()
    {
        if (!APIDefs)
        {
            return nullptr;
        }

        return static_cast<MumbleLink*>(APIDefs->DataLink.Get("DL_MUMBLE_LINK"));
    }

    void logCallback(const logging::LogEntry& entry)
    {
        if (!APIDefs)
        {
            return;
        }

        ELogLevel level;
        switch (entry.level)
        {
        case logging::LogLevel::Debug: level = ELogLevel_DEBUG;
            break;
        case logging::LogLevel::Info: level = ELogLevel_INFO;
            break;
        case logging::LogLevel::Warning: level = ELogLevel_WARNING;
            break;
        case logging::LogLevel::Error: level = ELogLevel_CRITICAL;
            break;
        default: level = ELogLevel_OFF;
            break;
        }

        APIDefs->Log(level, "UnhideNPCs", entry.message.c_str());
    }

    void options()
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
        if (unpc::hProxyModule || unpc::injected)
        {
            return;
        }

        ui::renderOptions();
    }

    void onLoad(AddonAPI* aApi)
    {
        if (unpc::hProxyModule || unpc::injected)
        {
            return;
        }

        APIDefs = aApi;

        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(aApi->ImguiContext));
        const auto a = reinterpret_cast<void*(*)(size_t, void*)>(reinterpret_cast<std::uintptr_t>(aApi->ImguiMalloc));
        const auto f = reinterpret_cast<void(*)(void*, void*)>(reinterpret_cast<std::uintptr_t>(aApi->ImguiFree));
        ImGui::SetAllocatorFunctions(a, f);

        APIDefs->Renderer.Register(ERenderType_OptionsRender, options);

        unpc::start();
    }

    void onUnload()
    {
        if (unpc::hProxyModule || unpc::injected)
        {
            return;
        }

        APIDefs->Renderer.Deregister(options);

        unpc::stop();

        FreeLibrary(unpc::hModule);
    }
}

nexus::AddonDefinition* GetAddonDef()
{
    nexus::AddonDef.Signature        = unpc::SIGNATURE;
    nexus::AddonDef.APIVersion       = NEXUS_API_VERSION;
    nexus::AddonDef.Name             = "UnhideNPCs";
    nexus::AddonDef.Version.Major    = unpc::version::YEAR;
    nexus::AddonDef.Version.Minor    = unpc::version::MONTH;
    nexus::AddonDef.Version.Build    = unpc::version::DAY;
    nexus::AddonDef.Version.Revision = unpc::version::BUILD;
    nexus::AddonDef.Author           = "server-imp";
    nexus::AddonDef.Description      = "Stops the game from hiding NPCs/Monsters etc";
    nexus::AddonDef.Load             = nexus::onLoad;
    nexus::AddonDef.Unload           = nexus::onUnload;
    nexus::AddonDef.Flags            = nexus::EAddonFlags::EAddonFlags_IsVolatile;

    nexus::AddonDef.Provider   = nexus::EUpdateProvider_GitHub;
    nexus::AddonDef.UpdateLink = "https://github.com/server-imp/UnhideNPCs";

    return &nexus::AddonDef;
}
