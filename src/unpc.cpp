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

bool unpc::unloadOverlay {};

#include "re.hpp"

void hotkeyCallback(const std::string& id)
{
    if (!unpc::settings)
    {
        return;
    }

    auto lambda = [id](fw::Settings& settings)
    {
        for (auto& _setting : settings.settings())
        {
            bool matched = false;

            std::visit([&](auto&& setting)
            {
                using T = std::decay_t<decltype(*setting)>;
                if constexpr (!std::is_same_v<T, fw::Setting<bool>>)
                {
                    return;
                }
                if (id == setting->name())
                {
                    setting->set(!setting->get());
                    matched = true;
                }
            }, _setting);

            if (matched)
            {
                LOG_INFO("Hotkey triggered: {}", id);
                return true;
            }
        }
        return false;
    };

    if (lambda(*unpc::settings) || lambda(unpc::settings->profile()))
    {
        return;
    }

    if (id == "ForceVisibility")
    {
        ++re::forceVisibility;
    }
    else if (id == "ToggleOverlay")
    {
        if (unpc::settings->OverlayOpen.get() && hotkeyManager.isCapturing())
        {
            hotkeyManager.stopCapturing();
        }

        unpc::settings->OverlayOpen.set(!unpc::settings->OverlayOpen.get());
    }
    else if (id == "SwitchProfile")
    {
        auto idx = unpc::settings->ActiveProfile.get();
        ++idx;
        if (idx >= static_cast<int>(unpc::settings->children().size()))
        {
            idx = 0;
        }
        if (idx != unpc::settings->ActiveProfile.get())
        {
            unpc::settings->ActiveProfile.set(idx);
            unpc::settings->needSave();
            ++re::forceVisibility;
        }
    }
}

