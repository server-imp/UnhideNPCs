#ifndef UNHIDENPCS_SETTINGS_HPP
#define UNHIDENPCS_SETTINGS_HPP
#pragma once
#include "fw/config.hpp"

class Settings : public Config
{
private:
    // Showing
    CONFIG_PROPERTY(bool, AlwaysShowTarget, true, "Always show the targeted character, even if it would be hidden.")
    CONFIG_PROPERTY(bool, PlayerOwned, false, "Characters that players own (pets, clones, minis etc) will also be unhidden.")
    CONFIG_PROPERTY(bool, UnhideNpcs, true, "Unhide NPCs")
    CONFIG_PROPERTY(int32_t, MinimumRank, 0, "Only NPCs that have at least this rank get unhidden.")
    CONFIG_PROPERTY(int32_t, Attackable, 0, "Only NPCs that match this get unhidden.\n0: Both\n1: Only Attackable\n2: Only Non-Attackable")
    CONFIG_PROPERTY(float, MaximumDistance, 0, "Characters outside of this distance won't be unhidden. (0=unlimited)")

    // Hiding
    CONFIG_PROPERTY(bool, HidePlayers, false, "Hide all players (Not yourself)")
    CONFIG_PROPERTY(bool, HidePlayerOwned, false, "Hide all characters owned by players, except yourself (pets, clones, minis etc)")
    CONFIG_PROPERTY(bool, HideBlockedPlayers, false, "Hide any players that you have blocked")
    CONFIG_PROPERTY(bool, HideBlockedPlayersOwned, false, "Hide any characters that are owned by blocked players (pets, clones, minis etc)")
    CONFIG_PROPERTY(bool, HideNonGroupMembers, false, "Hide any players who are not in the same group as you")
    CONFIG_PROPERTY(bool, HideNonGroupMembersOwned, false, "Hide any characters that are owned by non-group members (pets, clones, minis etc)")
    CONFIG_PROPERTY(bool, HideNonGuildMembers, false, "Hide any players that aren't guild members")
    CONFIG_PROPERTY(bool, HideNonGuildMembersOwned, false, "Hide any characters that are owned by non-guild members (pets, clones, minis etc)")
    CONFIG_PROPERTY(bool, HideNonFriends, false, "Hide any players that aren't friends")
    CONFIG_PROPERTY(bool, HideNonFriendsOwned, false, "Hide any characters that are owned by non-friend players (pets, clones, minis etc)")
    CONFIG_PROPERTY(bool, HidePlayerOwnedSelf, false, "Hide characters that are owned by you")
    CONFIG_PROPERTY(int32_t, MaxPlayersVisible, 0, "Maximum number of visible players. (0=unlimited)")
    CONFIG_PROPERTY(int32_t, MaxPlayerOwnedVisible, 0, "Maximum number of visible player-owned characters. (0=unlimited)")
    CONFIG_PROPERTY(int32_t, MaxNpcs, 0, "Maximum number of visible NPCs. (0=unlimited)")
    CONFIG_PROPERTY(bool, DisableHidingInInstances, false, "Disables the hiding options while in an instance (Fractals, Dungeons etc.)")

    // Misc
    CONFIG_PROPERTY(bool, ForceConsole, false, "Create a console window.\n" "Note: If the console window is exited, then the game will exit as well.")
    CONFIG_PROPERTY(
        bool,
        LoadScreenBoost,
        false,
        "Speed up loading screens by temporarily limiting number of characters to 0 when one is triggered.\n"
        "Note: This will cause characters to start loading after the loading screen is finished,\n"
        "meaning there will be invisible characters for a bit after loading."
    )
    CONFIG_PROPERTY(bool, CloseOnEscape, true, "Close the overlay when Escape is pressed")
    CONFIG_PROPERTY(float, OverlayFontSize, 14.0f, "The font size used for the overlay\nRequires a restart/reload to reflect changes")
    CONFIG_PROPERTY(bool, DisableOverlay, false, "Disable the built in overlay when using Injection or Proxy mode")
    CONFIG_PROPERTY(bool, OverlayOpen, true, "Overlay opened/closed");

public:
    explicit Settings(const std::filesystem::path& filePath)
        : Config(filePath)
    {
        this->save();
    }
};

#endif //UNHIDENPCS_SETTINGS_HPP
