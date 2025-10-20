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
    (
        bool,
        UnhidePlayerOwned,
        false,
        "When enabled, it will also unhide player pets, clones, minis etc"
    )

    CONFIG_PROPERTY
    (
        float,
        MaximumDistance,
        0,
        "The maximum distance (in meters) at which NPCs will be unhidden.\n" "Set to 0 or below for no distance check."
    )

public:
    explicit Settings(const std::filesystem::path& filePath)
        : Config(filePath)
    {
        this->save();
    }
};

#endif //UNHIDENPCS_SETTINGS_HPP
