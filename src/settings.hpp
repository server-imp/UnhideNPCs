#ifndef UNHIDENPCS_SETTINGS_HPP
#define UNHIDENPCS_SETTINGS_HPP
#pragma once
#include "hook.hpp"
#include "fw/settings.hpp"

#define S(type, name, value, comment) fw::Setting<type>& name = add<type>(value, #name, comment)

inline void forceVisibility(fw::SettingRefVariant& setting)
{
    ++re::forceVisibility;
}

class SettingsProfile : public fw::Settings
{
protected:
    std::unique_ptr<fw::Settings> createChild(const std::string& name) override;

public:
    fw::Setting<bool>& UnhideNPCs = add<bool>(true, "UnhideNPCs", "Unhide NPCs").onChanged(forceVisibility);

    S(bool, UnhidePlayers, false, "Unhide players").onChanged(forceVisibility);
    S(
        bool,
        PlayerOwned,
        false,
        "Characters that players own (pets, clones, minis etc) will also be unhidden."
    ).onChanged(forceVisibility);

    S(bool, AlwaysShowTarget, true, "Always show the targeted character, even if it would be hidden.").onChanged(
        forceVisibility
    );

    S(bool, UnhideLowQuality, false, "Use low quality models when unhiding characters").onChanged(forceVisibility);

    S(int32_t, MinimumRank, 0, "Only NPCs that have at least this rank get unhidden.").onChanged(forceVisibility).
validator(
            [](auto& value)
            {
                if (value < 0 || value >= static_cast<int32_t>(re::gw2::ECharacterRankMax))
                {
                    value = 0;
                }
            }
        );

    S(int32_t, Attackable, 0, "Only NPCs that match this get unhidden.").onChanged(forceVisibility).validator(
        [](auto& value)
        {
            if (value < 0 || value > 2)
            {
                value = 0;
            }
        }
    );

    S(float, MaximumDistance, 0, "Characters outside of this distance won't be unhidden. (0=unlimited)").onChanged(
        forceVisibility
    ).validator(
        [](auto& value)
        {
            if (value < 0)
            {
                value = 0;
            }
            else if (value > 1000)
            {
                value = 1000;
            }
        }
    );

    S(bool, HidePlayers, false, "Hide all players (Not yourself)").onChanged(forceVisibility);

