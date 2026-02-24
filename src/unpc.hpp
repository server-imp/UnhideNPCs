#ifndef UNHIDENPCS_UNPC_HPP
#define UNHIDENPCS_UNPC_HPP
#pragma once
#include "fw/logger.hpp"
#include "fw/memory/memory.hpp"
#include "gw2.hpp"
#include "hotkey.hpp"
#include "MumbleLink.hpp"
#include "settings.hpp"
#include "version.hpp"

namespace unpc
{
    enum class EMode : int32_t
    {
        Unknown  = 0,
        Proxy    = 1,
        Injected = 2,
        Nexus    = 3,
        ArcDps   = 4
    };

    constexpr auto SIGNATURE = 1817724315;

    inline auto mode = EMode::Unknown;

    extern std::atomic_bool nexusPresent;
    extern std::atomic_bool arcDpsPresent;
    extern std::atomic_bool injected;
    extern std::atomic_bool exit;
    extern std::atomic_bool stopping;

    extern std::optional<logging::Logger> logger;
    extern std::optional<Settings>        settings;
    extern std::optional<memory::Detour>  npcHook;
    extern HotkeyManager                  hotkeyManager;

    extern HANDLE      hMutex;
    extern HMODULE     hModule;
    extern HANDLE      hThread;
    extern std::string proxyModuleName;
    extern HMODULE     hProxyModule;

    extern MumbleLink* mumbleLink;
    extern int32_t*    loadingScreenActive;

    extern uint32_t numPlayersVisible;
    extern uint32_t numPlayerOwnedVisible;
    extern uint32_t numNpcsVisible;
    extern uint32_t numPlayersInArea;

    extern bool unloadOverlay;

    namespace current_settings
    {
        extern std::mutex mutex;

        extern bool    unhideNpcs;
        extern bool    unhidePlayers;
        extern bool    playerOwned;
        extern bool    alwaysShowTarget;
        extern bool    unhideLowQuality;
        extern int32_t minimumRank;
        extern int32_t attackable;
        extern float   maximumDistance;

        extern bool    hidePlayers;
        extern bool    hidePlayerOwned;
        extern bool    hideBlockedPlayers;
        extern bool    hideBlockedPlayersOwned;
        extern bool    hideNonGroupMembers;
        extern bool    hideNonGroupMembersOwned;
        extern bool    hideNonGuildMembers;
        extern bool    hideNonGuildMembersOwned;
        extern bool    hideNonFriends;
        extern bool    hideNonFriendsOwned;
        extern bool    hidePlayerOwnedSelf;
        extern bool    hidePlayersInCombat;
        extern bool    hidePlayerOwnedInCombat;
        extern int32_t maxPlayersVisible;
        extern int32_t maxPlayerOwnedVisible;
        extern int32_t maxNpcs;
        extern bool    disableHidingInInstances;

        extern bool  forceConsole;
        extern bool  loadScreenBoost;
        extern bool  closeOnEscape;
        extern float overlayFontSize;
        extern bool  disableOverlay;
        extern bool  overlayOpen;

        void update();
    }

    void onHookTick();

    // This function may be called second, returning true means the character will be forced invisible
    bool shouldHide(
        bool    isPlayer,
        bool    isPlayerOwned,
        bool    isOwnerLocalPlayer,
        bool    isTarget,
        bool    isAttackable,
        uint8_t rank,
        float   distance,
        bool    isFriend,
        bool    isBlocked,
        bool    isActiveGuildMember,
        bool    isGuildMember,
        bool    isPartyMember,
        bool    isSquadMember
    );

    // This function is called first, returning true means the character will be forced visible
    // if returning false, it will go on to call shouldHide
    bool shouldShow(
        bool    isPlayer,
        bool    isPlayerOwned,
        bool    isOwnerLocalPlayer,
        bool    isTarget,
        bool    isAttackable,
        uint8_t rank,
        float   distance,
        bool    isFriend,
        bool    isBlocked,
        bool    isActiveGuildMember,
        bool    isGuildMember,
        bool    isPartyMember,
        bool    isSquadMember
    );

    void start();

    void stop();

    void entrypoint();
}

#endif //UNHIDENPCS_UNPC_HPP
