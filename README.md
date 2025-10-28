# UnhideNPCs

Ever been annoyed that enemies, bosses and NPCs disappear when many players are nearby?  
UnhideNPCs keeps them visible.

Using this, you could even put `Character Model Limit` to `Lowest` and still see all enemies/bosses/NPCs!

## Notes
UnhideNPCs works as expected in my testing, but it hasn’t been widely used yet. Use with caution, as unforeseen issues may appear.

UnhideNPCs does nothing while in PvP or WvW.

When using the Nexus or ArcDPS there is in-game configuration available, in proxy or injected modes you need to manually edit the config file)

Tested on Windows 11 25H2 and Fedora Linux 42 (GE-Proton10-20).

## Installation
There are four different ways to use UnhideNPCs; you only need to follow one of them.

### 1) Nexus (recommended)
1. It's available in-game in the Nexus Library
   - When you are using Nexus, a configuration menu is available in-game
   - <img width="220" height="350" alt="image" src="https://github.com/user-attachments/assets/61df3622-7c4f-4256-8de5-826a243e891a" />
   - `Images may or may not be out of date, and not show the latest options`



### 2) ArcDPS
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Place it in your game folder (e.g. `C:\Program Files\Guild Wars 2`).
3. ArcDPS will load the plugin automatically on next launch.
   - <img width="280" height="282" alt="image" src="https://github.com/user-attachments/assets/58c7c9b3-6f80-4bf4-9386-5ef3a5864222" />
   - `Images may or may not be out of date, and not show the latest options`
   - Hotkey: ALT+SHIFT+U



### 3) DLL Proxy
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Rename it to one of the following: `dxgi.dll`/`midimap.dll`/`d3d9.dll`.
   - Note: `midimap.dll` does not appear to work on Linux/Proton in my tests
3. Place it in your game folder.
4. The game will load it automatically on next launch.

### 4) Manual injection (for developers/testing mainly)
1. Inject `UnhideNPCs.dll` using your injector of choice, a simple one is included in the project for convenience.
2. When injected:
   - A console window will open for logs (closing it will also close the game).
   - Press `END` to unload the plugin so it can be reloaded without restarting the game.

## Configuration
There is a config file at `<game folder>/addons/UnhideNPCs/config.cfg` with the following options:

- `ForceConsole`: < true/false >
  - Forces the creation of a console window when set to true.
  - Note: If the console window is exited, then the game will exit as well.
  - Default: false

- `AlwaysShowTarget`: < true/false >
  - Always show the targeted character, even if it would otherwise be hidden.
  - Default: true

- `PlayerOwned`: < true/false >
  - When enabled, player-owned NPCs (pets, clones, minis, etc.) will also be unhidden.
  - Default: false

- `MinimumRank`: < number >
  - NPCs below this rank will not be unhidden.
    - `0`: Normal
    - `1`: Veteran
    - `2`: Elite
    - `3`: Champion
    - `4`: Legendary
  - Default: 0

- `Attackable`: < number >
  - Defines which NPCs to unhide based on their attackable status:
    - `0`: Unhide all NPCs
    - `1`: Only attackable NPCs
    - `2`: Only non-attackable NPCs
  - Default: 0

- `MaximumDistance`: < number >
  - The maximum distance (in meters) at which NPCs will be unhidden.
  - Set to 0 or below for no distance check.
  - Default: 0

- `HidePlayers`: < true/false >
  - When enabled, player characters will be hidden to improve performance.
  - Their names are still visible, and you can still target them.
  - Default: false

- `MaxPlayersVisible`: < number >
  - Maximum number of visible players. Set to 0 for no limit.
  - Default: 0

- `MaxPlayerOwnedVisible`: < number >
  - Maximum number of visible player-owned NPCs (pets, clones, minis, etc.). Set to 0 for no limit.
  - Default: 0

- `HidePlayerOwned`: < true/false >
  - When enabled, NPCs that are owned by players (pets, clones, minis, etc.) will be hidden.
  - Default: false

- `HidePlayerOwnedSelf`: < true/false >
  - When enabled, NPCs owned by your own character will also be hidden.
  - Default: false

- `DisableHidingInInstances`: < true/false >
  - When enabled, the hiding/unhiding options are disabled while in instanced content (Fractals, Dungeons, etc.).
  - Default: false

- `LoadScreenBoost`: < true/false >
  - Speed up loading screens by temporarily limiting the number of characters to 0 while a loading screen is active.
  - Note: Characters will start loading after the loading screen finishes, so some characters may be invisible for a short time.
  - Default: false

- `ArcDPS_UIOpen`: < true/false >
  - Tracks whether the ArcDPS UI is open; used by the ArcDPS integration.
  - Default: true

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
You are free to use, modify, and distribute this software under the terms of the GPLv3.  
See the [LICENSE](LICENSE) file for full details.

## Contact / Issues

If you encounter issues or have questions, please open an issue in this repository.  
When reporting a problem, include these if relevant:

- Steps to reproduce the issue.
- Relevant logs from `<game folder>/addons/UnhideNPCs/log.txt`.
