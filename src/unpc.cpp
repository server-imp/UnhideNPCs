#include "unpc.hpp"

#include "hook.hpp"
#include "hotkey.hpp"
#include "ui.hpp"
#include "integration/nexus.hpp"

using namespace std::chrono_literals;
using namespace memory;

using namespace unpc;

std::atomic_bool               unpc::nexusPresent {};
std::atomic_bool               unpc::arcDpsPresent {};
std::atomic_bool               unpc::injected {};
std::atomic_bool               unpc::exit {};
std::optional<logging::Logger> unpc::logger;
std::optional<Settings>        unpc::settings;
std::optional<Detour>          unpc::npcHook {};

HotkeyManager unpc::hotkeyManager("ArenaNet_Gr_Window_Class", std::filesystem::current_path() / "addons" / "UnhideNPCs" / "hotkeys.json");
HANDLE        unpc::hMutex {};
HMODULE       unpc::hModule {};

HANDLE unpc::hThread {};

std::string unpc::proxyModuleName {};
HMODULE     unpc::hProxyModule {};
MumbleLink* unpc::mumbleLink {};
int32_t*    unpc::loadingScreenActive {};

uint32_t unpc::numPlayersVisible {};
uint32_t unpc::numPlayerOwnedVisible {};
uint32_t unpc::numNpcsVisible {};
bool     unpc::unloadOverlay {};

#include "re.hpp"

void hotkeyCallback(const std::string& id)
{
    if (!unpc::settings)
    {
        return;
    }

    if (id == "ToggleUnhideNPCs")
    {
        const auto current = unpc::settings->getUnhideNpcs();
        unpc::settings->setUnhideNpcs(!current);
    }
    else if (id == "ToggleUnhidePlayerOwned")
    {
        const auto current = unpc::settings->getPlayerOwned();
        unpc::settings->setPlayerOwned(!current);
    }
    else if (id == "ToggleUnhideTarget")
    {
        const auto current = unpc::settings->getAlwaysShowTarget();
        unpc::settings->setAlwaysShowTarget(!current);
    }
    else if (id == "ToggleHidePlayers")
    {
        const auto current = unpc::settings->getHidePlayers();
        unpc::settings->setHidePlayers(!current);
    }
    else if (id == "ToggleHideBlocked")
    {
        const auto current = unpc::settings->getHideBlockedPlayers();
        unpc::settings->setHideBlockedPlayers(!current);
    }
    else if (id == "ToggleHideNonGroup")
    {
        const auto current = unpc::settings->getHideNonGroupMembers();
        unpc::settings->setHideNonGroupMembers(!current);
    }
    else if (id == "ToggleHideNonGuild")
    {
        const auto current = unpc::settings->getHideNonGuildMembers();
        unpc::settings->setHideNonGuildMembers(!current);
    }
    else if (id == "ToggleHideNonFriends")
    {
        const auto current = unpc::settings->getHideNonFriends();
        unpc::settings->setHideNonFriends(!current);
    }
    else if (id == "ToggleHideAllOwned")
    {
        const auto current = unpc::settings->getHidePlayerOwned();
        unpc::settings->setHidePlayerOwned(!current);
    }
    else if (id == "ToggleHideBlockedOwned")
    {
        const auto current = unpc::settings->getHideBlockedPlayersOwned();
        unpc::settings->setHideBlockedPlayersOwned(!current);
    }
    else if (id == "ToggleHideNonGroupOwned")
    {
        const auto current = unpc::settings->getHideNonGroupMembersOwned();
        unpc::settings->setHideNonGroupMembersOwned(!current);
    }
    else if (id == "ToggleHideNonGuildOwned")
    {
        const auto current = unpc::settings->getHideNonGuildMembersOwned();
        unpc::settings->setHideNonGuildMembersOwned(!current);
    }
    else if (id == "ToggleHideNonFriendsOwned")
    {
        const auto current = unpc::settings->getHideNonFriendsOwned();
        unpc::settings->setHideNonFriendsOwned(!current);
    }
    else if (id == "ToggleHideSelfOwned")
    {
        const auto current = unpc::settings->getHidePlayerOwnedSelf();
        unpc::settings->setHidePlayerOwnedSelf(!current);
    }
    else if (id == "ToggleHidePlayersInCombat")
    {
        const auto current = unpc::settings->getHidePlayersInCombat();
        unpc::settings->setHidePlayersInCombat(!current);
    }
    else if (id == "ToggleHidePlayerOwnedInCombat")
    {
        const auto current = unpc::settings->getHidePlayerOwnedInCombat();
        unpc::settings->setHidePlayerOwnedInCombat(!current);
    }
    else if (id == "ToggleDisableInInstances")
    {
        const auto current = unpc::settings->getDisableHidingInInstances();
        unpc::settings->setDisableHidingInInstances(!current);
    }
    else if (id == "ForceVisibility")
    {
        ++re::forceVisibility;
    }
    else
    {
        LOG_WARN("Unknown hotkey: {}", id);
    }
}

