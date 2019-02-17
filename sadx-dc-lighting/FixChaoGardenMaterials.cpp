#include "stdafx.h"

#undef _D3D8TYPES_H_ // Dirty hack to fix variables include
#include <SADXVariables.h>
#include <MemAccess.h>
#include <Trampoline.h>

// Static materials (in the main exe)
#include "ssgarden.h"
#include "ecgarden.h"

// DLL materials (because why not have a dll to swap out landtables amirite?)
#include "mrgarden.h"
#include "mrgarden_night.h"

#include "FixChaoGardenMaterials.h"

static Trampoline* ChaoGardenMR_SetLandTable_Day_t     = nullptr;
static Trampoline* ChaoGardenMR_SetLandTable_Evening_t = nullptr;
static Trampoline* ChaoGardenMR_SetLandTable_Night_t   = nullptr;

template <typename T = NJS_MATERIAL, size_t N>
void copy_materials(const T (&mats)[N], size_t offset, HMODULE key = reinterpret_cast<HMODULE>(0x400000))
{
	memcpy((void*)((int)key + offset), (void*)mats, sizeof(T) * N);
}

/// <summary>
/// Copies materials that are shared between day time
/// and evening Mystic Ruins Chao Gardens.
/// </summary>
static void copy_shared()
{
	copy_materials(matlist_00007404, 0x00007404, ModuleHandles[2]);
	copy_materials(matlist_00007518, 0x00007518, ModuleHandles[2]);
	copy_materials(matlist_00008A08, 0x00008A08, ModuleHandles[2]);
	copy_materials(matlist_0000955C, 0x0000955C, ModuleHandles[2]);
	copy_materials(matlist_00009DA0, 0x00009DA0, ModuleHandles[2]);
	copy_materials(matlist_0000A000, 0x0000A000, ModuleHandles[2]);
	copy_materials(matlist_0000A8D0, 0x0000A8D0, ModuleHandles[2]);
	copy_materials(matlist_0000BF90, 0x0000BF90, ModuleHandles[2]);
	copy_materials(matlist_0000FE98, 0x0000FE98, ModuleHandles[2]);
	copy_materials(matlist_000106CC, 0x000106CC, ModuleHandles[2]);
	copy_materials(matlist_00010848, 0x00010848, ModuleHandles[2]);
	copy_materials(matlist_00013700, 0x00013700, ModuleHandles[2]);
	copy_materials(matlist_00013B08, 0x00013B08, ModuleHandles[2]);
	copy_materials(matlist_00014030, 0x00014030, ModuleHandles[2]);
	copy_materials(matlist_00014518, 0x00014518, ModuleHandles[2]);
	copy_materials(matlist_00014A18, 0x00014A18, ModuleHandles[2]);
	copy_materials(matlist_00014EE8, 0x00014EE8, ModuleHandles[2]);
	copy_materials(matlist_000153B8, 0x000153B8, ModuleHandles[2]);
	copy_materials(matlist_000156F0, 0x000156F0, ModuleHandles[2]);
	copy_materials(matlist_00015C58, 0x00015C58, ModuleHandles[2]);
	copy_materials(matlist_00016070, 0x00016070, ModuleHandles[2]);
	copy_materials(matlist_000165B8, 0x000165B8, ModuleHandles[2]);
	copy_materials(matlist_00017778, 0x00017778, ModuleHandles[2]);
	copy_materials(matlist_00017AC0, 0x00017AC0, ModuleHandles[2]);
	copy_materials(matlist_00017E08, 0x00017E08, ModuleHandles[2]);
	copy_materials(matlist_00018788, 0x00018788, ModuleHandles[2]);
	copy_materials(matlist_00018DCC, 0x00018DCC, ModuleHandles[2]);
	copy_materials(matlist_0001940C, 0x0001940C, ModuleHandles[2]);
	copy_materials(matlist_00019A4C, 0x00019A4C, ModuleHandles[2]);
	copy_materials(matlist_0001A08C, 0x0001A08C, ModuleHandles[2]);
	copy_materials(matlist_0001A6CC, 0x0001A6CC, ModuleHandles[2]);
	copy_materials(matlist_0001AD0C, 0x0001AD0C, ModuleHandles[2]);
	copy_materials(matlist_0001B34C, 0x0001B34C, ModuleHandles[2]);
	copy_materials(matlist_0001B98C, 0x0001B98C, ModuleHandles[2]);
	copy_materials(matlist_0001BFCC, 0x0001BFCC, ModuleHandles[2]);
	copy_materials(matlist_0001C60C, 0x0001C60C, ModuleHandles[2]);
	copy_materials(matlist_0001D78C, 0x0001D78C, ModuleHandles[2]);
	copy_materials(matlist_0001E90C, 0x0001E90C, ModuleHandles[2]);
	copy_materials(matlist_0001FA8C, 0x0001FA8C, ModuleHandles[2]);
	copy_materials(matlist_00020C0C, 0x00020C0C, ModuleHandles[2]);
	copy_materials(matlist_00021D8C, 0x00021D8C, ModuleHandles[2]);
	copy_materials(matlist_00022F0C, 0x00022F0C, ModuleHandles[2]);
	copy_materials(matlist_0002408C, 0x0002408C, ModuleHandles[2]);
	copy_materials(matlist_0002520C, 0x0002520C, ModuleHandles[2]);
	copy_materials(matlist_0002638C, 0x0002638C, ModuleHandles[2]);
	copy_materials(matlist_0002750C, 0x0002750C, ModuleHandles[2]);
	copy_materials(matlist_0002868C, 0x0002868C, ModuleHandles[2]);
	copy_materials(matlist_0002980C, 0x0002980C, ModuleHandles[2]);
	copy_materials(matlist_0002A98C, 0x0002A98C, ModuleHandles[2]);
	copy_materials(matlist_0002BB0C, 0x0002BB0C, ModuleHandles[2]);
	copy_materials(matlist_0002CC90, 0x0002CC90, ModuleHandles[2]);
	copy_materials(matlist_0002D308, 0x0002D308, ModuleHandles[2]);
	copy_materials(matlist_0002D9A0, 0x0002D9A0, ModuleHandles[2]);
	copy_materials(matlist_0002E038, 0x0002E038, ModuleHandles[2]);
	copy_materials(matlist_0002E2C0, 0x0002E2C0, ModuleHandles[2]);
	copy_materials(matlist_0002E548, 0x0002E548, ModuleHandles[2]);
	copy_materials(matlist_0002E7D0, 0x0002E7D0, ModuleHandles[2]);
	copy_materials(matlist_0002EEA0, 0x0002EEA0, ModuleHandles[2]);
	copy_materials(matlist_0002FC88, 0x0002FC88, ModuleHandles[2]);
	copy_materials(matlist_000305C8, 0x000305C8, ModuleHandles[2]);
	copy_materials(matlist_00030EF8, 0x00030EF8, ModuleHandles[2]);
	copy_materials(matlist_000322B0, 0x000322B0, ModuleHandles[2]);
	copy_materials(matlist_00032BDC, 0x00032BDC, ModuleHandles[2]);
	copy_materials(matlist_00033730, 0x00033730, ModuleHandles[2]);
}

