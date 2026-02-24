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
std::atomic_bool               unpc::stopping {};
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
uint32_t unpc::numPlayersInArea {};

bool     unpc::unloadOverlay {};

#include "re.hpp"

void hotkeyCallback(const std::string& id)
{
    if (!unpc::settings)
    {
        return;
    }

    auto toggle = [](bool& value)
    {
        value = !value;
        return value;
    };

    std::lock_guard lock { current_settings::mutex };

    if (id == "ToggleUnhideNPCs")
    {
        unpc::settings->setUnhideNpcs(toggle(current_settings::unhideNpcs));
    }
    else if (id == "ToggleUnhidePlayers")
    {
        unpc::settings->setUnhidePlayers(toggle(current_settings::unhidePlayers));
    }
    else if (id == "ToggleUnhidePlayerOwned")
    {
        unpc::settings->setPlayerOwned(toggle(current_settings::playerOwned));
    }
    else if (id == "ToggleUnhideTarget")
    {
        unpc::settings->setAlwaysShowTarget(toggle(current_settings::alwaysShowTarget));
    }
    else if (id == "ToggleUnhideLowQuality")
    {
        unpc::settings->setUnhideLowQuality(toggle(current_settings::unhideLowQuality));
    }
    else if (id == "ToggleHidePlayers")
    {
        unpc::settings->setHidePlayers(toggle(current_settings::hidePlayers));
    }
    else if (id == "ToggleHideBlocked")
    {
        unpc::settings->setHideBlockedPlayers(toggle(current_settings::hideBlockedPlayers));
    }
    else if (id == "ToggleHideNonGroup")
    {
        unpc::settings->setHideNonGroupMembers(toggle(current_settings::hideNonGroupMembers));
    }
    else if (id == "ToggleHideNonGuild")
    {
        unpc::settings->setHideNonGuildMembers(toggle(current_settings::hideNonGuildMembers));
    }
    else if (id == "ToggleHideNonFriends")
    {
        unpc::settings->setHideNonFriends(toggle(current_settings::hideNonFriends));
    }
    else if (id == "ToggleHideAllOwned")
    {
        unpc::settings->setHidePlayerOwned(toggle(current_settings::hidePlayerOwned));
    }
    else if (id == "ToggleHideBlockedOwned")
    {
        unpc::settings->setHideBlockedPlayersOwned(toggle(current_settings::hideBlockedPlayersOwned));
    }
    else if (id == "ToggleHideNonGroupOwned")
    {
        unpc::settings->setHideNonGroupMembersOwned(toggle(current_settings::hideNonGroupMembersOwned));
    }
    else if (id == "ToggleHideNonGuildOwned")
    {
        unpc::settings->setHideNonGuildMembersOwned(toggle(current_settings::hideNonGuildMembersOwned));
    }
    else if (id == "ToggleHideNonFriendsOwned")
    {
        unpc::settings->setHideNonFriendsOwned(toggle(current_settings::hideNonFriendsOwned));
    }
    else if (id == "ToggleHideSelfOwned")
    {
        unpc::settings->setHidePlayerOwnedSelf(toggle(current_settings::hidePlayerOwnedSelf));
    }
    else if (id == "ToggleHidePlayersInCombat")
    {
        unpc::settings->setHidePlayersInCombat(toggle(current_settings::hidePlayersInCombat));
    }
    else if (id == "ToggleHidePlayerOwnedInCombat")
    {
        unpc::settings->setHidePlayerOwnedInCombat(toggle(current_settings::hidePlayerOwnedInCombat));
    }
    else if (id == "ToggleDisableInInstances")
    {
        unpc::settings->setDisableHidingInInstances(toggle(current_settings::disableHidingInInstances));
    }
    else if (id == "ForceVisibility")
    {
        ++re::forceVisibility;
    }
    else if (id == "ToggleOverlay")
    {
        if (current_settings::overlayOpen && hotkeyManager.isCapturing())
        {
            hotkeyManager.stopCapturing();
        }

        unpc::settings->setOverlayOpen(toggle(current_settings::overlayOpen));
    }
    else
    {
        LOG_WARN("Unknown hotkey: {}", id);
    }
}