void initializeHotkeys()
{
    hotkeyManager.registerHotkey("ToggleUnhideNPCs", "Unhide NPCs");
    hotkeyManager.registerHotkey("ToggleUnhidePlayerOwned", "Unhide Player-Owned");
    hotkeyManager.registerHotkey("ToggleUnhideTarget", "Unhide Target");
    hotkeyManager.registerHotkey("ToggleHidePlayers", "Hide All Player");
    hotkeyManager.registerHotkey("ToggleHideBlocked", "Hide Blocked Player");
    hotkeyManager.registerHotkey("ToggleHideNonGroup", "Hide Non-Group Player");
    hotkeyManager.registerHotkey("ToggleHideNonGuild", "Hide Non-Guild Player");
    hotkeyManager.registerHotkey("ToggleHideNonFriends", "Hide Non-Friends Player");
    hotkeyManager.registerHotkey("ToggleHideAllOwned", "Hide All Owned");
    hotkeyManager.registerHotkey("ToggleHideBlockedOwned", "Hide Blocked Owned");
    hotkeyManager.registerHotkey("ToggleHideNonGroupOwned", "Hide Non-Group Owned");
    hotkeyManager.registerHotkey("ToggleHideNonGuildOwned", "Hide Non-Guild Owned");
    hotkeyManager.registerHotkey("ToggleHideNonFriendsOwned", "Hide Non-Friends Owned");
    hotkeyManager.registerHotkey("ToggleHideSelfOwned", "Hide Self Owned");
    hotkeyManager.registerHotkey("ToggleHidePlayersInCombat", "Hide Players in Combat");
    hotkeyManager.registerHotkey("ToggleHidePlayerOwnedInCombat", "Hide Player-Owned in Combat");
    hotkeyManager.registerHotkey("ToggleDisableInInstances", "Disable in Instances");
    hotkeyManager.registerHotkey("ForceVisibility", "Force Visibility");

    hotkeyManager.registerCallback(hotkeyCallback);

    hotkeyManager.load();
}