static void __cdecl ChaoGardenMR_SetLandTable_Day_r()
{
	TARGET_DYNAMIC(ChaoGardenMR_SetLandTable_Day)();

	copy_shared();
	copy_materials(matlist_00009440, 0x00009440, ModuleHandles[2]);
}

static void __cdecl ChaoGardenMR_SetLandTable_Evening_r()
{
	TARGET_DYNAMIC(ChaoGardenMR_SetLandTable_Evening)();

	copy_shared();
	copy_materials(matlist_00009440_e, 0x00009440, ModuleHandles[2]);
}

static void __cdecl ChaoGardenMR_SetLandTable_Night_r()
{
	TARGET_DYNAMIC(ChaoGardenMR_SetLandTable_Night)();

	copy_materials(matlist_00007254, 0x00007254, ModuleHandles[2]);
	copy_materials(matlist_00007368, 0x00007368, ModuleHandles[2]);
	copy_materials(matlist_00008858, 0x00008858, ModuleHandles[2]);
	copy_materials(matlist_00009290, 0x00009290, ModuleHandles[2]);
	copy_materials(matlist_000093C0, 0x000093C0, ModuleHandles[2]);
	copy_materials(matlist_00009C08, 0x00009C08, ModuleHandles[2]);
	copy_materials(matlist_00009E68, 0x00009E68, ModuleHandles[2]);
	copy_materials(matlist_0000A738, 0x0000A738, ModuleHandles[2]);
	copy_materials(matlist_0000BDF8, 0x0000BDF8, ModuleHandles[2]);
	copy_materials(matlist_0000FD00, 0x0000FD00, ModuleHandles[2]);
	copy_materials(matlist_00010534, 0x00010534, ModuleHandles[2]);
	copy_materials(matlist_000106B0, 0x000106B0, ModuleHandles[2]);
	copy_materials(matlist_00013568, 0x00013568, ModuleHandles[2]);
	copy_materials(matlist_00013970, 0x00013970, ModuleHandles[2]);
	copy_materials(matlist_00013E98, 0x00013E98, ModuleHandles[2]);
	copy_materials(matlist_00014380, 0x00014380, ModuleHandles[2]);
	copy_materials(matlist_00014880, 0x00014880, ModuleHandles[2]);
	copy_materials(matlist_00014D50, 0x00014D50, ModuleHandles[2]);
	copy_materials(matlist_00015220, 0x00015220, ModuleHandles[2]);
	copy_materials(matlist_00015558, 0x00015558, ModuleHandles[2]);
	copy_materials(matlist_00015AC0, 0x00015AC0, ModuleHandles[2]);
	copy_materials(matlist_00015ED8, 0x00015ED8, ModuleHandles[2]);
	copy_materials(matlist_00016420, 0x00016420, ModuleHandles[2]);
	copy_materials(matlist_000175E0, 0x000175E0, ModuleHandles[2]);
	copy_materials(matlist_00017928, 0x00017928, ModuleHandles[2]);
	copy_materials(matlist_00017C70, 0x00017C70, ModuleHandles[2]);
	copy_materials(matlist_000185F0, 0x000185F0, ModuleHandles[2]);
	copy_materials(matlist_00018C34, 0x00018C34, ModuleHandles[2]);
	copy_materials(matlist_00019274, 0x00019274, ModuleHandles[2]);
	copy_materials(matlist_000198B4, 0x000198B4, ModuleHandles[2]);
	copy_materials(matlist_00019EF4, 0x00019EF4, ModuleHandles[2]);
	copy_materials(matlist_0001A534, 0x0001A534, ModuleHandles[2]);
	copy_materials(matlist_0001AB74, 0x0001AB74, ModuleHandles[2]);
	copy_materials(matlist_0001B1B4, 0x0001B1B4, ModuleHandles[2]);
	copy_materials(matlist_0001B7F4, 0x0001B7F4, ModuleHandles[2]);
	copy_materials(matlist_0001BE34, 0x0001BE34, ModuleHandles[2]);
	copy_materials(matlist_0001C474, 0x0001C474, ModuleHandles[2]);
	copy_materials(matlist_0001D5F4, 0x0001D5F4, ModuleHandles[2]);
	copy_materials(matlist_0001E774, 0x0001E774, ModuleHandles[2]);
	copy_materials(matlist_0001F8F4, 0x0001F8F4, ModuleHandles[2]);
	copy_materials(matlist_00020A74, 0x00020A74, ModuleHandles[2]);
	copy_materials(matlist_00021BF4, 0x00021BF4, ModuleHandles[2]);
	copy_materials(matlist_00022D74, 0x00022D74, ModuleHandles[2]);
	copy_materials(matlist_00023EF4, 0x00023EF4, ModuleHandles[2]);
	copy_materials(matlist_00025074, 0x00025074, ModuleHandles[2]);
	copy_materials(matlist_000261F4, 0x000261F4, ModuleHandles[2]);
	copy_materials(matlist_00027374, 0x00027374, ModuleHandles[2]);
	copy_materials(matlist_000284F4, 0x000284F4, ModuleHandles[2]);
	copy_materials(matlist_00029674, 0x00029674, ModuleHandles[2]);
	copy_materials(matlist_0002A7F4, 0x0002A7F4, ModuleHandles[2]);
	copy_materials(matlist_0002B974, 0x0002B974, ModuleHandles[2]);
	copy_materials(matlist_0002CAF8, 0x0002CAF8, ModuleHandles[2]);
	copy_materials(matlist_0002D170, 0x0002D170, ModuleHandles[2]);
	copy_materials(matlist_0002D808, 0x0002D808, ModuleHandles[2]);
	copy_materials(matlist_0002DEA0, 0x0002DEA0, ModuleHandles[2]);
	copy_materials(matlist_0002E128, 0x0002E128, ModuleHandles[2]);
	copy_materials(matlist_0002E3B0, 0x0002E3B0, ModuleHandles[2]);
	copy_materials(matlist_0002E638, 0x0002E638, ModuleHandles[2]);
	copy_materials(matlist_0002ED08, 0x0002ED08, ModuleHandles[2]);
	copy_materials(matlist_0002FAF0, 0x0002FAF0, ModuleHandles[2]);
	copy_materials(matlist_00030430, 0x00030430, ModuleHandles[2]);
	copy_materials(matlist_00030D60, 0x00030D60, ModuleHandles[2]);
	copy_materials(matlist_00032118, 0x00032118, ModuleHandles[2]);
	copy_materials(matlist_00032A44, 0x00032A44, ModuleHandles[2]);
	copy_materials(matlist_000338D8, 0x000338D8, ModuleHandles[2]);
}