void initializeHotkeys()
{
    auto lambda = [](fw::Settings& settings)
    {
        for (auto& _setting : settings.settings())
        {
            std::visit([&](auto&& setting)
            {
                using T = std::decay_t<decltype(*setting)>;
                if constexpr (!std::is_same_v<T, fw::Setting<bool>>)
                {
                    return;
                }

                hotkeyManager.registerHotkey(setting->name(), setting->description());
            }, _setting);
        }
    };

    hotkeyManager.registerHotkey("ForceVisibility", "Force Visibility");
    hotkeyManager.registerHotkey("ToggleOverlay", "Alt. Overlay Toggle");
    hotkeyManager.registerHotkey("SwitchProfile", "Switches to the next profile");

    lambda(*unpc::settings);
    lambda(unpc::settings->profile());

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

    settings.emplace(std::filesystem::current_path() / "addons" / "UnhideNPCs" / "settings.json");

    if (unpc::settings->ForceConsole.get())
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

    hotkeyManager.update();

    if (mode != EMode::Injected && mode != EMode::Proxy)
    {
        return;
    }

    if (!settings)
        return;

    settings->save();

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
    if (!settings)
    {
        return false;
    }

    const auto& profile = settings->profile();

    if (isTarget && profile.AlwaysShowTarget.get())
    {
        return false;
    }

    if (isPlayer)
    {
        if (profile.MaxPlayersVisible.get() > 0 && unpc::numPlayersVisible > static_cast<uint32_t>(profile.MaxPlayersVisible.get()))
        {
            return true;
        }

        if (profile.HidePlayers.get())
        {
            return true;
        }

        if (profile.HideNonFriends.get() && !isFriend)
        {
            return true;
        }

        if (profile.HideBlockedPlayers.get() && isBlocked)
        {
            return true;
        }

        if (profile.HideNonGuildMembers.get() && !(isActiveGuildMember || isGuildMember))
        {
            return true;
        }

        const bool groupMember = isPartyMember || isSquadMember;
        if (profile.HideNonGroupMembers.get() && !groupMember)
        {
            return true;
        }

        if (profile.HideStrangers.get() && !isFriend && !isPartyMember && !isSquadMember)
        {
            return true;
        }

        if (profile.HidePlayersInCombat.get() && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return true;
        }

        return false;
    }

    if (isPlayerOwned)
    {
        if (profile.MaxPlayerOwnedVisible.get() > 0
            && unpc::numPlayerOwnedVisible > static_cast<uint32_t>(profile.MaxPlayerOwnedVisible.get()))
        {
            return true;
        }

        if (isOwnerLocalPlayer && profile.HidePlayerOwnedSelf.get())
        {
            return true;
        }

        if (profile.HidePlayerOwned.get() && !isOwnerLocalPlayer)
        {
            return true;
        }

        if (profile.HideNonFriendsOwned.get() && !isFriend)
        {
            return true;
        }

        if (profile.HideBlockedPlayersOwned.get() && isBlocked)
        {
            return true;
        }

        if (profile.HideNonGuildMembersOwned.get() && !(isActiveGuildMember || isGuildMember))
        {
            return true;
        }

        const bool groupMember = isPartyMember || isSquadMember;
        if (profile.HideNonGroupMembersOwned.get() && !groupMember)
        {
            return true;
        }

        if (profile.HideStrangersOwned.get() && !isOwnerLocalPlayer && !isFriend && !isPartyMember && !isSquadMember)
        {
            return true;
        }

        if (!isOwnerLocalPlayer && profile.HidePlayerOwnedInCombat.get() && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return true;
        }

        return false;
    }

    if (profile.MaxNpcs.get() > 0 && unpc::numNpcsVisible > static_cast<uint32_t>(profile.MaxNpcs.get()))
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
    if (!settings)
    {
        return false;
    }

    const auto& profile = settings->profile();

    if (isTarget && profile.AlwaysShowTarget.get())
    {
        return true;
    }

    if (isPlayer)
    {
        if (!profile.UnhidePlayers.get())
        {
            return false;
        }

        if (profile.MaxPlayersVisible.get() > 0 && unpc::numPlayersVisible >= static_cast<uint32_t>(profile.MaxPlayersVisible.get()))
        {
            return false;
        }

        if (profile.HidePlayers.get())
        {
            return false;
        }

        if (profile.HideNonFriends.get() && !isFriend)
        {
            return false;
        }

        if (profile.HideBlockedPlayers.get() && isBlocked)
        {
            return false;
        }

        if (profile.HideNonGuildMembers.get() && !(isActiveGuildMember || isGuildMember))
        {
            return false;
        }

        const bool groupMember = isPartyMember || isSquadMember;
        if (profile.HideNonGroupMembers.get() && !groupMember)
        {
            return false;
        }

        if (profile.HideStrangers.get() && !isFriend && !isPartyMember && !isSquadMember)
        {
            return false;
        }

        if (profile.HidePlayersInCombat.get() && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return false;
        }

        return true;
    }

    if (isPlayerOwned)
    {
        if (!profile.PlayerOwned.get())
        {
            return false;
        }

        if (profile.MaxPlayerOwnedVisible.get() > 0
            && unpc::numPlayerOwnedVisible >= static_cast<uint32_t>(profile.MaxPlayerOwnedVisible.get()))
        {
            return false;
        }

        if (profile.HidePlayerOwnedSelf.get() && isOwnerLocalPlayer)
        {
            return false;
        }

        if (profile.HidePlayerOwned.get() && !isOwnerLocalPlayer)
        {
            return false;
        }

        if (profile.HideNonFriendsOwned.get() && !isFriend)
        {
            return false;
        }

        if (profile.HideBlockedPlayersOwned.get() && isBlocked)
        {
            return false;
        }

        if (profile.HideNonGuildMembersOwned.get() && !(isActiveGuildMember || isGuildMember))
        {
            return false;
        }

        const bool groupMember = isPartyMember || isSquadMember;
        if (profile.HideNonGroupMembersOwned.get() && !groupMember)
        {
            return false;
        }

        if (profile.HideStrangersOwned.get() && !isOwnerLocalPlayer && !isFriend && !isPartyMember && !isSquadMember)
        {
            return false;
        }

        if (!isOwnerLocalPlayer && profile.HidePlayerOwnedInCombat.get() && unpc::mumbleLink && unpc::mumbleLink->getContext().isInCombat())
        {
            return false;
        }

        return true;
    }

    if (!profile.UnhideNPCs.get())
    {
        return false;
    }

    if (profile.MaxNpcs.get() > 0 && unpc::numNpcsVisible >= static_cast<uint32_t>(profile.MaxNpcs.get()))
    {
        return false;
    }

    if (rank < profile.MinimumRank.get())
    {
        return false;
    }

    if (profile.Attackable.get() == 1 && !isAttackable)
    {
        return false;
    }
    if (profile.Attackable.get() == 2 && isAttackable)
    {
        return false;
    }

    if (profile.MaximumDistance.get() > 0 && distance >= profile.MaximumDistance.get())
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