void initializeHotkeys()
{
    hotkeyManager.registerHotkey("ToggleUnhideNPCs", "Unhide NPCs");
    hotkeyManager.registerHotkey("ToggleUnhidePlayers", "Unhide Players");
    hotkeyManager.registerHotkey("ToggleUnhidePlayerOwned", "Unhide Player-Owned");
    hotkeyManager.registerHotkey("ToggleUnhideTarget", "Unhide Target");
    hotkeyManager.registerHotkey("ToggleUnhideLowQuality", "Low Quality Models");
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
    hotkeyManager.registerHotkey("ToggleOverlay", "Alt. Overlay Toggle");

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

    current_settings::update();

    if (current_settings::minimumRank < 0 || current_settings::minimumRank >= static_cast<int32_t>(re::gw2::ECharacterRankMax))
    {
        current_settings::minimumRank = settings->setMinimumRank(0);
    }

    if (current_settings::attackable < 0 || current_settings::attackable > 2)
    {
        current_settings::attackable = settings->setAttackable(0);
    }

    const auto maximumDistance = current_settings::maximumDistance * 0.3125f;
    if (maximumDistance < 0.0f || maximumDistance > 1000.0f)
    {
        current_settings::maximumDistance = settings->setMaximumDistance(0.0f) * 32.0f;
    }

    if (current_settings::overlayFontSize < 10 || current_settings::overlayFontSize > 20)
    {
        current_settings::overlayFontSize = settings->setOverlayFontSize(20);
    }

    if (current_settings::maxPlayersVisible > 1000)
    {
        current_settings::maxPlayersVisible = settings->setMaxPlayersVisible(1000);
    }

    const auto maximumPlayerOwned = settings->getMaxPlayerOwnedVisible();
    if (current_settings::maxPlayerOwnedVisible > 1000)
    {
        current_settings::maxPlayerOwnedVisible = settings->setMaxPlayerOwnedVisible(1000);
    }

    if (current_settings::forceConsole)
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

    /*if (!game.findPattern(re::pattern6, pointer))
    {
        LOG_ERR("Unable to find pattern 6");
        return false;
    }
    re::gw2::getUIContext = reinterpret_cast<re::gw2::GetUIContextFn>(pointer.add(9).rip().raw());
    LOG_INFO("Pattern 6 OK");*/

    if (!npcHook->enable())
    {
        LOG_ERR("Failed to enable hook");
        return false;
    }
    LOG_INFO("Hook OK");

    initializeHotkeys();

    return true;
}

std::mutex current_settings::mutex {};

bool    current_settings::unhideNpcs {};
bool    current_settings::unhidePlayers {};
bool    current_settings::playerOwned {};
bool    current_settings::alwaysShowTarget {};
bool    current_settings::unhideLowQuality {};
int32_t current_settings::minimumRank {};
int32_t current_settings::attackable {};
float   current_settings::maximumDistance {};

bool    current_settings::hidePlayers {};
bool    current_settings::hidePlayerOwned {};
bool    current_settings::hideBlockedPlayers {};
bool    current_settings::hideBlockedPlayersOwned {};
bool    current_settings::hideNonGroupMembers {};
bool    current_settings::hideNonGroupMembersOwned {};
bool    current_settings::hideNonGuildMembers {};
bool    current_settings::hideNonGuildMembersOwned {};
bool    current_settings::hideNonFriends {};
bool    current_settings::hideNonFriendsOwned {};
bool    current_settings::hidePlayerOwnedSelf {};
bool    current_settings::hidePlayersInCombat {};
bool    current_settings::hidePlayerOwnedInCombat {};
int32_t current_settings::maxPlayersVisible {};
int32_t current_settings::maxPlayerOwnedVisible {};
int32_t current_settings::maxNpcs {};
bool    current_settings::disableHidingInInstances {};

bool  current_settings::forceConsole {};
bool  current_settings::loadScreenBoost {};
bool  current_settings::closeOnEscape {};
float current_settings::overlayFontSize {};
bool  current_settings::disableOverlay {};
bool  current_settings::overlayOpen {};