static void fix_light(NJS_OBJECT* obj)
{
	if (!obj)
	{
		return;
	}

	auto model = obj->getbasicdxmodel();

	if (model != nullptr && model->nbMat > 0)
	{
		for (int i = 0; i < model->nbMat; i++)
		{
			model->mats[i].attrflags &= ~NJD_FLAG_IGNORE_LIGHT;
		}
	}

	fix_light(obj->child);
	fix_light(obj->sibling);
}

void FixChaoGardenMaterials()
{
	ChaoGardenMR_SetLandTable_Day_t     = new Trampoline(0x0072A790, 0x0072A796, ChaoGardenMR_SetLandTable_Day_r);
	ChaoGardenMR_SetLandTable_Evening_t = new Trampoline(0x0072A820, 0x0072A826, ChaoGardenMR_SetLandTable_Evening_r);
	ChaoGardenMR_SetLandTable_Night_t   = new Trampoline(0x0072A8B0, 0x0072A8B6, ChaoGardenMR_SetLandTable_Night_r);

#pragma region Station Square
	fix_light(&ChaoRaceDoor_Model);
	fix_light(&BlackMarketDoor_Model);
	fix_light(&SSGardenExit_Model);

	copy_materials(matlist_03236884, 0x03236884);
	copy_materials(matlist_03236AF8, 0x03236AF8);
	copy_materials(matlist_03236D08, 0x03236D08);
	copy_materials(matlist_03236E70, 0x03236E70);
	copy_materials(matlist_03237250, 0x03237250);
	copy_materials(matlist_03237560, 0x03237560);
	copy_materials(matlist_03237A50, 0x03237A50);
	copy_materials(matlist_03237FB8, 0x03237FB8);
	copy_materials(matlist_03238388, 0x03238388);
	copy_materials(matlist_03238E54, 0x03238E54);
	copy_materials(matlist_032390C0, 0x032390C0);
	copy_materials(matlist_03239438, 0x03239438);
	copy_materials(matlist_03239AD4, 0x03239AD4);
	copy_materials(matlist_03239C40, 0x03239C40);
	copy_materials(matlist_03239FE0, 0x03239FE0);
	copy_materials(matlist_0323A148, 0x0323A148);
	copy_materials(matlist_0323A2B0, 0x0323A2B0);
	copy_materials(matlist_0323A444, 0x0323A444);
	copy_materials(matlist_0323A9D8, 0x0323A9D8);
	copy_materials(matlist_0323AC10, 0x0323AC10);
	copy_materials(matlist_0323B7B8, 0x0323B7B8);
	copy_materials(matlist_0323B8F0, 0x0323B8F0);
	copy_materials(matlist_0323BA98, 0x0323BA98);
	copy_materials(matlist_0323BCD0, 0x0323BCD0);
	copy_materials(matlist_0323BE38, 0x0323BE38);
	copy_materials(matlist_0323BFA0, 0x0323BFA0);
	copy_materials(matlist_0323C4F0, 0x0323C4F0);
	copy_materials(matlist_03243EB0, 0x03243EB0);
	copy_materials(matlist_032446F0, 0x032446F0);
	copy_materials(matlist_03245900, 0x03245900);
	copy_materials(matlist_03248830, 0x03248830);
	copy_materials(matlist_03249754, 0x03249754);
	copy_materials(matlist_0324B524, 0x0324B524);
	copy_materials(matlist_0324C238, 0x0324C238);
	copy_materials(matlist_0324C9EC, 0x0324C9EC);
	copy_materials(matlist_0324CB08, 0x0324CB08);
	copy_materials(matlist_0324CC28, 0x0324CC28);
	copy_materials(matlist_0324E098, 0x0324E098);
	copy_materials(matlist_03254028, 0x03254028);
	copy_materials(matlist_03255714, 0x03255714);
	copy_materials(matlist_03257214, 0x03257214);
	copy_materials(matlist_03257E00, 0x03257E00);
	copy_materials(matlist_03258DA0, 0x03258DA0);
	copy_materials(matlist_032594B0, 0x032594B0);
	copy_materials(matlist_0325A450, 0x0325A450);
	copy_materials(matlist_0325AB60, 0x0325AB60);
	copy_materials(matlist_0325B744, 0x0325B744);
	copy_materials(matlist_0325BCB8, 0x0325BCB8);
	copy_materials(matlist_0325D7A4, 0x0325D7A4);
	copy_materials(matlist_0325E390, 0x0325E390);
	copy_materials(matlist_0325F660, 0x0325F660);
	copy_materials(matlist_03260160, 0x03260160);
	copy_materials(matlist_03261900, 0x03261900);
	copy_materials(matlist_03262058, 0x03262058);
	copy_materials(matlist_03262DDC, 0x03262DDC);
	copy_materials(matlist_03264B08, 0x03264B08);
	copy_materials(matlist_03264D60, 0x03264D60);
	copy_materials(matlist_03269D68, 0x03269D68);
	copy_materials(matlist_0326A754, 0x0326A754);
	copy_materials(matlist_0326A8E4, 0x0326A8E4);
#pragma endregion

#pragma region Egg Carrier
	copy_materials(matlist_02FD83B0, 0x02FD83B0);
	copy_materials(matlist_02FD8938, 0x02FD8938);
	copy_materials(matlist_02FD0F64, 0x02FD0F64);
	copy_materials(matlist_02FD116C, 0x02FD116C);
	copy_materials(matlist_02FD24CC, 0x02FD24CC);
	copy_materials(matlist_02FD4810, 0x02FD4810);
	copy_materials(matlist_02FD4AC0, 0x02FD4AC0);
	copy_materials(matlist_02FD4D58, 0x02FD4D58);
	copy_materials(matlist_02FD583C, 0x02FD583C);
	copy_materials(matlist_02FD59B8, 0x02FD59B8);
	copy_materials(matlist_02FD5B4C, 0x02FD5B4C);
	copy_materials(matlist_02FD5CE4, 0x02FD5CE4);
	copy_materials(matlist_02FD5E7C, 0x02FD5E7C);
	copy_materials(matlist_02FD6018, 0x02FD6018);
	copy_materials(matlist_02FD75F8, 0x02FD75F8);
	copy_materials(matlist_02FD9148, 0x02FD9148);
	copy_materials(matlist_02FDC7B8, 0x02FDC7B8);
	copy_materials(matlist_02FDE5AC, 0x02FDE5AC);
	copy_materials(matlist_02FE0104, 0x02FE0104);
	copy_materials(matlist_02FE4F68, 0x02FE4F68);
	copy_materials(matlist_02FE6E6C, 0x02FE6E6C);
	copy_materials(matlist_02FE7A18, 0x02FE7A18);
	copy_materials(matlist_02FE8538, 0x02FE8538);
	copy_materials(matlist_02FE90A0, 0x02FE90A0);
	copy_materials(matlist_02FE9C48, 0x02FE9C48);
	copy_materials(matlist_02FE9F58, 0x02FE9F58);
	copy_materials(matlist_02FEAA38, 0x02FEAA38);
	copy_materials(matlist_02FEB5E0, 0x02FEB5E0);
	copy_materials(matlist_02FEB850, 0x02FEB850);
	copy_materials(matlist_02FEC420, 0x02FEC420);
	copy_materials(matlist_02FECFC8, 0x02FECFC8);
	copy_materials(matlist_02FED238, 0x02FED238);
	copy_materials(matlist_02FEDB10, 0x02FEDB10);
	copy_materials(matlist_02FEE5E0, 0x02FEE5E0);
	copy_materials(matlist_02FEEF78, 0x02FEEF78);
	copy_materials(matlist_02FEFA58, 0x02FEFA58);
	copy_materials(matlist_02FEFBDC, 0x02FEFBDC);
	copy_materials(matlist_02FEFE7C, 0x02FEFE7C);
	copy_materials(matlist_02FF002C, 0x02FF002C);
	copy_materials(matlist_02FF01B4, 0x02FF01B4);
	copy_materials(matlist_02FF03C4, 0x02FF03C4);
	copy_materials(matlist_02FF0BC8, 0x02FF0BC8);
	copy_materials(matlist_02FF1EB8, 0x02FF1EB8);
	copy_materials(matlist_02FF33D0, 0x02FF33D0);
	copy_materials(matlist_02FF3500, 0x02FF3500);
	copy_materials(matlist_02FF38D0, 0x02FF38D0);
	copy_materials(matlist_02FF3CA0, 0x02FF3CA0);
	copy_materials(matlist_02FF4070, 0x02FF4070);
	copy_materials(matlist_02FF4440, 0x02FF4440);
	copy_materials(matlist_02FF4810, 0x02FF4810);
	copy_materials(matlist_02FF4BE0, 0x02FF4BE0);
	copy_materials(matlist_02FF4FB0, 0x02FF4FB0);
	copy_materials(matlist_02FF5380, 0x02FF5380);
	copy_materials(matlist_02FF5750, 0x02FF5750);
	copy_materials(matlist_02FF5B20, 0x02FF5B20);
	copy_materials(matlist_02FF5EF0, 0x02FF5EF0);
	copy_materials(matlist_02FF62C0, 0x02FF62C0);
	copy_materials(matlist_02FF6690, 0x02FF6690);
	copy_materials(matlist_02FF6A60, 0x02FF6A60);
	copy_materials(matlist_02FF6E30, 0x02FF6E30);
	copy_materials(matlist_02FF7200, 0x02FF7200);
	copy_materials(matlist_02FF75D0, 0x02FF75D0);
	copy_materials(matlist_02FF79A0, 0x02FF79A0);
	copy_materials(matlist_02FF7D70, 0x02FF7D70);
	copy_materials(matlist_02FF7EF8, 0x02FF7EF8);
	copy_materials(matlist_02FF9BD4, 0x02FF9BD4);
	copy_materials(matlist_02FFA2D0, 0x02FFA2D0);
	copy_materials(matlist_02FFA9D0, 0x02FFA9D0);
	copy_materials(matlist_02FFB0D0, 0x02FFB0D0);
	copy_materials(matlist_02FFB7D0, 0x02FFB7D0);
	copy_materials(matlist_02FFC674, 0x02FFC674);
	copy_materials(matlist_02FFD018, 0x02FFD018);
	copy_materials(matlist_030007B4, 0x030007B4);
	copy_materials(matlist_03000C10, 0x03000C10);
	copy_materials(matlist_03001964, 0x03001964);
	copy_materials(matlist_03001DC0, 0x03001DC0);
	copy_materials(matlist_03002B14, 0x03002B14);
	copy_materials(matlist_03002F70, 0x03002F70);
	copy_materials(matlist_03003CC4, 0x03003CC4);
	copy_materials(matlist_03004120, 0x03004120);
#pragma endregion
}
