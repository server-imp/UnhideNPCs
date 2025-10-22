#include "pch.hpp"
#include "unpc.hpp"
#include "fw/proxy.hpp"
#include "proxy/dxgi.hpp"
#include "proxy/midimap.hpp"
#include "proxy/d3d9.hpp"

#include "arcdps.hpp"
#include "nexus.hpp"

BOOL APIENTRY DllMain(const HMODULE hModule, const DWORD ul_reason_for_call, PVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        unpc::hModule = hModule;

        unpc::loadedByNexus  = nexus::isNexus();
        unpc::loadedByArcDPS = !unpc::loadedByNexus && isArcDPS();

        // make sure we are the only instance of UnhideNPCs that is loaded
        if (!util::checkMutex("UnhideNPCsMutex", unpc::hMutex))
        {
            return TRUE;
        }

        if (unpc::loadedByNexus || unpc::loadedByArcDPS)
        {
            return TRUE;
        }

        // attempts to determine if we are performing as a proxy dll
        // if we are, then we call LoadLibrary for our own module to
        // increase refcount, otherwise we might get unloaded prematurely
        if (proxy::check({"dxgi.dll", "midimap.dll", "d3d9.dll"}, unpc::proxyModuleName))
        {
            unpc::hProxyModule = LoadLibraryA(unpc::proxyModuleName.c_str());
        }

        unpc::entrypoint();
    }

    return TRUE;
}
