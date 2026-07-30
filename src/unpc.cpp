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
std::optional<Detour>          unpc::hook {};
std::optional<Detour>          unpc::hook2 {};

HotkeyManager unpc::hotkeyManager(
    "ArenaNet_Gr_Window_Class",
    std::filesystem::current_path() / "addons" / "UnhideNPCs" / "hotkeys.json");
HANDLE  unpc::hMutex {};
HMODULE unpc::hModule {};

HANDLE unpc::hThread {};

std::string unpc::proxyModuleName {};
HMODULE     unpc::hProxyModule {};
MumbleLink* unpc::mumbleLink {};
int32_t*    unpc::loadingScreenActive {};

uint32_t unpc::numPlayersVisible {};
uint32_t unpc::numPlayerOwnedVisible {};
uint32_t unpc::numNPCVisible {};
uint32_t unpc::numPlayersInArea {};

std::shared_mutex                  unpc::renderCirclesMutex {};
std::vector<std::pair<vec2, vec4>> unpc::renderCircles {};

#include "re.hpp"

static void hotkeyCallback(const std::string& id)
{
    if (!settings)
    {
        return;
    }

    auto lambda = [id](fw::Settings& settings)
    {
        for (auto& _setting : settings.settings())
        {
            auto matched = false;

            std::visit(
                [&](auto&& setting)
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
                },
                _setting);

            if (matched)
            {
                LOG_INFO("Hotkey triggered: {}", id);
                return true;
            }
        }
        return false;
    };

    if (lambda(*settings) || lambda(settings->profile()))
    {
        return;
    }

    if (id == "ForceVisibility")
    {
        ++re::forceVisibility;
    } else if (id == "ToggleOverlay")
    {
        if (settings->OverlayOpen.get() && hotkeyManager.isCapturing())
        {
            hotkeyManager.stopCapturing();
        }

        settings->OverlayOpen.set(!settings->OverlayOpen.get());
    } else if (id == "SwitchProfile")
    {
        auto idx = settings->ActiveProfile.get();
        ++idx;
        if (idx >= static_cast<int>(settings->children().size()))
        {
            idx = 0;
        }
        if (idx != settings->ActiveProfile.get())
        {
            settings->ActiveProfile.set(idx);
            settings->needSave();
            ++re::forceVisibility;
        }
    }
}

static void initializeHotkeys()
{
    auto lambda = [](fw::Settings& settings)
    {
        for (auto& _setting : settings.settings())
        {
            std::visit(
                [&](auto&& setting)
                {
                    using T = std::decay_t<decltype(*setting)>;
                    if constexpr (!std::is_same_v<T, fw::Setting<bool>>)
                    {
                        return;
                    }

                    hotkeyManager.registerHotkey(setting->name(), setting->description());
                },
                _setting);
        }
    };

    hotkeyManager.registerHotkey("ForceVisibility", "Force Visibility");
    hotkeyManager.registerHotkey("ToggleOverlay", "Alt. Overlay Toggle");
    hotkeyManager.registerHotkey("SwitchProfile", "Switches to the next profile");

    lambda(*settings);
    lambda(settings->profile());

    hotkeyManager.registerCallback(hotkeyCallback);
    hotkeyManager.load();
}

