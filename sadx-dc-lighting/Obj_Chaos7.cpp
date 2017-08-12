#include "stdafx.h"

#include "d3d.h"
#include <SADXModLoader.h>
#include <Trampoline.h>

#include "globals.h"
#include "Obj_Chaos7.h"

DataPointer(Uint8, PerfectChaosPhase, 0x03C69E28);

static bool use_secondary = false;
static Trampoline* Obj_Chaos7_t = nullptr;

static void __cdecl Obj_Chaos7_r(ObjectMaster* a1)
{
	if (!a1->Data1->Action)
	{
		use_secondary = false;
	}

	TARGET_DYNAMIC(Obj_Chaos7)(a1);

	bool use = PerfectChaosPhase == 1 && !*(int*)0x3B2C578; // address is EV_MainThread_ptr

	if (use != use_secondary)
	{
		use_secondary = use;

		if (use)
		{
			globals::palettes.LoadPalette(CurrentLevel, 1);
			globals::palettes.SetLastLevel(CurrentLevel, 1);
		}
		else
		{
			globals::palettes.LoadFiles();
		}
	}
}

void Chaos7_Init()
{
	Obj_Chaos7_t = new Trampoline(0x0055DCD0, 0x0055DCD6, Obj_Chaos7_r);
}
