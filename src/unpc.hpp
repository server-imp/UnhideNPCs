#ifndef UNHIDENPCS_UNPC_HPP
#define UNHIDENPCS_UNPC_HPP
#pragma once
#include "fw/logger.hpp"
#include "fw/util.hpp"
#include "fw/memory/memory.hpp"
#include "gw2.hpp"
#include "settings.hpp"

namespace unpc
{
    extern int32_t signature;

    namespace version
    {
        extern int16_t year;
        extern int16_t month;
        extern int16_t day;
        extern int16_t build;
    }

    extern bool loadedByNexus;
    extern bool exit;
    extern bool injected;

    extern std::optional<logging::Logger> logger;
    extern std::optional<Settings>        settings;

    extern HANDLE      hMutex;
    extern HMODULE     hModule;
    extern HANDLE      hThread;
    extern std::string proxyModuleName;
    extern HMODULE     hProxyModule;

    void start();
    void stop();
    void entrypoint();
}

#endif //UNHIDENPCS_UNPC_HPP