bool initialize()
{
    LOG_INFO("Initializing");

    if (!mumbleLink)
    {
        mumbleLink = mode == EMode::Nexus ? nexus::getMumbleLink() : getMumbleLink();
        if (!mumbleLink)
        {
            LOG_ERR("MumbleLink Failed");
            return false;
        }
    }
    LOG_INFO("MumbleLink OK");

    settings.emplace(std::filesystem::current_path() / "addons" / "UnhideNPCs" / "config.cfg");
    if (!settings || !settings->loaded())
    {
        LOG_ERR("Failed to load config");
        return false;
    }
    LOG_INFO("Config OK");

    const auto minRank = settings->getMinimumRank();
    if (minRank < 0 || minRank >= static_cast<int32_t>(re::gw2::ECharacterRankMax))
    {
        settings->setMinimumRank(0);
    }

    const auto attitudeMode = settings->getAttackable();
    if (attitudeMode < 0 || attitudeMode > 2)
    {
        settings->setAttackable(0);
    }

    const auto maximumDistance = settings->getMaximumDistance();
    if (maximumDistance < 0.0f || maximumDistance > 1000.0f)
    {
        settings->setMaximumDistance(0.0f);
    }

    const auto fontSize = settings->getOverlayFontSize();
    if (fontSize < 10 || fontSize > 20)
    {
        settings->setOverlayFontSize(20);
    }

    const auto maximumPlayers = settings->getMaxPlayersVisible();
    if (maximumPlayers > 1000)
    {
        settings->setMaxPlayersVisible(1000);
    }
    const auto maximumPlayerOwned = settings->getMaxPlayerOwnedVisible();
    if (maximumPlayerOwned > 1000)
    {
        settings->setMaxPlayerOwnedVisible(1000);
    }

    if (settings->getForceConsole())
    {
        logger->setConsole(true);
    }

    Module game {};
    if (!Module::tryGetByName("Gw2-64.exe", game))
    {
        LOG_ERR("Unable to get Gw2-64.exe module");
        return false;
    }
    LOG_INFO("Gw2-64.exe OK");

    Handle pointer {};
    if (!game.findPattern(re::pattern1, pointer))
    {
        LOG_ERR("Unable to find pattern 1");
        return false;
    }
    re::vtableIndex = pointer.add(2).deref<uint32_t>() / 8;
    LOG_DBG("VTable Index: {}", re::vtableIndex);
    LOG_INFO("Pattern 1 OK");

    if (!game.findPattern(re::pattern2, pointer))
    {
        LOG_ERR("Unable to find pattern 2");
        return false;
    }
    pointer = pointer.add(10).resolve_relative_call();
    LOG_DBG("Resolved call to {}+{:X}", game.name(), pointer.sub(game.start()).raw());
    npcHook.emplace("Hook", pointer.to_ptr<void*>(), reinterpret_cast<void*>(re::hook));
    LOG_INFO("Pattern 2 OK");

    if (!game.findPattern(re::pattern3, pointer))
    {
        LOG_ERR("Unable to find pattern 3");
        return false;
    }
    pointer             = pointer.add(5);
    loadingScreenActive = pointer.add(6).add(pointer.add(2).deref<int32_t>()).to_ptr<int32_t*>();
    LOG_DBG("Loading screen active: {:08X}", reinterpret_cast<uintptr_t>(loadingScreenActive));
    LOG_INFO("Pattern 3 OK");

    if (!game.findPattern(re::pattern4, pointer))
    {
        LOG_ERR("Unable to find pattern 4");
        return false;
    }
    re::gw2::getContextCollection = reinterpret_cast<re::gw2::GetContextCollectionFn>(pointer.raw());
    LOG_INFO("Pattern 4 OK");

    if (!Scanner::findStringReference(re::pattern5, pointer))
    {
        LOG_ERR("Unable to find pattern 5");
        return false;
    }
    re::gw2::getAvContext = reinterpret_cast<re::gw2::GetAvContextFn>(pointer.add(12).resolve_relative_call().raw());
    LOG_INFO("Pattern 5 OK");

    if (!npcHook->enable())
    {
        LOG_ERR("Failed to enable hook");
        return false;
    }
    LOG_INFO("Hook OK");

    initializeHotkeys();

    return true;
}

void unpc::onHookTick()
{
    if (exit)
    {
        return;
    }

    if (unloadOverlay && ui::d3dHook)
    {
        ui::d3dHook.reset();
        unloadOverlay = false;
    }

    hotkeyManager.tick();

    if (mode != EMode::Injected && mode != EMode::Proxy)
    {
        return;
    }

    if (!settings || settings->getDisableOverlay())
    {
        return;
    }

    const bool loadingScreen = *loadingScreenActive;
    const bool uiVersion     = mumbleLink && mumbleLink->uiVersion == 2;

    if (loadingScreen || uiVersion)
    {
        // If loading wait 3 seconds, otherwise initialize overlay straight away
        static uint64_t initialTick = GetTickCount64() - (uiVersion ? 3000 : 0);
        if (GetTickCount64() - initialTick < 3000)
        {
            return;
        }
        initialTick = GetTickCount64();

        if (!ui::d3dHook && (mode == EMode::Proxy || mode == EMode::Injected))
        {
            ui::d3dHook = hooks::D3D11::create("ArenaNet_Gr_Window_Class", "", ui::onD3DPresent, ui::onD3DResizeBuffers, ui::onD3DStarted, ui::onD3DShutdown);

            if (!ui::d3dHook || !ui::d3dHook.value()->enable())
            {
                LOG_INFO("Overlay Not OK");
            }
            else
            {
                LOG_INFO("Overlay OK");
            }
        }
    }
}

