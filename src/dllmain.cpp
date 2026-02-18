#include <algorithm>

#include "pch.hpp"
#include "unpc.hpp"
#include "fw/proxy.hpp"
#include "proxy/dxgi.hpp"
#include "proxy/midimap.hpp"
#include "proxy/d3d9.hpp"

#include "integration/arcdps.hpp"
#include "integration/nexus.hpp"

bool isInGameFolder(const HMODULE hModule)
{
    std::filesystem::path exePath, dllPath;
    if (!util::getModuleFilePath(nullptr, exePath) || !util::getModuleFilePath(hModule, dllPath))
    {
        return false;
    }

    exePath = exePath.parent_path();
    dllPath = dllPath.parent_path();

    // allowed folders relative to exe
    const std::filesystem::path allowed[] = { exePath, exePath / "addons", exePath / "bin64" };

    return std::any_of(
        std::begin(allowed),
        std::end(allowed),
        [&](const std::filesystem::path& p)
        {
            return dllPath == p;
        }
    );
}

BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD ulReasonForCall, PVOID)
{
    if (ulReasonForCall == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        unpc::hModule = hModule;

        unpc::nexusPresent  = nexus::isNexus();
        unpc::arcDpsPresent = !unpc::nexusPresent && isArcDps();
        unpc::injected      = !isInGameFolder(hModule);

        // make sure we are the only instance of UnhideNPCs that is loaded
        if (!util::checkMutex("UnhideNPCsMutex", unpc::hMutex))
        {
            util::closeHandle(unpc::hMutex);
            return TRUE;
        }

        // attempts to determine if we are performing as a proxy dll
        // if we are, then we call LoadLibrary for our own module to
        // increase refcount, otherwise we might get unloaded prematurely
        if (proxy::check({ "dxgi.dll", "midimap.dll", "d3d9.dll" }, unpc::proxyModuleName))
        {
            if (!GetModuleHandleEx(0, unpc::proxyModuleName.c_str(), &unpc::hProxyModule))
            {
                return FALSE;
            }

            unpc::entrypoint();
            return TRUE;
        }

        if (!unpc::injected && (unpc::nexusPresent || unpc::arcDpsPresent))
        {
            return TRUE;
        }

        unpc::entrypoint();
    }

    return TRUE;
}
