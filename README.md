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

<img src="https://github.com/user-attachments/assets/83f8d3e6-39a8-40d7-ba6b-bc8ee36f0860" width="33%">


### ArcDPS / Proxy / Injection Version
When using **ArcDPS**, **Proxy** or **Injection**, the configuration appears in its own separate window:

<img src="https://github.com/user-attachments/assets/5daeb30a-7062-4e74-a2ca-c402947f0690" width="33%">

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
Configuration options are documented in [CONFIGURATION.md](CONFIGURATION.md).

Hotkeys are saved at `<game folder>/addons/UnhideNPCs/hotkeys.json`.

Config values are saved at `<game folder>/addons/UnhideNPCs/config.cfg`.

## License

This project is licensed under the **GNU General Public License v3.0 (GPLv3)**.  
You are free to use, modify and distribute this software under the terms of the GPLv3.  
See the [LICENSE](LICENSE) file for full details.

## Contact / Issues

If you encounter issues or have questions, please open an issue in this repository.  
When reporting a problem, include these if relevant:

- Steps to reproduce the issue.
- Relevant logs from `<game folder>/addons/UnhideNPCs/log.txt`.
