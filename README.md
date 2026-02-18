# UnhideNPCs

Ever been annoyed that enemies, bosses and NPCs disappear when many players are nearby?  
UnhideNPCs keeps them visible.

Using this, you could even put `Character Model Limit` to `Lowest` and still see all enemies/bosses/NPCs!

## Notes
UnhideNPCs does nothing while in PvP or WvW.

Tested on Windows 11 25H2 and Fedora Linux 43 (GE-Proton10-30).

## In-Game Configuration

This add-on includes an in-game configuration interface.

> **Note:** The images below may or may not show an older version of the plugin

### Nexus Version
When installed via **Nexus**, the configuration is available directly in the Nexus UI:

<img width="275" height="437" alt="Nexus configuration UI" src="https://github.com/user-attachments/assets/61df3622-7c4f-4256-8de5-826a243e891a" />

---

### ArcDPS / Proxy / Injection Version
When using **ArcDPS**, **Proxy or **Injection**, the configuration appears in its own separate window:

<img width="304" height="451" alt="image" src="https://github.com/user-attachments/assets/efe630e7-2b7b-4d98-baa8-cb0e052474d1" />


> **Note:** The "Overlay" section's settings shown in the image are only functional when using **Proxy** or **Injection** mode.

> If you tick "Disable Overlay", it will immediately unload the window, and if you want it back, you need to edit the config file
> and set DisableOverlay to false.

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

- `ForceConsole`: < true/false >
  - Forces the creation of a console window when set to true.
  - Note: If the console window is exited, then the game will exit as well.
  - Default: false

- `AlwaysShowTarget`: < true/false >
  - Always show the targeted character, even if it would be hidden.
  - Default: true

- `PlayerOwned`: < true/false >
  - NPCs that players own (pets, clones, minis, etc.) will also be unhidden.
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
  - `1`: Only attackable
  - `2`: Only non-attackable
  - Default: 0

- `HidePlayers`: < true/false >
  - Players will be hidden when this is enabled, useful for boosting performance.
  - Their names are still visible, and you can still target them.
  - Default: false

- `MaxPlayersVisible`: < number >
  - Maximum number of visible players. Set to 0 for no limit.
  - Default: 0

- `MaxPlayerOwnedVisible`: < number >
  - Maximum number of visible player-owned NPCs (pets, clones, minis, etc.). Set to 0 for no limit.
  - Default: 0

- `HideNonGuildMembers`: < true/false >
  - Hide any players that are not mutual guild members.
  - Default: false

- `HideNonGuildMembersOwned`: < true/false >
  - Also hide their owned characters (pets, clones etc.).
  - Default: false

- `HideNonGroupMembers`: < true/false >
  - Hide any players who are not in the same group as you (party, squad).
  - Default: false

- `HideNonGroupMembersOwned`: < true/false >
  - Also hide their owned characters (pets, clones etc.).
  - Default: false

- `HidePlayerOwned`: < true/false >
  - NPCs that are owned by players (pets, clones, minis, etc.) will be hidden.
  - Default: false

- `HidePlayerOwnedSelf`: < true/false >
  - Also hide NPCs that you own.
  - Default: false

- `DisableHidingInInstances`: < true/false >
  - Disables the hiding options while in an instance (Fractals, Dungeons, etc.).
  - Default: false

- `MaximumDistance`: < number >
  - NPCs within this distance will be unhidden. Set to `0` for no distance check.
  - Default: 0

- `LoadScreenBoost`: < true/false >
  - Speed up loading screens by temporarily limiting number of characters to `0` when one is triggered.
  - Note: Characters will start loading after the loading screen is finished, which can cause brief invisibility after loading.
  - Default: false

- `ArcDPS_UIOpen`: < true/false >
  - ArcDPS UI opened/closed state.
  - Default: true

- `DisableOverlay`: < true/false >
  - Disable the built-in overlay when using Injection or Proxy mode.
  - Default: false

- `CloseOnEscape`: < true/false >
  - Close the overlay when Escape is pressed.
  - Default: true

- `OverlayFontSize`: < number >
  - Font size used for the overlay.
  - Requires restart/reload to reflect changes.
  - Default: 14.0

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
You are free to use, modify and distribute this software under the terms of the GPLv3.  
See the [LICENSE](LICENSE) file for full details.

## Contact / Issues

If you encounter issues or have questions, please open an issue in this repository.  
When reporting a problem, include these if relevant:

- Steps to reproduce the issue.
- Relevant logs from `<game folder>/addons/UnhideNPCs/log.txt`.
