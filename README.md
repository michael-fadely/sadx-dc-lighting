# sadx-dc-lighting
[![Build status](https://ci.appveyor.com/api/projects/status/0xab7rqxy33nv835?svg=true)](https://ci.appveyor.com/project/SonicFreak94/sadx-dc-lighting)

*sadx-dc-lighting* (Lantern Engine) is a mod for Sonic Adventure DX PC which implements the palette-based "Lantern" lighting engine from Sonic Adventure on the Dreamcast.

### Prerequisites
- A graphics card which supports vertex texture sampling (i.e a graphics card released more recently than the year 2000).
- Visual C++ Redistributable for Visual Studio 2015 (x86!): https://www.microsoft.com/en-us/download/details.aspx?id=48145
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