    S(
        bool,
        HidePlayerOwned,
        false,
        "Hide all characters owned by players, except yourself (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HideBlockedPlayers, false, "Hide any players that you have blocked").onChanged(forceVisibility);
    S(
        bool,
        HideBlockedPlayersOwned,
        false,
        "Hide any characters that are owned by blocked players (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HideNonGroupMembers, false, "Hide any players who are not in the same group as you (Party or Squad)").onChanged(forceVisibility);

    S(
        bool,
        HideNonGroupMembersOwned,
        false,
        "Hide any characters that are owned by non-group members (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HideNonGuildMembers, false, "Hide any players that aren't guild members").onChanged(forceVisibility);

    S(
        bool,
        HideNonGuildMembersOwned,
        false,
        "Hide any characters that are owned by non-guild members (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HideNonFriends, false, "Hide any players that aren't friends").onChanged(forceVisibility);

    S(
        bool,
        HideNonFriendsOwned,
        false,
        "Hide any characters that are owned by non-friend players (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HideStrangers, false, "Hide any players who are not: friends, guild, party or squad members").onChanged(
        forceVisibility
    );

    S(
        bool,
        HideStrangersOwned,
        false,
        "Hide any characters that are owned by strangers (pets, clones, minis etc)"
    ).onChanged(forceVisibility);

    S(bool, HidePlayerOwnedSelf, false, "Hide characters that are owned by you").onChanged(forceVisibility);

    S(bool, HidePlayersInCombat, false, "Hide players when you are in combat").onChanged(forceVisibility);

    S(bool, HidePlayerOwnedInCombat, false, "Hide player-owned characters when you are in combat").onChanged(
        forceVisibility
    );

    S(int32_t, MaxPlayersVisible, 0, "Maximum number of visible players. (0=unlimited)").onChanged(forceVisibility).
validator(
            [](auto& value)
            {
                if (value < 0)
                {
                    value = 0;
                }
                else if (value > 1000)
                {
                    value = 1000;
                }
            }
        );

    S(int32_t, MaxPlayerOwnedVisible, 0, "Maximum number of visible player-owned characters. (0=unlimited)").onChanged(
        forceVisibility
    ).validator(
        [](auto& value)
        {
            if (value < 0)
            {
                value = 0;
            }
            else if (value > 1000)
            {
                value = 1000;
            }
        }
    );

    S(int32_t, MaxNpcs, 0, "Maximum number of visible NPCs. (0=unlimited)").onChanged(forceVisibility).validator(
        [](auto& value)
        {
            if (value < 0)
            {
                value = 0;
            }
            else if (value > 1000)
            {
                value = 1000;
            }
        }
    );

    S(int32_t, InstanceBehaviour, 0, "0: Always On\n1: Disabled in instances\n2: Instances only").onChanged(
        forceVisibility
    ).validator(
        [](auto& value)
        {
            if (value < 0 || value > 2)
            {
                value = 0;
            }
        }
    );

    explicit SettingsProfile(fw::Settings* owner, const std::string& name) : fw::Settings(owner, name, "") {}
};

class Settings : public fw::Settings
{
protected:
    virtual std::unique_ptr<fw::Settings> createChild(const std::string& name) override;

public:
    S(
        bool,
        ForceConsole,
        false,
        "Create a console window.\nNote: If the console window is exited, then the game will exit as well."
    );

    S(
        bool,
        LoadScreenBoost,
        true,
        "Speed up loading screens by temporarily limiting number of characters to 0 when one is triggered.\nNote: This will cause characters to start loading after the loading screen is finished,\nmeaning there will be invisible characters for a bit after loading."
    );

    S(bool, CloseOnEscape, true, "Close the overlay when Escape is pressed");

    S(float, OverlayFontSize, 14.0f, "The font size used for the overlay").validator(
        [](auto& value)
        {
            if (value < 10)
            {
                value = 10;
            }
            else if (value > 20)
            {
                value = 20;
            }
        }
    );

    S(bool, OverlayOpen, true, "Overlay opened/closed");

    S(int32_t, ActiveProfile, 0, "The currently active profile index").validator(
        [&](auto& value)
        {
            if (value < 0 || value >= _children.size())
            {
                value = 0;
            }
        }
    );

    [[nodiscard]] SettingsProfile& profile() const;

    explicit Settings(const std::filesystem::path& filePath);

    [[nodiscard]] bool profileExists(const std::string& name) const;
    void               addProfile(const std::string& name);
    void               removeProfile(const std::string& name);
    void               renameProfile(const std::string& oldName, const std::string& newName);
};

inline std::unique_ptr<fw::Settings> SettingsProfile::createChild(const std::string& name)
{
    return std::make_unique<SettingsProfile>(this, name);
}

inline std::unique_ptr<fw::Settings> Settings::createChild(const std::string& name)
{
    return std::make_unique<SettingsProfile>(this, name);
}

inline SettingsProfile& Settings::profile() const
{
    const auto idx = ActiveProfile.get() % _children.size();
    return *dynamic_cast<SettingsProfile*>(_children[idx].get());
}

inline Settings::Settings(const std::filesystem::path& filePath) : fw::Settings(nullptr, "", filePath)
{
    addChild<SettingsProfile>(this, "Default");

    if (!std::filesystem::exists(filePath))
    {
        this->save(true);
        this->load();
        return;
    }

    this->load();
    this->save(true);
}

inline bool Settings::profileExists(const std::string& name) const
{
    for (const auto& child : _children)
    {
        if (child->name() == name)
        {
            return true;
        }
    }
    return false;
}

inline void Settings::addProfile(const std::string& name)
{
    if (name.empty() || profileExists(name))
        return;

    addChild<SettingsProfile>(this, name);
    needSave();
}

inline void Settings::removeProfile(const std::string& name)
{
    if (name.empty())
        return;

    for (auto& child : _children)
    {
        if (child->name() == name)
        {
            _children.erase(std::remove(std::begin(_children), std::end(_children), child), std::end(_children));
            needSave();
            break;
        }
    }
}

inline void Settings::renameProfile(const std::string& oldName, const std::string& newName)
{
    if (oldName.empty() || newName.empty())
        return;

    if (profileExists(newName))
        return;

    for (auto& child : _children)
    {
        if (child->name() == oldName)
        {
            child->setName(newName);
        }
    }
}
#undef S

#endif //UNHIDENPCS_SETTINGS_HPP
