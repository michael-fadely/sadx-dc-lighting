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
- A graphics card which supports vertex texture sampling (i.e a graphics card released more recently than the year 2000).
- Visual C++ Redistributable for Visual Studio 2017 (x86!): https://aka.ms/vs/17/release/vc_redist.x86.exe
- DirectX 9.0c end-user runtime: https://www.microsoft.com/en-us/download/details.aspx?id=8109
- The latest* version of the SADX Mod Loader.
- d3d8to9 (included in the Mod Loader as of 2022): https://github.com/crosire/d3d8to9/releases/latest

\**If you have a reasonably new version, it has automatic update checking in the options tab. If not, download here and extract to your game's root directory: http://mm.reimuhakurei.net/sadxmods/SADXModLoader.7z*

### Installing d3d8to9
In recent versions of the Mod Loader, d3d8to9 is included and enabled by default. Before using Lantern Engine, make sure "Enable Direct3D 9" is checked in the SADX Mod Manager's Graphics options. 

To install d3d8to9 manually you can follow these steps:
- Download d3d8.dll from https://github.com/crosire/d3d8to9/releases/latest.
- Place d3d8.dll in the root of your SADX folder (where sonic.exe is).

### Installing sadx-dc-lighting
- Download the archive from https://github.com/SonicFreak94/sadx-dc-lighting/releases/latest.
- Open the archive and extract the sadx-dc-lighting folder itself into your SADX mods folder. The mods folder should be in your game's root directory.
- Enable Lantern Engine from the SADX Mod Manager.