static bool initialize()
{
    if (!mumbleLink)
    {
        mumbleLink = mode == EMode::Nexus ? nexus::getMumbleLink() : getMumbleLink();
        if (!mumbleLink)
        {
            LOG_ERR("MumbleLink Failed");
            return false;
        }
    }

    settings.emplace(std::filesystem::current_path() / "addons" / "UnhideNPCs" / "settings.json");
    if (!settings->loaded())
    {
        LOG_ERR("Settings Failed");
        return false;
    }

    if (auto& profile = settings->profile(); profile.HideOffScreen.get())
    {
        if (!profile.UnhidePlayers.get())
            profile.UnhidePlayers.set(true);
        if (!profile.UnhideNPCs.get())
            profile.UnhideNPCs.set(true);
        if (!profile.PlayerOwned.get())
            profile.PlayerOwned.set(true);
    }

    if (settings->ForceConsole.get())
    {
        logger->setConsole(true);
    }

    Module game {};
    if (!Module::tryGetByName("Gw2-64.exe", game))
    {
        game = Module::getMain();
        LOG_ERR("Unable to find \"Gw2-64.exe\", using fallback module: \"{}\"", game.name());
    }

    Handle  ptr {};
    RefData ref {};

    auto pattern1 = Pattern("pattern1", re::pattern1).callback(
        [&](const Handle& ptr)
        {
            re::vtableIndex = ptr.add(2).deref<uint32_t>() / 8;
            LOG_DBG("VTable Index: {}", re::vtableIndex);
            return true;
        });

    if (!pattern1.resolve(game, ptr))
    {
        return false;
    }

    auto pattern2 = Pattern("pattern2", re::pattern2).add(10).relCall();
    if (!pattern2.resolve(game, ptr))
    {
        return false;
    }
    hook.emplace("Hook 1", ptr.to_ptr<void*>(), reinterpret_cast<void*>(re::hook));

    auto pattern3 = Pattern("pattern3", re::pattern3).callback(
        [&](Handle& ptr)
        {
            ptr                 = ptr.add(5);
            loadingScreenActive = ptr.add(6).add(ptr.add(2).deref<int32_t>()).to_ptr<int32_t*>();
            ptr                 = Handle(loadingScreenActive);
            LOG_DBG("Loading screen active: {}", ptr.formatted());
            return true;
        });
    if (!pattern3.resolve(game, ptr))
    {
        return false;
    }

    std::vector<Handle> asserts {};
    if (!findAsserts(
        game,
        std::get<0>(re::pattern7_1),
        std::get<1>(re::pattern7_1),
        std::get<2>(re::pattern7_1),
        asserts) && !findAsserts(
        game,
        std::get<0>(re::pattern7_2),
        std::get<1>(re::pattern7_2),
        std::get<2>(re::pattern7_2),
        asserts))
    {
        LOG_ERR("Unable to find pattern 7");
        return false;
    }
    re::gw2::getContextCollection = reinterpret_cast<re::gw2::GetContextCollectionFn>(asserts[0].sub(13).
        resolve_relative_call().raw());

    if (!game.findStringReference(re::pattern5, ref))
    {
        LOG_ERR("Unable to find pattern 5");
        return false;
    }
    re::gw2::getAvContext = reinterpret_cast<re::gw2::GetAvContextFn>(ref.instruction().add(12).resolve_relative_call().
                                                                          raw());

    if (!findAsserts(
        game,
        std::get<0>(re::pattern8_1),
        std::get<1>(re::pattern8_1),
        std::get<2>(re::pattern8_1),
        asserts) && !findAsserts(
        game,
        std::get<0>(re::pattern8_2),
        std::get<1>(re::pattern8_2),
        std::get<2>(re::pattern8_2),
        asserts))
    {
        LOG_ERR("Unable to find pattern 8");
        return false;
    }
    if (!game.findFunctionStart(asserts[0], ptr))
    {
        LOG_ERR("Unable to find pattern 8 [2]");
        return false;
    }
    hook2.emplace("Hook 2", ptr.to_ptr<void*>(), reinterpret_cast<void*>(re::hook2));

    if (!hook->enable())
    {
        LOG_ERR("Failed to enable hook");
        return false;
    }

    if (!hook2->enable())
    {
        LOG_ERR("Failed to enable hook");
        return false;
    }

    initializeHotkeys();
    game.clear();

    return true;
}