bool unpc::shouldHide(
    const bool    isPlayer,
    const bool    isPlayerOwned,
    const bool    isOwnerLocalPlayer,
    const bool    isTarget,
    const bool    isAttackable,
    const uint8_t rank,
    const float   distance,
    const float   maxDistance,
    const bool    isFriend,
    const bool    isBlocked,
    const bool    isActiveGuildMember,
    const bool    isGuildMember,
    const bool    isPartyMember,
    const bool    isSquadMember,
    const bool    isMounted
)
{
    if (unpc::settings->getDisableHidingInInstances() && unpc::mumbleLink && unpc::mumbleLink->getContext().mapType == MapType::Instances)
    {
        return false;
    }

    if (isTarget && unpc::settings->getAlwaysShowTarget())
    {
        return false;
    }

    if (isPlayer)
    {
        const auto maxPlayersVisible = unpc::settings->getMaxPlayersVisible();
        if (maxPlayersVisible > 0 && unpc::numPlayersVisible >= maxPlayersVisible)
        {
            return true;
        }

        if (unpc::settings->getHidePlayers())
        {
            return true;
        }

        if (unpc::settings->getHideNonFriends() && !isFriend)
        {
            return true;
        }

        if (unpc::settings->getHideBlockedPlayers() && isBlocked)
        {
            return true;
        }

        const bool guild = isActiveGuildMember || isGuildMember;
        if (unpc::settings->getHideNonGuildMembers() && !guild)
        {
            return true;
        }

        const bool group = isPartyMember || isSquadMember;
        if (unpc::settings->getHideNonGroupMembers() && !group)
        {
            return true;
        }

        if (!isOwnerLocalPlayer && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat() && unpc::settings->getHidePlayersInCombat())
        {
            return true;
        }

        return false;
    }

    if (isPlayerOwned)
    {
        const auto maxPlayerOwnedVisible = unpc::settings->getMaxPlayerOwnedVisible();
        if (maxPlayerOwnedVisible > 0 && unpc::numPlayerOwnedVisible >= maxPlayerOwnedVisible)
        {
            return true;
        }

        if (isOwnerLocalPlayer && unpc::settings->getHidePlayerOwnedSelf())
        {
            return true;
        }

        if (unpc::settings->getHidePlayerOwned() && !isOwnerLocalPlayer)
        {
            return true;
        }

        if (unpc::settings->getHideNonFriendsOwned() && !isFriend)
        {
            return true;
        }

        if (unpc::settings->getHideBlockedPlayersOwned() && isBlocked)
        {
            return true;
        }

        const bool guild = isActiveGuildMember || isGuildMember;
        if (unpc::settings->getHideNonGuildMembersOwned() && !guild)
        {
            return true;
        }

        const bool group = isPartyMember || isSquadMember;
        if (unpc::settings->getHideNonGroupMembersOwned() && !group)
        {
            return true;
        }

        if (unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat() && unpc::settings->getHidePlayerOwnedInCombat())
        {
            return true;
        }

        return false;
    }

    const auto maxNpcs = unpc::settings->getMaxNpcs();
    if (maxNpcs > 0 && unpc::numNpcsVisible >= maxNpcs)
    {
        return true;
    }

    return false;
}

bool unpc::shouldShow(
    const bool    isPlayer,
    const bool    isPlayerOwned,
    const bool    isOwnerLocalPlayer,
    const bool    isTarget,
    const bool    isAttackable,
    const uint8_t rank,
    const float   distance,
    const float   maxDistance,
    const bool    isFriend,
    const bool    isBlocked,
    const bool    isActiveGuildMember,
    const bool    isGuildMember,
    const bool    isPartyMember,
    const bool    isSquadMember,
    const bool    isMounted
)
{
    if (isTarget && unpc::settings->getAlwaysShowTarget())
    {
        return true;
    }

    if (isPlayer)
    {
        const auto maxPlayersVisible = unpc::settings->getMaxPlayersVisible();
        if (maxPlayersVisible > 0 && unpc::numPlayersVisible >= maxPlayersVisible)
        {
            return false;
        }

        if (unpc::settings->getHidePlayers())
        {
            return false;
        }

        if (unpc::settings->getHideNonFriends() && !isFriend)
        {
            return false;
        }

        if (unpc::settings->getHideBlockedPlayers() && isBlocked)
        {
            return false;
        }

        const bool guild = isActiveGuildMember || isGuildMember;
        if (unpc::settings->getHideNonGuildMembers() && !guild)
        {
            return false;
        }

        const bool group = isPartyMember || isSquadMember;
        if (unpc::settings->getHideNonGroupMembers() && !group)
        {
            return false;
        }

        if (unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat() && unpc::settings->getHidePlayersInCombat())
        {
            return false;
        }

        return true;
    }

    if (isPlayerOwned)
    {
        if (!unpc::settings->getPlayerOwned())
        {
            return false;
        }

        const auto maxPlayerOwnedVisible = unpc::settings->getMaxPlayerOwnedVisible();
        if (maxPlayerOwnedVisible > 0 && unpc::numPlayerOwnedVisible >= maxPlayerOwnedVisible)
        {
            return false;
        }

        if (unpc::settings->getHidePlayerOwned())
        {
            if (isOwnerLocalPlayer && !unpc::settings->getHidePlayerOwnedSelf())
            {
                return true;
            }
            return false;
        }

        if (unpc::settings->getHideNonFriendsOwned() && !isFriend)
        {
            return false;
        }

        if (unpc::settings->getHideBlockedPlayersOwned() && isBlocked)
        {
            return false;
        }

        const bool guild = isActiveGuildMember || isGuildMember;
        if (unpc::settings->getHideNonGuildMembersOwned() && !guild)
        {
            return false;
        }

        bool group = isPartyMember || isSquadMember;
        if (unpc::settings->getHideNonGroupMembersOwned() && !group)
        {
            return false;
        }

        if (unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat() && unpc::settings->getHidePlayerOwnedInCombat())
        {
            return false;
        }

        return true;
    }

    if (!unpc::settings->getUnhideNpcs())
    {
        return false;
    }

    const auto maxNpcs = unpc::settings->getMaxNpcs();
    if (maxNpcs > 0 && unpc::numNpcsVisible >= maxNpcs)
    {
        return false;
    }

    const auto minRank = static_cast<re::gw2::ECharacterRank>(unpc::settings->getMinimumRank());
    if (rank < minRank)
    {
        return false;
    }

    const auto _mode = unpc::settings->getAttackable();
    if (_mode == 1 && !isAttackable)
    {
        return false;
    }
    if (_mode == 2 && isAttackable)
    {
        return false;
    }

    if (maxDistance > 0 && distance >= maxDistance)
    {
        return false;
    }

    return true;
}

