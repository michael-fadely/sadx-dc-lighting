#pragma once

#define _D3D8TYPES_H_

#define EXPORT __declspec(dllexport)
#define TARGET_DYNAMIC(name) ((decltype(name##_r)*)name##_t->Target())
#define TARGET_STATIC(name) ((decltype(name##_r)*)name##_t.Target())

#define WIN32_LEAN_AND_MEAN

// Windows API
#include <Windows.h>

// Direct3D
#include <d3d9.h>
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <ModLoader/MemAccess.h>
#include <SADXModLoader.h>
#include <Trampoline.h>

// Standard library
#include <exception>
#include <fstream>
#include <ninja.h>
#include <sstream>
#include <string>
#include <vector>

// Local
#include "Trampoline.h"
#include "d3d.h"
#include "datapointers.h"
#include "fog.h"
#include "globals.h"
#include "lantern.h"
