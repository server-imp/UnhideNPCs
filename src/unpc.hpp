#ifndef UNHIDENPCS_UNPC_HPP
#define UNHIDENPCS_UNPC_HPP
#pragma once
#include "fw/logger.hpp"
#include "fw/util.hpp"
#include "fw/memory/memory.hpp"
#include "gw2.hpp"
#include "MumbleLink.hpp"
#include "settings.hpp"
#include "version.hpp"

namespace unpc
{
    enum class eMode : int32_t
    {
        Unknown  = 0,
        Proxy    = 1,
        Injected = 2,
        Nexus    = 3,
        ArcDPS   = 4
    };

    constexpr int32_t signature = 1817724315;

    extern eMode mode;

    extern bool nexusPresent;
    extern bool arcDpsPresent;
    extern bool injected;
    extern bool exit;

    extern std::optional<logging::Logger> logger;
    extern std::optional<Settings>        settings;
    extern std::optional<memory::detour>  npcHook;

    extern HANDLE      hMutex;
    extern HMODULE     hModule;
    extern HANDLE      hThread;
    extern std::string proxyModuleName;
    extern HMODULE     hProxyModule;

    extern MumbleLink* mumbleLink;
    extern int32_t*    loadingScreenActive;

    extern uint32_t numPlayersVisible;
    extern uint32_t numPlayerOwnedVisible;

    // This function may be called second, returning true means the character will be forced invisible
    bool shouldHide
    (
        bool    isPlayer,
        bool    isPlayerOwned,
        bool    isOwnerLocalPlayer,
        bool    isTarget,
        bool    isAttackable,
        uint8_t rank,
        float   distance,
        float   maxDistance
    );

    // This function is called first, returning true means the character will be forced visible
    // if returning false, it will go on to call shouldHide
    bool shouldShow
    (
        bool    isPlayer,
        bool    isPlayerOwned,
        bool    isOwnerLocalPlayer,
        bool    isTarget,
        bool    isAttackable,
        uint8_t rank,
        float   distance,
        float   maxDistance
    );

    void start();

    void stop();

    void entrypoint();
}

#endif //UNHIDENPCS_UNPC_HPP
