#ifndef UNHIDENPCS_SETTINGS_HPP
#define UNHIDENPCS_SETTINGS_HPP
#pragma once
#include "fw/config.hpp"

class Settings : public Config
{
private:
    CONFIG_PROPERTY
    (
        bool,
        ForceConsole,
        false,
        "Forces the creation of a console window when set to true.\n"
        "Note: If the console window is exited, then the game will exit as well."
    )

    CONFIG_PROPERTY
    (bool, PlayerOwned, false, "NPCs that are owned by players (pets, clones, minis etc) will also be unhidden.")

    CONFIG_PROPERTY(int32_t, MinimumRank, 0, "Only NPCs that have at least this rank get unhidden.")

    CONFIG_PROPERTY
    (
        int32_t,
        Attackable,
        0,
        "Only NPCs that match this get unhidden.\n0: Both\n1: Only Attackable\n2: Only Non-Attackable"
    )

    CONFIG_PROPERTY
    (
        bool,
        HidePlayers,
        false,
        "Players will be hidden when this is ticked, useful for boosting performance.\n"
        "Their names are still visible, and you can still target them"
    )

    CONFIG_PROPERTY(float, MaximumDistance, 0, "NPCs within this distance will be unhidden. (0=no distance check)")

    CONFIG_PROPERTY(bool, ArcDPS_UIOpen, true, "ArcDPS UI Opened/Closed")

public:
    explicit Settings(const std::filesystem::path& filePath)
        : Config(filePath)
    {
        this->save();
    }
};

#endif //UNHIDENPCS_SETTINGS_HPP