void unpc::onHookTick()
{
    if (exit)
    {
        return;
    }

    if (!settings || !settings->loaded())
    {
        return;
    }

    hotkeyManager.update();
    settings->save();

    if (mode != EMode::Injected && mode != EMode::Proxy)
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
            ui::d3dHook = hooks::D3D11::create(
                "ArenaNet_Gr_Window_Class",
                "",
                ui::onD3DPresent,
                ui::onD3DResizeBuffers,
                ui::onD3DStarted,
                ui::onD3DShutdown);

            if (!ui::d3dHook || !ui::d3dHook.value()->enable())
            {
                LOG_INFO("Overlay Not OK");
            } else
            {
                LOG_INFO("Overlay OK");
            }
        }
    }
}

bool unpc::shouldHide(const re::CharacterEntry& c, const bool instanced)
{
    if (!settings)
    {
        return false;
    }

    const auto& profile = settings->profile();

    if (!c.isOnScreen && profile.HideOffScreen.get())
    {
        return true;
    }

    if (c.isTarget && profile.AlwaysShowTarget.get())
    {
        return false;
    }

    if (c.isPlayer)
    {
        const auto maxVisible = profile.MaxPlayersVisible.get();
        if (maxVisible > 0 && numPlayersVisible >= static_cast<uint32_t>(maxVisible))
        {
            return true;
        }

        if (profile.HidePlayers.get())
        {
            return true;
        }

        if (profile.HideNonFriends.get() && !c.isFriend)
        {
            return true;
        }

        if (profile.HideBlockedPlayers.get() && c.isBlocked)
        {
            return true;
        }

        if (profile.HideNonGuildMembers.get() && !(c.isActiveGuildMember || c.isGuildMember))
        {
            return true;
        }

        const bool groupMember = c.isPartyMember || c.isSquadMember;
        if (profile.HideNonGroupMembers.get() && !groupMember)
        {
            return true;
        }

        if (profile.HideStrangers.get() && !c.isFriend && !c.isPartyMember && !c.isSquadMember)
        {
            return true;
        }

        if (profile.HidePlayersInCombat.get() && mumbleLink && mumbleLink->getContext().isInCombat())
        {
            return true;
        }

        return false;
    }

    if (c.isPlayerOwned)
    {
        const auto maxVisible = profile.MaxPlayerOwnedVisible.get();
        if (maxVisible > 0 && numPlayerOwnedVisible >= static_cast<uint32_t>(maxVisible))
        {
            return true;
        }

        if (profile.HideOwnedInstances.get() && instanced && !c.isOwnerLocalPlayer)
        {
            return true;
        }

        if (c.isOwnerLocalPlayer && profile.HidePlayerOwnedSelf.get())
        {
            return true;
        }

        if (profile.HidePlayerOwned.get() && !c.isOwnerLocalPlayer)
        {
            return true;
        }

        if (profile.HideNonFriendsOwned.get() && !c.isFriend)
        {
            return true;
        }

        if (profile.HideBlockedPlayersOwned.get() && c.isBlocked)
        {
            return true;
        }

        if (profile.HideNonGuildMembersOwned.get() && !(c.isActiveGuildMember || c.isGuildMember))
        {
            return true;
        }

        const bool groupMember = c.isPartyMember || c.isSquadMember;
        if (profile.HideNonGroupMembersOwned.get() && !groupMember)
        {
            return true;
        }

        if (profile.HideStrangersOwned.get() && !c.isOwnerLocalPlayer && !c.isFriend && !c.isPartyMember && !c.
            isSquadMember)
        {
            return true;
        }

        if (!c.isOwnerLocalPlayer && profile.HidePlayerOwnedInCombat.get() && mumbleLink && mumbleLink->getContext().
            isInCombat())
        {
            return true;
        }

        return false;
    }

    const auto maxVisible = profile.MaxNpcs.get();
    if (maxVisible > 0 && numNPCVisible >= static_cast<uint32_t>(maxVisible))
    {
        return true;
    }

    return false;
}

