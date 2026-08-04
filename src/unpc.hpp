#ifndef UNHIDENPCS_UNPC_HPP
#define UNHIDENPCS_UNPC_HPP
#pragma once
#include "MumbleLink.hpp"
#include "fw.hpp"
#include "gw2.hpp"
#include "hotkey.hpp"
#include "settings.hpp"

namespace unpc
{
    enum class EMode : int32_t
    {
        Unknown  = 0,
        Proxy    = 1,
        Injected = 2,
        Nexus    = 3,
        ArcDps   = 4
    };

    constexpr auto SIGNATURE = 1817724315;

    inline auto mode = EMode::Unknown;

    extern std::atomic_bool nexusPresent;
    extern std::atomic_bool arcDpsPresent;
    extern std::atomic_bool injected;
    extern std::atomic_bool exit;
    extern std::atomic_bool stopping;

    extern std::optional<logging::Logger> logger;
    extern std::optional<Settings>        settings;
    extern std::optional<memory::Detour>  hook;
    extern std::optional<memory::Detour>  hook2;
    extern HotkeyManager                  hotkeyManager;

    extern HANDLE      hMutex;
    extern HMODULE     hModule;
    extern HANDLE      hThread;
    extern std::string proxyModuleName;
    extern HMODULE     hProxyModule;

    extern MumbleLink* mumbleLink;
    extern int32_t*    loadingScreenActive;

    extern uint32_t numPlayersVisible;
    extern uint32_t numPlayersCulled;
    extern uint32_t numMountsVisible;
    extern uint32_t numMountsCulled;
    extern uint32_t numPlayerOwnedVisible;
    extern uint32_t numPlayerOwnedCulled;
    extern uint32_t numNPCVisible;
    extern uint32_t numNPCCulled;

    struct DebugRenderEntry
    {
        vec2        screenPos;
        u32         color;
        const char* text;

        DebugRenderEntry(vec2 screenPos, u32 color, const char* text) : screenPos(screenPos), color(color), text(text)
        {
        }

        DebugRenderEntry(vec2 screenPos, u32 color) : screenPos(screenPos), color(color), text(nullptr) {}

        DebugRenderEntry(vec2 screenPos, const char* text) : screenPos(screenPos), color(0xFF0000FF), text(text) {}

        DebugRenderEntry(vec2 screenPos) : screenPos(screenPos), color(0xFF0000FF), text(nullptr) {}
    };

    extern std::shared_mutex             debugRenderMutex;
    extern std::vector<DebugRenderEntry> debugRenderEntries;

    void onHookTick();

    // used for the debug renderer to display reason why a character is displayed/hidden
    extern const char* reason;

    // This function may be called second, returning true means the character will be forced invisible
    bool shouldHide(const re::CharacterEntry& c, bool instanced);

    // This function is called first, returning true means the character will be forced visible
    // if returning false, it will go on to call shouldHide
    bool shouldShow(const re::CharacterEntry& c, bool instanced);

    void start();

    void stop();

    void entrypoint();

    // Locate asserts/errors with the specified values
    // if lineNumber is equal to SIZE_MAX then it ignores the line number check
    // returns the address of the first instruction (mov)
    bool findAsserts(memory::Module&              module,
                     size_t                       lineNumber,
                     const std::string&           file,
                     const std::string&           message,
                     std::vector<memory::Handle>& results);

    bool findAsserts(memory::Module&              module,
                     const std::string&           file,
                     const std::string&           message,
                     std::vector<memory::Handle>& results);
} // namespace unpc

#endif // UNHIDENPCS_UNPC_HPP
