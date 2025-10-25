#include "unpc.hpp"
#include "ui.hpp"

#include "integration/nexus.hpp"

using namespace std::chrono_literals;
using namespace memory;

using namespace unpc;

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
    LOG_INFO("Initializing");

    if (!mumbleLink)
    {
        mumbleLink = mode == eMode::Nexus ? nexus::getMumbleLink() : getMumbleLink();
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
    if (minRank < 0 || minRank >= static_cast<int32_t>(re::gw2::eCharacterRank_MAX))
        settings->setMinimumRank(0);

    const auto attitudeMode = settings->getAttackable();
    if (attitudeMode < 0 || attitudeMode > 2)
        settings->setAttackable(0);

    const auto maximumDistance = settings->getMaximumDistance();
    if (maximumDistance < 0.0f || maximumDistance > 1000.0f)
        settings->setMaximumDistance(0.0f);

    if (settings->getForceConsole())
        logger->setConsole(true);

    module game{};
    if (!module::tryGetByName("Gw2-64.exe", game))
    {
        LOG_ERR("Unable to get Gw2-64.exe module");
        return false;
    }
    LOG_INFO("Gw2-64.exe OK");

    handle pointer{};
    if (!game.find_pattern(re::pattern1, pointer))
    {
        LOG_ERR("Unable to find pattern 1");
        return false;
    }
    re::vtableIndex = pointer.add(2).deref<uint32_t>() / 8;
    LOG_DBG("VTable Index: {}", re::vtableIndex);
    LOG_INFO("Pattern 1 OK");

    if (!game.find_pattern(re::pattern2, pointer))
    {
        LOG_ERR("Unable to find pattern 2");
        return false;
    }
    pointer = pointer.add(10).resolve_relative_call();
    LOG_DBG("Resolved call to {}+{:X}", game.name(), pointer.sub(game.start()).raw());
    npcHook.emplace("Hook", pointer.to_ptr<void*>(), re::Hook);
    LOG_INFO("Pattern 2 OK");

    if (!game.find_pattern(re::pattern3, pointer))
    {
        LOG_ERR("Unable to find pattern 3");
        return false;
    }
    loadingScreenActive = pointer.add(6).add(pointer.add(2).deref<int32_t>()).to_ptr<int32_t*>();
    LOG_DBG("Loading screen active: {:08X}", reinterpret_cast<uintptr_t>(loadingScreenActive));
    LOG_INFO("Pattern 3 OK");

    if (!game.find_pattern(re::pattern4, pointer))
    {
        LOG_ERR("Unable to find pattern 4");
        return false;
    }
    re::gw2::getContextCollection = reinterpret_cast<re::gw2::GetContextCollectionFn>(pointer.raw());
    LOG_INFO("Pattern 4 OK");

    if (!scanner::find_string_reference(re::pattern5, pointer))
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

    return true;
}

void unpc::start()
{
    if (settings && logger && npcHook)
        return;

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
        logger->setConsole(true);
        LOG_INFO("Mode: Injected");
        mode = eMode::Injected;
    }
    else if (hProxyModule)
    {
        LOG_INFO("Mode: Proxy({})", proxyModuleName);
        mode = eMode::Proxy;
    }
    else if (nexusPresent && util::isModuleInAnyDirsRelativeToExe(hModule, {"addons"}))
    {
        LOG_INFO("Mode: Nexus");
        mode = eMode::Nexus;
        logger->registerCallback(nexus::logCallback);
    }
    else if (arcDpsPresent && util::isModuleInAnyDirsRelativeToExe(hModule, {"", "bin64"}))
    {
        LOG_INFO("Mode: ArcDPS");
        mode = eMode::ArcDPS;
    }
    else
    {
        LOG_INFO("Mode: Unknown");
        mode = eMode::Unknown;
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

    if (npcHook)
    {
        npcHook->disable(true);
        npcHook.reset();
    }

    if (mode == eMode::Nexus)
    {
        logger->unregisterCallback(nexus::logCallback);
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
