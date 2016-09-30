#pragma once

#define _D3D8TYPES_H_
#define EXPORT __declspec(dllexport)

#define WIN32_LEAN_AND_MEAN

// Windows API
#include <Windows.h>

// Direct3D
#include <d3d9.h>
#include <d3dx9.h>

// d3d8to9
#include <d3d8to9.hpp>

// Mod loader
#include <SADXModLoader.h>
#include <Trampoline.h>

// Standard library
#include <fstream>
#include <vector>
#include <exception>
#include <string>
#include <sstream>

// Local
#include "d3d.h"
#include "datapointers.h"
#include "fog.h"
#include "globals.h"
