# UnhideNPCs

Ever been annoyed that enemies, bosses and NPCs disappear when many players are nearby?  
UnhideNPCs keeps them visible.

Using this, you could even put `Character Model Limit` to `Lowest` and still see all enemies/bosses/NPCs!

## Notes
UnhideNPCs does nothing while in PvP or WvW.

Tested on Windows 11 25H2 and Fedora Linux 43 (GE-Proton10-30).

## In-Game Configuration

This add-on includes an in-game configuration interface.

> **Note:** You can CTRL+Click the sliders if you prefer typing the numbers.
> 
> The images below may or may not show an older version of the plugin

### Nexus Version
When installed via **Nexus**, the configuration is available directly in the Nexus UI:

<img src="https://github.com/user-attachments/assets/c50332b7-3217-47f0-869c-f35f5dcfbb5e" width="33%">

### ArcDPS / Proxy / Injection Version
When using **ArcDPS**, **Proxy** or **Injection**, the configuration appears in its own separate window:

<img src="https://github.com/user-attachments/assets/5a5d718d-de0f-4718-838f-42718b10b3aa" width="33%">

## Installation
There are four different ways to use UnhideNPCs; only follow one of them.

### 1) Nexus (recommended)
1. It's available in-game in the Nexus Library

### 2) ArcDPS
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Place it in your game folder (e.g. `C:\Program Files\Guild Wars 2`).
3. ArcDPS will load the plugin automatically on the next launch.

### 3) DLL Proxy
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Rename it to one of the following: `dxgi.dll`/`midimap.dll`/`d3d9.dll`.
    - Note: `midimap.dll` does not appear to work on Linux/Proton in my tests
3. Place it in your game folder.
4. The game will load it automatically on the next launch.

### 4) Manual injection (for developers/testing mainly)
1. Inject `UnhideNPCs.dll` using your injector of choice, a simple one is included in the project for convenience.
2. When injected:
    - Press `END` to unload the plugin so it can be reloaded without restarting the game.

## Configuration
There is a config file at `<game folder>/addons/UnhideNPCs/config.cfg` with the following options:

- `AlwaysShowTarget`: < true/false >
  - Always show the targeted character, even if it would be hidden.
  - Default: true

- `PlayerOwned`: < true/false >
  - Characters that players own (pets, clones, minis, etc.) will also be unhidden.
  - Default: false

- `UnhideNpcs`: < true/false >
  - Unhide NPCs.
  - Default: true

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

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
You are free to use, modify and distribute this software under the terms of the GPLv3.  
See the [LICENSE](LICENSE) file for full details.

## Contact / Issues

If you encounter issues or have questions, please open an issue in this repository.  
When reporting a problem, include these if relevant:

- Steps to reproduce the issue.
- Relevant logs from `<game folder>/addons/UnhideNPCs/log.txt`.