bool unpc::shouldShow(const re::CharacterEntry& c, const bool instanced)
{
    if (!settings)
    {
        return false;
    }

    const auto& profile = settings->profile();

    if (!c.isOnScreen && profile.HideOffScreen.get())
    {
        return false;
    }

    if (c.isTarget && profile.AlwaysShowTarget.get())
    {
        return true;
    }

    if (c.isPlayer)
    {
        if (!profile.UnhidePlayers.get())
        {
            return false;
        }

        const auto maxVisible = profile.MaxPlayersVisible.get();
        if (maxVisible > 0 && numPlayersVisible >= static_cast<uint32_t>(maxVisible))
        {
            return false;
        }

        if (profile.HidePlayers.get())
        {
            return false;
        }

        if (profile.HideNonFriends.get() && !c.isFriend)
        {
            return false;
        }

        if (profile.HideBlockedPlayers.get() && c.isBlocked)
        {
            return false;
        }

        if (profile.HideNonGuildMembers.get() && !(c.isActiveGuildMember || c.isGuildMember))
        {
            return false;
        }

        const bool groupMember = c.isPartyMember || c.isSquadMember;
        if (profile.HideNonGroupMembers.get() && !groupMember)
        {
            return false;
        }

        if (profile.HideStrangers.get() && !c.isFriend && !c.isPartyMember && !c.isSquadMember)
        {
            return false;
        }

        if (profile.HidePlayersInCombat.get() && mumbleLink && mumbleLink->getContext().isInCombat())
        {
            return false;
        }

        return true;
    }

    if (c.isPlayerOwned)
    {
        if (!profile.PlayerOwned.get())
        {
            return false;
        }

        const auto maxVisible = profile.MaxPlayerOwnedVisible.get();
        if (maxVisible > 0 && numPlayerOwnedVisible >= static_cast<uint32_t>(maxVisible))
        {
            return false;
        }

        if (profile.HideOwnedInstances.get() && instanced && !c.isOwnerLocalPlayer)
        {
            return false;
        }

        if (profile.HidePlayerOwnedSelf.get() && c.isOwnerLocalPlayer)
        {
            return false;
        }

        if (profile.HidePlayerOwned.get() && !c.isOwnerLocalPlayer)
        {
            return false;
        }

        if (profile.HideNonFriendsOwned.get() && !c.isFriend)
        {
            return false;
        }

        if (profile.HideBlockedPlayersOwned.get() && c.isBlocked)
        {
            return false;
        }

        if (profile.HideNonGuildMembersOwned.get() && !(c.isActiveGuildMember || c.isGuildMember))
        {
            return false;
        }

        const bool groupMember = c.isPartyMember || c.isSquadMember;
        if (profile.HideNonGroupMembersOwned.get() && !groupMember)
        {
            return false;
        }

        if (profile.HideStrangersOwned.get() && !c.isOwnerLocalPlayer && !c.isFriend && !c.isPartyMember && !c.
            isSquadMember)
        {
            return false;
        }

        if (!c.isOwnerLocalPlayer && profile.HidePlayerOwnedInCombat.get() && mumbleLink && mumbleLink->getContext().
            isInCombat())
        {
            return false;
        }

        return true;
    }

    if (!profile.UnhideNPCs.get())
    {
        return false;
    }

    const auto maxVisible = profile.MaxNpcs.get();
    if (maxVisible > 0 && numNPCVisible >= static_cast<uint32_t>(maxVisible))
    {
        return false;
    }

    if (c.rank < profile.MinimumRank.get())
    {
        return false;
    }

    if (profile.Attackable.get() == 1 && !c.isAttackable)
    {
        return false;
    }
    if (profile.Attackable.get() == 2 && c.isAttackable)
    {
        return false;
    }

    if (profile.MaximumDistance.get() > 0 && c.distance >= profile.MaximumDistance.get())
    {
        return false;
    }

    return true;
}

