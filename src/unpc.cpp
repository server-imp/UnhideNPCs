#include "unpc.hpp"
#include "ui.hpp"

#include "integration/nexus.hpp"

using namespace std::chrono_literals;
using namespace memory;

auto unpc::mode = eMode::Unknown;

HANDLE      unpc::hMutex{};
HMODULE     unpc::hModule{};
HANDLE      unpc::hThread{};
std::string unpc::proxyModuleName{};
HMODULE     unpc::hProxyModule{};
MumbleLink* unpc::mumbleLink{};
int32_t*    unpc::loadingScreenActive{};

bool unpc::nexusPresent{};
bool unpc::arcDpsPresent{};
bool unpc::injected{};
bool unpc::exit{};

std::optional<logging::Logger> unpc::logger;
std::optional<Settings>        unpc::settings;
std::optional<detour>          npcHook{};

#include "re.hpp"

bool initialize()
{
    LOG_DBG("Beginning initialization");

    if (!unpc::mumbleLink)
    {
        unpc::mumbleLink = unpc::mode == unpc::eMode::Nexus ? nexus::getMumbleLink() : getMumbleLink();
        if (!unpc::mumbleLink)
        {
            LOG_DBG("Failed to get MumbleLink");
            return false;
        }
    }

    unpc::settings.emplace(std::filesystem::current_path() / "addons" / "UnhideNPCs" / "config.cfg");
    if (!unpc::settings || !unpc::settings->loaded())
        return false;

    const auto minRank = unpc::settings->getMinimumRank();
    if (minRank < 0 || minRank >= static_cast<int32_t>(re::gw2::eCharacterRank_MAX))
        unpc::settings->setMinimumRank(0);

    const auto attitudeMode = unpc::settings->getAttackable();
    if (attitudeMode < 0 || attitudeMode > 2)
        unpc::settings->setAttackable(0);

    const auto maximumDistance = unpc::settings->getMaximumDistance();
    if (maximumDistance < 0.0f || maximumDistance > 1000.0f)
        unpc::settings->setMaximumDistance(0.0f);

    if (unpc::settings->getForceConsole())
        unpc::logger->setConsole(true);

    module game{};
    if (!module::tryGetByName("Gw2-64.exe", game))
    {
        return false;
    }

#ifdef BUILDING_ON_GITHUB
    unpc::logger->setLevel(logging::LogLevel::Info);
#endif

    handle pointer{};
    if (!game.find_pattern(re::pattern1, pointer))
    {
        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern 1");
        return false;
    }
    re::vtableIndex = pointer.add(2).deref<uint32_t>() / 8;
    LOG_DBG("VTable Index: {}", re::vtableIndex);

    if (!game.find_pattern(re::pattern2, pointer))
    {
        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern 2");
        return false;
    }
    pointer = pointer.add(10).resolve_relative_call();
    LOG_DBG("Resolved call to {}+{:X}", game.name(), pointer.sub(game.start()).raw());
    npcHook.emplace("Hook", pointer.to_ptr<void*>(), re::Hook);

    if (!game.find_pattern(re::pattern3, pointer))
    {
        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern 3");
        return false;
    }
    unpc::loadingScreenActive = pointer.add(6).add(pointer.add(2).deref<int32_t>()).to_ptr<int32_t*>();
    LOG_DBG("Loading screen active: {:08X}", reinterpret_cast<uintptr_t>(unpc::loadingScreenActive));

#ifdef BUILDING_ON_GITHUB
    unpc::logger->setLevel(logging::LogLevel::Debug);
#endif

    if (!npcHook->enable())
    {
        LOG_DBG("Hook enable failed");
        return false;
    }

    LOG_DBG("Initialization complete");
    return true;
}

void unpc::start()
{
    if (settings && logger && npcHook)
        return;

    logger.emplace
        ("UnhideNPCs", std::filesystem::current_path() / "addons" / "UnhideNPCs" / "log.txt", logging::LogLevel::Debug);

    LOG_DBG("Version {}", version::string);
    LOG_DBG("Built on {} at {}", __DATE__, __TIME__);

    if (injected)
    {
        logger->setConsole(true);
        LOG_DBG("Mode: Injected");
        mode = eMode::Injected;
    }
    else if (hProxyModule)
    {
        LOG_DBG("Mode: Proxy({})", proxyModuleName);
        mode = eMode::Proxy;
    }
    else if (nexusPresent && util::isModuleInAnyDirsRelativeToExe(hModule, {"addons"}))
    {
        LOG_DBG("Mode: Nexus");
        mode = eMode::Nexus;
    }
    else if (arcDpsPresent && util::isModuleInAnyDirsRelativeToExe(hModule, {"", "bin64"}))
    {
        LOG_DBG("Mode: ArcDPS");
        mode = eMode::ArcDPS;
    }
    else
    {
        LOG_DBG("Mode: Unknown");
        mode = eMode::Unknown;
        exit = true;
        return;
    }

    if (!initialize())
    {
        exit = true;
    }
}

void unpc::stop()
{
    LOG_DBG("Exiting");

    if (npcHook)
    {
        npcHook->disable(true);
        npcHook.reset();
    }

    settings.reset();
    logger.reset();

    if (mumbleLink && mode != eMode::Nexus)
    {
        UnmapViewOfFile(mumbleLink);
        mumbleLink = nullptr;
    }

    util::closeHandle(hMutex);
}

void unpc::entrypoint()
{
    // need to use CreateThread because for some reason std::thread doesn't allow the dll to unload cleanly
    hThread = CreateThread
    (
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
                FreeLibraryAndExitThread(hModule, 0);

            // otherwise just free the proxy module(decrease refcount) if we have one and exit the thread
            util::freeLibrary(hProxyModule);
            ExitThread(0);
        },
        nullptr,
        0,
        nullptr
    );
}
