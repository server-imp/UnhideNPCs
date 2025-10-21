#ifndef UNHIDENPCS_ARCDPS_HPP
#define UNHIDENPCS_ARCDPS_HPP
#pragma once
#include "pch.hpp"
#include "unpc.hpp"

inline bool isArcDPS()
{
    const HMODULE hModule = LoadLibraryA("d3d11.dll");
    if (!hModule)
        return false;

    const auto check = GetProcAddress(hModule, "arcdps_identifier_export");
    FreeLibrary(hModule);

    return check != nullptr;
}

typedef struct arcdps_exports
{
    uint64_t    size;      // [required]
    uint32_t    sig;       // [required]
    uint32_t    imguivers; // [required]
    const char* out_name;  // [required]
    const char* out_build; // [required]
    void*       wnd_nofilter;
    void*       combat;
    void*       imgui; // keep as required, but can be null
    void*       options_tab;
    void*       combat_local;
    void*       wnd_filter;
    void*       options_windows;
} arcdps_exports;

inline arcdps_exports arc_exports{};

inline arcdps_exports* mod_init()
{
    memset(&arc_exports, 0, sizeof(arcdps_exports));
    arc_exports.sig       = unpc::signature;
    arc_exports.imguivers = 18000; // placeholder, ImGui not used
    arc_exports.size      = sizeof(arcdps_exports);
    arc_exports.out_name  = "UnhideNPCs";
    arc_exports.out_build = unpc::version::string;

    if (unpc::loadedByNexus)
        return &arc_exports;

    unpc::start();
    return &arc_exports;
}

inline uintptr_t mod_release()
{
    if (!unpc::loadedByNexus)
        unpc::stop();
    return 0;
}

extern "C" inline __declspec(dllexport) void* get_init_addr
(char* arcversion, void* imguictx, void* id3dptr, HANDLE arcdll, void* mallocfn, void* freefn, uint32_t d3dversion)
{
    return reinterpret_cast<void*>(mod_init);
}

extern "C" inline __declspec(dllexport) void* get_release_addr()
{
    return reinterpret_cast<void*>(mod_release);
}

#endif //UNHIDENPCS_ARCDPS_HPP