void unpc::start()
{
    if (settings && logger && npcHook)
    {
        return;
    }

#ifdef BUILDING_ON_GITHUB
    auto logLevel = logging::LogLevel::Info;
#else
    auto logLevel = logging::LogLevel::Info;
#endif

    logger.emplace("UnhideNPCs", std::filesystem::current_path() / "addons" / "UnhideNPCs" / "log.txt", logLevel);

    LOG_INFO("Starting");
    LOG_INFO("Version {}", version::STRING);
    LOG_INFO("Built on {} at {}", __DATE__, __TIME__);

    if (injected)
    {
        LOG_INFO("Mode: Injected");
        mode = EMode::Injected;
    }
    else if (hProxyModule)
    {
        LOG_INFO("Mode: Proxy({})", proxyModuleName);
        mode = EMode::Proxy;
    }
    else if (nexusPresent && util::isModuleInAnyDirsRelativeToExe(hModule, { "addons" }))
    {
        LOG_INFO("Mode: Nexus");
        mode = EMode::Nexus;
        logger->registerCallback(nexus::logCallback);
    }
    else if (arcDpsPresent && util::isModuleInAnyDirsRelativeToExe(hModule, { "", "bin64" }))
    {
        LOG_INFO("Mode: ArcDPS");
        mode = EMode::ArcDps;
    }
    else
    {
        LOG_INFO("Mode: Unknown");
        mode = EMode::Unknown;
        exit = true;
        return;
    }

    if (!initialize())
    {
        LOG_ERR("Initialization failed");
        exit = true;
        return;
    }

    LOG_INFO("Initialization complete");
}

void unpc::stop()
{
    LOG_INFO("Stopping");

    if (ui::d3dHook)
    {
        ui::d3dHook.reset();
    }

    if (npcHook)
    {
        npcHook->disable(true);
        npcHook.reset();
    }

    if (mode == EMode::Nexus)
    {
        logger->unregisterCallback(nexus::logCallback);
    }

    settings.reset();
    logger.reset();

    if (mumbleLink && mode != EMode::Nexus)
    {
        UnmapViewOfFile(mumbleLink);
        mumbleLink = nullptr;
    }

    util::closeHandle(hMutex);
}

void unpc::entrypoint()
{
    // need to use CreateThread because for some reason std::thread doesn't allow the dll to unload cleanly
    hThread = CreateThread(
        nullptr,
        0,
        [](PVOID) -> DWORD
        {
            start();

            while (!exit)
            {
                if (injected && ui::wasKeyPressed(VK_END))
                {
                    exit = true;
                    break;
                }

                Sleep(250);
            }

            stop();

            util::closeHandle(hThread);
            if (!hProxyModule)
            {
                FreeLibraryAndExitThread(hModule, 0);
            }

            // otherwise just free the proxy module(decrease refcount) if we have one and exit the thread
            util::freeLibrary(hProxyModule);
            ExitThread(0);
        },
        nullptr,
        0,
        nullptr
    );
}
