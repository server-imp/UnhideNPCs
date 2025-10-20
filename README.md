# UnhideNPCs

Ever been annoyed that enemies, bosses and NPCs disappear when many players are nearby?  
UnhideNPCs keeps them visible.

Using this, you could even put `Character Model Limit` to `Lowest` and still see all enemies/bosses/NPCs!

## Notes
By default it won't unhide NPCs that are owned by other players (e.g. pets, minis, summons). You can change this in the config file.

UnhideNPCs works as expected in my testing, but it hasn’t been widely used yet. Use with caution, as unforeseen issues may appear.

There’s no visible in-game UI (unless you are using Nexus), but you can confirm the plugin is active by checking the log file at `<game folder>/addons/UnhideNPCs/log.txt`.

Tested on Windows 11 25H2 and Fedora Linux 42 (GE-Proton10-20).

## Installation
There are four different ways to use UnhideNPCs; you only need to follow one of them.
### 1) ArcDPS
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Place it in your game folder (e.g. `C:\Program Files\Guild Wars 2`).
3. ArcDPS will load the plugin automatically on next launch.

### 2) Nexus
1. It's available in-game in the Nexus Library
   - When you are using Nexus, a configuration menu is available in-game

### 3) DLL Proxy
1. Download `UnhideNPCs.dll` from [Releases](https://github.com/server-imp/UnhideNPCs/releases).
2. Rename it to one of the following: `dxgi.dll`/`midimap.dll`/`d3d9.dll`.
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


- `UnhidePlayerOwnedNPCs`: < true/false >
    - When enabled, it will also unhide player pets, clones, minis etc
    - Default: false


- `MaximumDistance`: < number >
  - The maximum distance (in meters) at which NPCs will be unhidden.
  - Set to 0 or below for no distance check.
  - Default: 0

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
You are free to use, modify, and distribute this software under the terms of the GPLv3.  
See the [LICENSE](LICENSE) file for full details.

## Contact / Issues

If you encounter issues or have questions, please open an issue in this repository.  
When reporting a problem, include these if relevant:

- Steps to reproduce the issue.
- Relevant logs from `<game folder>/addons/UnhideNPCs/log.txt`.