void unpc::start()
{
    const auto start = std::chrono::high_resolution_clock::now();

    if (settings && logger && hook)
    {
        return;
    }

#ifdef BUILDING_ON_GITHUB
    auto logLevel = logging::LogLevel::Info;
#else
    auto logLevel = logging::LogLevel::Debug;
#endif

    logger.emplace(
        "UnhideNPCs",
        std::filesystem::current_path() / "addons" / "UnhideNPCs" / "unhidenpcs.log",
        logLevel);

    LOG_INFO("Starting");
    LOG_INFO("Version {}", version::STRING);

    if (injected)
    {
        LOG_INFO("Mode: Injected");
        mode = EMode::Injected;
    } else if (hProxyModule)
    {
        LOG_INFO("Mode: Proxy({})", proxyModuleName);
        mode = EMode::Proxy;
    } else if (nexusPresent && util::isModuleInAnyDirsRelativeToExe(hModule, { "addons" }))
    {
        LOG_INFO("Mode: Nexus");
        mode = EMode::Nexus;
        logger->registerCallback(nexus::logCallback);
    } else if (arcDpsPresent && util::isModuleInAnyDirsRelativeToExe(hModule, { "", "bin64" }))
    {
        LOG_INFO("Mode: ArcDPS");
        mode = EMode::ArcDps;
    } else
    {
        LOG_INFO("Mode: Unknown");
        mode = EMode::Unknown;
        exit = true;
        return;
    }

    const auto result   = initialize();
    const auto end      = std::chrono::high_resolution_clock::now();
    auto       duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    if (!result)
    {
        LOG_ERR("Initialization failed");
        exit = true;
        return;
    }

    LOG_INFO("Started in {}ms", duration);
}

void unpc::stop()
{
    LOG_INFO("Stopping");

    // force visibility before exit to prevent leaving characters permanently invisible
    ++re::forceVisibility;
    stopping.store(true, std::memory_order_release);

    while (re::forceVisibility > 0)
    {
        ;
    }

    if (ui::d3dHook)
    {
        ui::d3dHook.reset();
    }

    if (hook)
    {
        if (hook->enabled())
        {
            hook->disable();

            while (re::hookInFlight.load(std::memory_order_acquire) != 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    if (hook2)
    {
        if (hook2->enabled())
        {
            hook2->disable();
        }
    }

    MH_Uninitialize();

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
        nullptr);
}

bool unpc::findAsserts(
    Module&              module,
    const size_t         lineNumber,
    const std::string&   file,
    const std::string&   message,
    std::vector<Handle>& results)
{
    if (lineNumber != SIZE_MAX)
    {
        LOG_DBG("Looking for asserts [0x{:X}, \"{}\", \"{}\"]", lineNumber, file, message);
    } else
    {
        LOG_DBG("Looking for asserts [\"{}\", \"{}\"]", file, message);
    }
    results.clear();

    std::vector<RefData> fileRefs {};
    std::vector<RefData> messageRefs {};

    if (!module.findStringReferences(file, fileRefs))
    {
        return false;
    }

    if (!module.findStringReferences(message, messageRefs))
    {
        return false;
    }

    for (const auto& messageRefData : messageRefs)
    {
        for (const auto& fileRefData : fileRefs)
        {
            const auto& fileRef    = fileRefData.instruction();
            const auto& messageRef = messageRefData.instruction();

            // 7 bytes before the reference to the message should be a reference to the file
            if (messageRef.sub(7) != fileRef)
            {
                continue;
            }

            // the subsequent instruction should be a call
            if (messageRef.add(7).deref<uint8_t>() != 0xE8)
                continue;

            // the preceding instruction should be a mov, matching the number
            const auto mov = fileRef.sub(5);
            if (mov.deref<uint8_t>() != 0xB8)
                continue;
            if (lineNumber != SIZE_MAX && mov.add(1).deref<i32>() != lineNumber)
                continue;

            // check if mov is rex prefixed
            if ((mov.sub(1).deref<uint8_t>() & 0xF0) == 0x40)
                results.push_back(mov.sub(1));
            else
                results.push_back(mov);
        }
    }

    LOG_DBG("Found {} asserts", results.size());
    return !results.empty();
}

bool unpc::findAsserts(
    Module&              module,
    const std::string&   file,
    const std::string&   message,
    std::vector<Handle>& results)
{
    return findAsserts(module, SIZE_MAX, file, message, results);
}
