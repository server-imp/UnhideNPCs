# Configuration
There are configurable hotkeys in the interface, they are saved at `<game folder>/addons/UnhideNPCs/hotkeys.json`

There is a config file at `<game folder>/addons/UnhideNPCs/config.cfg` with the following options:

- `UnhideNpcs`: < true/false >
  - Unhide NPCs.
  - Default: true

- `UnhidePlayers`: < true/false >
  - Unhide players.
  - Default: false

- `PlayerOwned`: < true/false >
  - Characters that players own (pets, clones, minis, etc.) will also be unhidden.
  - Default: false

- `AlwaysShowTarget`: < true/false >
  - Always show the targeted character, even if it would be hidden.
  - Default: true

- `UnhideLowQuality`: < true/false >
  - Use low quality models when unhiding characters.
  - Default: false

- `MinimumRank`: < number >
  - Only NPCs that have at least this rank get unhidden.
  - `0`: Normal
  - `1`: Veteran
  - `2`: Elite
  - `3`: Champion
  - `4`: Legendary
  - Default: 0

- `Attackable`: < number >
  - Only NPCs that match this get unhidden.
  - `0`: Both
  - `1`: Only Attackable
  - `2`: Only Non-Attackable
  - Default: 0

- `MaximumDistance`: < number >
  - Characters outside of this distance won't be unhidden. Set to `0` for unlimited.
  - Default: 0

- `HidePlayers`: < true/false >
  - Hide all players (not yourself).
  - Default: false

- `HidePlayerOwned`: < true/false >
  - Hide all characters owned by players, except your own (pets, clones, minis, etc.).
  - Default: false

- `HideBlockedPlayers`: < true/false >
  - Hide any players that you have blocked.
  - Default: false

- `HideBlockedPlayersOwned`: < true/false >
  - Hide any characters that are owned by blocked players (pets, clones, minis, etc.).
  - Default: false

- `HideNonGroupMembers`: < true/false >
  - Hide any players who are not in the same group as you.
  - Default: false

- `HideNonGroupMembersOwned`: < true/false >
  - Hide any characters that are owned by non-group members (pets, clones, minis, etc.).
  - Default: false

- `HideNonGuildMembers`: < true/false >
  - Hide any players that aren't guild members.
  - Default: false

- `HideNonGuildMembersOwned`: < true/false >
  - Hide any characters that are owned by non-guild members (pets, clones, minis, etc.).
  - Default: false

- `HideNonFriends`: < true/false >
  - Hide any players that aren't friends.
  - Default: false

- `HideNonFriendsOwned`: < true/false >
  - Hide any characters that are owned by non-friend players (pets, clones, minis, etc.).
  - Default: false

- `HidePlayerOwnedSelf`: < true/false >
  - Hide characters that are owned by you.
  - Default: false

- `HidePlayersInCombat`: < true/false >
  - Hide players when you are in combat.
  - Default: false

- `HidePlayerOwnedInCombat`: < true/false >
  - Hide player-owned characters when you are in combat.
  - Default: false

- `MaxPlayersVisible`: < number >
  - Maximum number of visible players. Set to `0` for no limit.
  - Default: 0

- `MaxPlayerOwnedVisible`: < number >
  - Maximum number of visible player-owned characters. Set to `0` for no limit.
  - Default: 0

- `MaxNpcs`: < number >
  - Maximum number of visible NPCs. Set to `0` for no limit.
  - Default: 0

- `DisableHidingInInstances`: < true/false >
  - Disables the hiding options while in an instance (Fractals, Dungeons, etc.).
  - Default: false

- `ForceConsole`: < true/false >
  - Create a console window.
  - Note: If the console window is exited, then the game will exit as well.
  - Default: false

- `LoadScreenBoost`: < true/false >
  - Speed up loading screens by temporarily limiting number of characters to `0` when one is triggered.
  - Note: Characters will start loading after the loading screen is finished, which can cause brief invisibility after loading.
  - Default: false

- `CloseOnEscape`: < true/false >
  - Close the overlay when Escape is pressed.
  - Default: true

- `OverlayFontSize`: < number >
  - The font size used for the overlay.
  - Requires restart/reload to reflect changes.
  - Default: 14.0

- `DisableOverlay`: < true/false >
  - Disable the built-in overlay when using Injection or Proxy mode.
  - Default: false

- `OverlayOpen`: < true/false >
  - Overlay opened/closed state.
  - Default: true
