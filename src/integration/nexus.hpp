#ifndef UNHIDENPCS_NEXUS_HPP
#define UNHIDENPCS_NEXUS_HPP
#pragma once

namespace logging {
    struct LogEntry;
}

struct MumbleLink;

namespace nexus
{
#include "Nexus.h"
    extern AddonDefinition AddonDef;
    extern AddonAPI*       APIDefs;

    bool isNexus();

    MumbleLink* getMumbleLink();

    void logCallback(const logging::LogEntry& entry);

    void options();

    void onLoad(AddonAPI* aApi);

    void onUnload();
}

extern "C" __declspec(dllexport) nexus::AddonDefinition* GetAddonDef();

#endif //UNHIDENPCS_NEXUS_HPP
