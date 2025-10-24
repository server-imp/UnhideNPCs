#ifndef UNHIDENPCS_UNPC_HPP
#define UNHIDENPCS_UNPC_HPP
#pragma once
#include "fw/logger.hpp"
#include "fw/util.hpp"
#include "fw/memory/memory.hpp"
#include "gw2.hpp"
#include "MumbleLink.hpp"
#include "settings.hpp"

namespace unpc
{
    enum class eMode : int32_t
    {
        Unknown = 0,
        Proxy = 1,
        Injected = 2,
        Nexus = 3,
        ArcDPS = 4
    };

    constexpr int32_t signature = 1817724315;

    namespace version
    {
#define YEAR  2025
#define MONTH 10
#define DAY   24
#define BUILD 1

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define VERSION_STRING STR(YEAR) "." STR(MONTH) "." STR(DAY) "." STR(BUILD)
        constexpr int16_t year  = YEAR;
        constexpr int16_t month = MONTH;
        constexpr int16_t day   = DAY;
        constexpr int16_t build = BUILD;

        constexpr char string[] = VERSION_STRING;
#undef VERSION_STRING
#undef BUILD
#undef DAY
#undef MONTH
#undef YEAR
#undef STR
#undef STR_HELPER
    }

    extern eMode mode;

    extern bool nexusPresent;
    extern bool arcDpsPresent;
    extern bool injected;
    extern bool exit;

    extern std::optional<logging::Logger> logger;
    extern std::optional<Settings>        settings;

    extern HANDLE      hMutex;
    extern HMODULE     hModule;
    extern HANDLE      hThread;
    extern std::string proxyModuleName;
    extern HMODULE     hProxyModule;

    extern MumbleLink* mumbleLink;
    extern int32_t* loadingScreenActive;

    void start();

    void stop();

    void entrypoint();
}

#endif //UNHIDENPCS_UNPC_HPP
