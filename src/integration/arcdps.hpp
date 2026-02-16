#ifndef UNHIDENPCS_ARCDPS_HPP
#define UNHIDENPCS_ARCDPS_HPP
#pragma once
#include "../pch.hpp"

bool isArcDPS();

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

inline arcdps_exports arc_exports {};

arcdps_exports* mod_init();

uintptr_t mod_release();

extern "C" __declspec(dllexport) void* get_init_addr(
    char*    arcversion,
    void*    imguictx,
    void*    id3dptr,
    HANDLE   arcdll,
    void*    mallocfn,
    void*    freefn,
    uint32_t d3dversion
);

extern "C" __declspec(dllexport) void* get_release_addr();

#endif //UNHIDENPCS_ARCDPS_HPP
