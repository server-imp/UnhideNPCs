#include "unpc.hpp"

using namespace std::chrono_literals;
using namespace memory;

int32_t unpc::signature = -1374920153;

int16_t unpc::version::year  = 2025;
int16_t unpc::version::month = 10;
int16_t unpc::version::day   = 20;
int16_t unpc::version::build = 1;

HANDLE      unpc::hMutex{};
HMODULE     unpc::hModule{};
HANDLE      unpc::hThread{};
std::string unpc::proxyModuleName{};
HMODULE     unpc::hProxyModule{};

bool unpc::loadedByNexus{};
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

    if (unpc::settings->getForceConsole())
        unpc::logger->setConsole(true);

    module game{};
    if (!module::tryGetByName("Gw2-64.exe", game))
    {
        return false;
    }

    handle pointer{};
    unpc::logger->setLevel(logging::LogLevel::Info);
    if (!game.find_pattern(re::pattern, pointer))
    {

        unpc::logger->setLevel(logging::LogLevel::Debug);
        LOG_DBG("Unable to find pattern");
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

void unpc::entrypoint()
{
    if (hThread)
        return;

    // need to use CreateThread because for some reason std::thread doesn't allow the dll to unload cleanly
    hThread = CreateThread
    (
        nullptr,
        0,
        [](PVOID) -> DWORD
        {
            logger.emplace
            (
                "UnhideNPCs",
                std::filesystem::current_path() / "addons" / "UnhideNPCs" / "log.txt",
                logging::LogLevel::Debug
            );

            LOG_DBG("Version {}.{}.{}.{}", version::year, version::month, version::day, version::build);
            LOG_DBG("Built on {} at {}", __DATE__, __TIME__);

            if (hProxyModule)
            {
                LOG_DBG("Proxy mode: {}", proxyModuleName);
            }

            // if we aren't in the game folder we are most likely injected by a dll injector
            if (!isInGameFolder(hModule))
            {
                injected = true;
                logger->setConsole(true);
                LOG_DBG("Injected = true");
            }
            else
            {
                LOG_DBG("Injected = false");
            }

            if (!initialize())
            {
                exit = true;
            }

            while (!exit)
            {
                if (injected && GetAsyncKeyState(VK_END))
                {
                    exit = true;
                    break;
                }

                Sleep(25);
            }

            LOG_DBG("Exiting");

            if (npcHook)
                npcHook->disable(true);

            npcHook.reset();
            settings.reset();
            logger.reset();

            util::closeHandle(hMutex);
            util::closeHandle(hThread);

            // if we are not a proxy module and not loaded by nexus, free our own module and exit the thread
            if (!hProxyModule && !loadedByNexus)
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
