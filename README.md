# Lantern Engine
[![Build status](https://ci.appveyor.com/api/projects/status/0xab7rqxy33nv835?svg=true)](https://ci.appveyor.com/project/SonicFreak94/sadx-dc-lighting)

*sadx-dc-lighting* (Lantern Engine) is a mod for Sonic Adventure DX PC which implements the palette-based "Lantern" lighting engine from Sonic Adventure on the Dreamcast.

## What does it look like?

Here are some comparison shots using Dreamcast stages.

| Vanilla | Lantern Engine |
| - | - |
| ![Red Mountain Act 2 with Dreamcast stage/textures (Lantern Engine disabled)](https://i.imgur.com/5SOaKWt.png) | ![Red Mountain Act 2 with Dreamcast stage/textures (Lantern Engine enabled)](https://i.imgur.com/b3d5djD.png) |
| ![Final Egg Act 1 (Lantern Engine disabled)](https://i.imgur.com/UZbb85Y.png) | ![Final Egg Act 1 (Lantern Engine enabled)](https://i.imgur.com/PkCDwtO.png) |

[More comparison screenshots](https://imgur.com/a/J7fZx) - [Dreamcast comparisons screenshots](https://imgur.com/a/Fedr2) - [Video comparison](https://youtu.be/7hcG_I9YvxA)

## How do I use it?

### Prerequisites
- A graphics card which supports vertex texture sampling (i.e a graphics card released more recently than the year 2000)
- [Visual C++ Redistributable for Visual Studio 2015-2022 (x86!)](https://aka.ms/vs/17/release/vc_redist.x86.exe)
- [DirectX 9.0c End-User Runtimes](https://www.microsoft.com/en-us/download/details.aspx?id=8109)
- [SADX Mod Loader](https://github.com/X-Hax/sadx-mod-loader)
- [d3d8to9](https://github.com/crosire/d3d8to9) (included in the Mod Loader - no need to install)

### Installing sadx-dc-lighting

To be able to run the mod, make sure Render Backend is set to DirectX 9 in the Mod Manager's Game Config/Graphics tab.

#### Installation through the Mod Manager

If you have 1-click install support enabled for the Mod Manager (usually enabled automatically), you can copy the highlighted text below, paste it in your browser's address bar and press Enter.

`sadxmm:https://github.com/michael-fadely/sadx-dc-lighting/releases/latest/download/sadx-dc-lighting.7z,author:SonicFreak94,name:Lantern%20Engine,folder:sadx-dc-lighting`

#### Manual installation
To install the mod manually, follow these steps:
- Download the archive from https://github.com/SonicFreak94/sadx-dc-lighting/releases/latest.
- Open the archive and extract the `sadx-dc-lighting` folder itself into your SADX mods folder. The mods folder should be in your game's root directory.
- Enable Lantern Engine in the Mod Manager's Mods tab.

### Troubleshooting


#### Error message "SADX Lantern Engine will not function without Direct3D 9" on startup

Open the Mod Manager, go to the Game Config/Graphics tab and make sure Render Backend is set to DirectX 9.

#### Shader compilation errors on startup: "unexpected KW_SAMPLER_STATE" and others

If you are on Windows, install the DirectX 9.0c End-User Runtimes:

[Web version](https://www.microsoft.com/en-us/download/details.aspx?id=35)

[Full version for manual installation](https://www.microsoft.com/en-us/download/details.aspx?id=8109)

If you are on Linux and running the game through Wine/Proton, the following tips could help:

- Run the game through Steam/Proton instead of Wine.
- Install the DirectX 9.0c End-User Runtimes manually to the same prefix as the game via Protontricks or Winetricks.
- Install the shader compiler manually using the following terminal command: `winetricks d3dx9 d3dcompiler_43 d3dcompiler_47`
- Extract `d3dcompiler_43.dll` and `d3dcompiler_47.dll` from the DirectX 9.0c End-User Runtimes redistributable, copy them to the game folder and add them as native library overrides in winecfg.
- Delete the folder `~/.local/share/Steam/steamapps/compatdata/71250` and reinstall the game.