void current_settings::update()
{
    if (!unpc::settings || !unpc::settings->loaded())
    {
        return;
    }

    std::lock_guard lock { mutex };

    unhideNpcs       = unpc::settings->getUnhideNpcs();
    unhidePlayers    = unpc::settings->getUnhidePlayers();
    playerOwned      = unpc::settings->getPlayerOwned();
    alwaysShowTarget = unpc::settings->getAlwaysShowTarget();
    unhideLowQuality = unpc::settings->getUnhideLowQuality();
    minimumRank      = static_cast<re::gw2::ECharacterRank>(unpc::settings->getMinimumRank());
    attackable       = unpc::settings->getAttackable();
    maximumDistance  = unpc::settings->getMaximumDistance() * 32.0f;

    hidePlayers              = unpc::settings->getHidePlayers();
    hidePlayerOwned          = unpc::settings->getHidePlayerOwned();
    hideBlockedPlayers       = unpc::settings->getHideBlockedPlayers();
    hideBlockedPlayersOwned  = unpc::settings->getHideBlockedPlayersOwned();
    hideNonGroupMembers      = unpc::settings->getHideNonGroupMembers();
    hideNonGroupMembersOwned = unpc::settings->getHideNonGroupMembersOwned();
    hideNonGuildMembers      = unpc::settings->getHideNonGuildMembers();
    hideNonGuildMembersOwned = unpc::settings->getHideNonGuildMembersOwned();
    hideNonFriends           = unpc::settings->getHideNonFriends();
    hideNonFriendsOwned      = unpc::settings->getHideNonFriendsOwned();
    hidePlayerOwnedSelf      = unpc::settings->getHidePlayerOwnedSelf();
    hidePlayersInCombat      = unpc::settings->getHidePlayersInCombat();
    hidePlayerOwnedInCombat  = unpc::settings->getHidePlayerOwnedInCombat();
    maxPlayersVisible        = unpc::settings->getMaxPlayersVisible();
    maxPlayerOwnedVisible    = unpc::settings->getMaxPlayerOwnedVisible();
    maxNpcs                  = unpc::settings->getMaxNpcs();
    disableHidingInInstances = unpc::settings->getDisableHidingInInstances();

    forceConsole    = unpc::settings->getForceConsole();
    loadScreenBoost = unpc::settings->getLoadScreenBoost();
    closeOnEscape   = unpc::settings->getCloseOnEscape();
    overlayFontSize = unpc::settings->getOverlayFontSize();
    disableOverlay  = unpc::settings->getDisableOverlay();
    overlayOpen     = unpc::settings->getOverlayOpen();
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

    hotkeyManager.update();
    current_settings::update();

    if (mode != EMode::Injected && mode != EMode::Proxy)
    {
        return;
    }

    if (!settings || current_settings::disableOverlay)
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
    const bool    isFriend,
    const bool    isBlocked,
    const bool    isActiveGuildMember,
    const bool    isGuildMember,
    const bool    isPartyMember,
    const bool    isSquadMember
)
{
    if (current_settings::disableHidingInInstances && unpc::mumbleLink && unpc::mumbleLink->getContext().mapType == MapType::Instances)
    {
        return false;
    }

    if (isTarget && current_settings::alwaysShowTarget)
    {
        return false;
    }

    if (isPlayer)
    {
        if (current_settings::maxPlayersVisible > 0 && unpc::numPlayersVisible >= current_settings::maxPlayersVisible)
        {
            return true;
        }

        if (current_settings::hidePlayers)
        {
            return true;
        }

        if (current_settings::hideNonFriends && !isFriend)
        {
            return true;
        }

        if (current_settings::hideBlockedPlayers && isBlocked)
        {
            return true;
        }

        if (current_settings::hideNonGuildMembers && !(isActiveGuildMember || isGuildMember))
        {
            return true;
        }

        if (current_settings::hideNonGroupMembers && !(isPartyMember || isSquadMember))
        {
            return true;
        }

        if (current_settings::hidePlayersInCombat && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return true;
        }

        return false;
    }

    if (isPlayerOwned)
    {
        if (current_settings::maxPlayerOwnedVisible > 0 && unpc::numPlayerOwnedVisible >= current_settings::maxPlayerOwnedVisible)
        {
            return true;
        }

        if (isOwnerLocalPlayer && current_settings::hidePlayerOwnedSelf)
        {
            return true;
        }

        if (current_settings::hidePlayerOwned && !isOwnerLocalPlayer)
        {
            return true;
        }

        if (current_settings::hideNonFriendsOwned && !isFriend)
        {
            return true;
        }

        if (current_settings::hideBlockedPlayersOwned && isBlocked)
        {
            return true;
        }

        if (current_settings::hideNonGuildMembersOwned && !(isActiveGuildMember || isGuildMember))
        {
            return true;
        }

        if (current_settings::hideNonGroupMembersOwned && !(isPartyMember || isSquadMember))
        {
            return true;
        }

        if (!isOwnerLocalPlayer && current_settings::hidePlayerOwnedInCombat && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return true;
        }

        return false;
    }

    if (current_settings::maxNpcs > 0 && unpc::numNpcsVisible >= current_settings::maxNpcs)
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
    const bool    isFriend,
    const bool    isBlocked,
    const bool    isActiveGuildMember,
    const bool    isGuildMember,
    const bool    isPartyMember,
    const bool    isSquadMember
)
{
    if (isTarget && current_settings::alwaysShowTarget)
    {
        return true;
    }

    if (isPlayer)
    {
        if (!current_settings::unhidePlayers)
        {
            return false;
        }

        if (current_settings::maxPlayersVisible > 0 && unpc::numPlayersVisible >= current_settings::maxPlayersVisible)
        {
            return false;
        }

        if (current_settings::hidePlayers)
        {
            return false;
        }

        if (current_settings::hideNonFriends && !isFriend)
        {
            return false;
        }

        if (current_settings::hideBlockedPlayers && isBlocked)
        {
            return false;
        }

        if (current_settings::hideNonGuildMembers && !(isActiveGuildMember || isGuildMember))
        {
            return false;
        }

        if (current_settings::hideNonGroupMembers && !(isPartyMember || isSquadMember))
        {
            return false;
        }

        if (current_settings::hidePlayersInCombat && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return false;
        }

        return true;
    }

    if (isPlayerOwned)
    {
        if (!current_settings::playerOwned)
        {
            return false;
        }

        if (current_settings::maxPlayerOwnedVisible > 0 && unpc::numPlayerOwnedVisible >= current_settings::maxPlayerOwnedVisible)
        {
            return false;
        }

        if (current_settings::hidePlayerOwnedSelf && isOwnerLocalPlayer)
        {
            return false;
        }

        if (current_settings::hidePlayerOwned && !isOwnerLocalPlayer)
        {
            return false;
        }

        if (current_settings::hideNonFriendsOwned && !isFriend)
        {
            return false;
        }

        if (current_settings::hideBlockedPlayersOwned && isBlocked)
        {
            return false;
        }

        if (current_settings::hideNonGuildMembersOwned && !(isActiveGuildMember || isGuildMember))
        {
            return false;
        }

        if (current_settings::hideNonGroupMembersOwned && !(isPartyMember || isSquadMember))
        {
            return false;
        }

        if (!isOwnerLocalPlayer && current_settings::hidePlayerOwnedInCombat && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return false;
        }

        return true;
    }

    if (!current_settings::unhideNpcs)
    {
        return false;
    }

    if (current_settings::maxNpcs > 0 && unpc::numNpcsVisible >= current_settings::maxNpcs)
    {
        return false;
    }

    if (rank < current_settings::minimumRank)
    {
        return false;
    }

    if (current_settings::attackable == 1 && !isAttackable)
    {
        return false;
    }
    if (current_settings::attackable == 2 && isAttackable)
    {
        return false;
    }

    if (current_settings::maximumDistance > 0 && distance >= current_settings::maximumDistance)
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
    auto logLevel = logging::LogLevel::Debug;
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

    stopping.store(true, std::memory_order_release);

    if (npcHook)
    {
        npcHook->disable(true);

        while (re::hookInFlight.load(std::memory_order_acquire) != 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
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
