#include "arcdps.hpp"
#include "../unpc.hpp"
#include "imgui.h"
#include "../ui.hpp"

bool isArcDPS()
{
    HMODULE hModule {};
    if (!GetModuleHandleEx(0, "d3d11.dll", &hModule))
    {
        return false;
    }

    const auto check = GetProcAddress(hModule, "arcdps_identifier_export");
    FreeLibrary(hModule);

    return check != nullptr;
}

arcdps_exports* mod_init()
{
    memset(&arc_exports, 0, sizeof(arcdps_exports));
    arc_exports.sig       = unpc::signature;
    arc_exports.imguivers = 18000; // placeholder, ImGui not used
    arc_exports.size      = sizeof(arcdps_exports);
    arc_exports.out_name  = "UnhideNPCs";
    arc_exports.out_build = unpc::version::STRING;

    if (unpc::nexusPresent || unpc::hProxyModule || unpc::injected)
        return &arc_exports;

    arc_exports.imgui = reinterpret_cast<void*>(reinterpret_cast<std::uintptr_t>(ui::renderWindow));

    unpc::start();
    return &arc_exports;
}

uintptr_t mod_release()
{
    if (!unpc::nexusPresent && !unpc::hProxyModule && !unpc::injected)
        unpc::stop();
    return 0;
}

void* get_init_addr(
    char*    arcversion,
    void*    imguictx,
    void*    id3dptr,
    HANDLE   arcdll,
    void*    mallocfn,
    void*    freefn,
    uint32_t d3dversion
)
{
    if (!unpc::nexusPresent && !unpc::hProxyModule && !unpc::injected)
    {
        ImGui::SetCurrentContext(static_cast<ImGuiContext*>(imguictx));
        const auto allocatorFn = reinterpret_cast<void*(*)(size_t, void*)>(reinterpret_cast<std::uintptr_t>(mallocfn));
        const auto freeFn      = reinterpret_cast<void(*)(void*, void*)>(reinterpret_cast<std::uintptr_t>(freefn));
        ImGui::SetAllocatorFunctions(allocatorFn, freeFn);
    }

    return reinterpret_cast<void*>(mod_init);
}

void* get_release_addr()
{
    return reinterpret_cast<void*>(mod_release);
}
