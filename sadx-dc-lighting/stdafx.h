#pragma once

#define _D3D8TYPES_H_

#define EXPORT __declspec(dllexport)

// Convenient macros for trampolines.
#define TARGET_DYNAMIC(name) ((decltype(name##_r)*)name##_t->Target())
#define TARGET_STATIC(name) ((decltype(name##_r)*)name##_t.Target())

// Non-static variants of the MemAccess macros
#define DataArray_(type, name, address, length) \
	type *const name = (type *)address
#define DataPointer_(type, name, address) \
	type &name = *(type *)address

// Enable shader precompilation (in release builds)
#ifndef _DEBUG
#define PRECOMPILE_SHADERS
#endif

#define WIN32_LEAN_AND_MEAN

#ifdef _DEBUG

// Windows API
#include <Windows.h>
#include <atlbase.h>
#include <WinCrypt.h>

// Direct3D
#include <d3d9.h>
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <MemAccess.h>
#include <SADXModLoader.h>
#include <Trampoline.h>

// MinHook
#include <MinHook.h>

// Standard library
#include <algorithm>
#include <array>
#include <deque>
#include <exception>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

// API
#include "../include/lanternapi.h"

// Local
#include "d3d.h"
#include "datapointers.h"
#include "ShaderParameter.h"
#include "FixChaoGardenMaterials.h"
#include "FixCharacterMaterials.h"
#include "globals.h"
#include "lantern.h"
#include "Obj_Past.h"
#include "Obj_SkyDeck.h"
#include "Obj_Chaos7.h"
#include "Trampoline.h"
#include "FileSystem.h"

// Materials
#include "ssgarden.h"
#include "ecgarden.h"
#include "mrgarden.h"
#include "mrgarden_night.h"

#endif
