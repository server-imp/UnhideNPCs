#include "unpc.hpp"

using namespace std::chrono_literals;
using namespace memory;

HANDLE      unpc::hMutex{};
HMODULE     unpc::hModule{};
HANDLE      unpc::hThread{};
std::string unpc::proxyModuleName{};
HMODULE     unpc::hProxyModule{};

bool unpc::loadedByNexus{};
bool unpc::loadedByArcDPS{};
bool unpc::exit{};
bool unpc::injected{};

std::optional<logging::Logger> unpc::logger;
std::optional<Settings>        unpc::settings;
std::optional<detour>          npcHook{};

#include "re.hpp"

bool isInGameFolder(HMODULE hModule);

bool initialize()
{
    LOG_DBG("Beginning initialization");

    unpc::settings.emplace(std::filesystem::current_path() / "addons" / "UnhideNPCs" / "config.cfg");
    if (!unpc::settings || !unpc::settings->loaded())
        return false;

    const auto minRank = unpc::settings->getMinimumRank();
    if (minRank < 0 || minRank >= static_cast<int32_t>(re::gw2::eCharacterRank::MAX))
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

    unpc::logger->setLevel(logging::LogLevel::Info);
    handle pointer{};
    if (!game.find_pattern(re::pattern1, pointer))
    {
        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern 1");
        return false;
    }
    re::vtableIndex = pointer.add(2).deref<uint32_t>() / 8;

    if (!game.find_pattern(re::pattern2, pointer))
    {
        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern 2");
        return false;
    }
    pointer = pointer.add(10).resolve_relative_call();
    LOG_DBG("Resolved call to {}+{:X}", game.name(), pointer.sub(game.start()).raw());

    npcHook.emplace("Hook", pointer.to_ptr<void*>(), re::Hook);
    unpc::logger->setLevel(logging::LogLevel::Debug);
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

    if (loadedByNexus)
    {
        LOG_DBG("Mode: Nexus");
    }
    else if (loadedByArcDPS)
    {
        LOG_DBG("Mode: ArcDPS");
    }
    else if (hProxyModule)
    {
        LOG_DBG("Mode: Proxy({})", proxyModuleName);
    }
    else if (!isInGameFolder(hModule))
    {
        injected = true;
        logger->setConsole(true);
        LOG_DBG("Mode: Injected");
    }
    else
    {
        LOG_DBG("Mode: Unknown??");
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

    util::closeHandle(hMutex);
}

void unpc::entrypoint()
{
    if (loadedByNexus || loadedByArcDPS)
    {
        return;
    }

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
                if (injected && GetAsyncKeyState(VK_END))
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

bool isInGameFolder(const HMODULE hModule)
{
    std::filesystem::path exePath, dllPath;
    if (!util::getModuleFilePath(nullptr, exePath) || !util::getModuleFilePath(hModule, dllPath))
        return false;

    exePath = exePath.parent_path();
    dllPath = dllPath.parent_path();

    // allowed folders relative to exe
    const std::filesystem::path allowed[] = {exePath, exePath / "addons", exePath / "bin64"};

    return std::any_of
    (
        std::begin(allowed),
        std::end(allowed),
        [&](const std::filesystem::path& p)
        {
            return dllPath == p;
        }
    );
}
