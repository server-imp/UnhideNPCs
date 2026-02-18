#ifndef UNHIDENPCS_SETTINGS_HPP
#define UNHIDENPCS_SETTINGS_HPP
#pragma once
#include "fw/config.hpp"

class Settings : public Config
{
private:
    CONFIG_PROPERTY(
        bool,
        ForceConsole,
        false,
        "Forces the creation of a console window when set to true.\n"
        "Note: If the console window is exited, then the game will exit as well."
    )

    CONFIG_PROPERTY(bool, AlwaysShowTarget, true, "Always show the targeted character, even if it would be hidden.")

    CONFIG_PROPERTY(
        bool,
        PlayerOwned,
        false,
        "NPCs that are owned by players (pets, clones, minis etc) will also be unhidden."
    )

    CONFIG_PROPERTY(int32_t, MinimumRank, 0, "Only NPCs that have at least this rank get unhidden.")

    CONFIG_PROPERTY(
        int32_t,
        Attackable,
        0,
        "Only NPCs that match this get unhidden.\n0: Both\n1: Only Attackable\n2: Only Non-Attackable"
    )

    CONFIG_PROPERTY(
        bool,
        HidePlayers,
        false,
        "Players will be hidden when this is ticked, useful for boosting performance.\n"
        "Their names are still visible, and you can still target them"
    )

    CONFIG_PROPERTY(int32_t, MaxPlayersVisible, 0, "Maximum number of visible players. (0=no limit)")
    CONFIG_PROPERTY(int32_t, MaxPlayerOwnedVisible, 0, "Maximum number of visible player owned NPCs. (0=no limit)")

    CONFIG_PROPERTY(bool, HideNonGuildMembers, false, "Hide any players that aren't mutual guild members")
    CONFIG_PROPERTY(bool, HideNonGuildMembersOwned, false, "Also hide their owned characters (pets, clones etc)")
    CONFIG_PROPERTY(
        bool,
        HideNonGroupMembers,
        false,
        "Hide any players who are not in the same group as you (party, squad)"
    )
    CONFIG_PROPERTY(bool, HideNonGroupMembersOwned, false, "Also hide their owned characters (pets, clones etc)")

    CONFIG_PROPERTY(
        bool,
        HidePlayerOwned,
        false,
        "NPCs that are owned by players (pets, clones, minis etc) will be hidden."
    )

    CONFIG_PROPERTY(bool, HidePlayerOwnedSelf, false, "Also hide NPCs that are owned by you.")

    CONFIG_PROPERTY(
        bool,
        DisableHidingInInstances,
        false,
        "Disables the hiding options while in an instance (Fractals, Dungeons etc.)"
    )

    CONFIG_PROPERTY(float, MaximumDistance, 0, "NPCs within this distance will be unhidden. (0=no distance check)")

    CONFIG_PROPERTY(
        bool,
        LoadScreenBoost,
        false,
        "Speed up loading screens by temporarily limiting number of characters to 0 when one is triggered.\n"
        "Note: This will cause characters to start loading after the loading screen is finished,\n"
        "meaning there will be invisible characters for a bit after loading."
    )

    CONFIG_PROPERTY(bool, ArcDPS_UIOpen, true, "ArcDPS UI Opened/Closed")
    CONFIG_PROPERTY(bool, DisableOverlay, false, "Disable the built in overlay when using Injection or Proxy mode")
    CONFIG_PROPERTY(bool, CloseOnEscape, true, "Close the overlay when Escape is pressed")
    CONFIG_PROPERTY(
        float,
        OverlayFontSize,
        14.0f,
        "The font size used for the overlay\nRequires a restart/reload to reflect changes"
    )

public:
    explicit Settings(const std::filesystem::path& filePath)
        : Config(filePath)
    {
        this->save();
    }
};

#endif //UNHIDENPCS_SETTINGS_HPP
