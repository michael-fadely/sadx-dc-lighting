#include "stdafx.h"

#ifdef _DEBUG
	#include <vector>

	#include "d3d8types.hpp"
	#include <SADXModLoader/SADXFunctions.h>
#else
	#include <ModLoader/MemAccess.h>
#endif

#include <ninja.h>
#include "FixCharacterMaterials.h"

static HMODULE chrmodels_handle = nullptr;

inline int get_handle()
{
	if (chrmodels_handle != nullptr)
	{
		return (int)chrmodels_handle;
	}

	chrmodels_handle = GetModuleHandle(L"CHRMODELS_orig");
	if (chrmodels_handle == nullptr)
	{
		throw;
	}

	return (int)chrmodels_handle;
}

#ifdef _DEBUG
static std::vector<NJS_MATERIAL*> materials;

template<typename T = Uint32, size_t N>
static void models(NJS_MODEL_SADX* model, const T(&ids)[N])
{
	if (!model)
	{
		return;
	}

	if (model->nbMat != N)
	{
		return;
	}

	const auto& mats = model->mats;

	if (!mats)
	{
		return;
	}

	auto it = std::find(materials.begin(), materials.end(), mats);

	if (it != materials.end())
	{
		return;
	}

	for (int i = 0; i < N; i++)
	{
		if (mats[i].attr_texId != ids[i])
			return;
	}

	materials.push_back(mats);
	PrintDebug("HIT: 0x%08X\n", (int)mats - (int)chrmodels_handle);
}
template<typename T = Uint32, size_t N>
static void models(const std::string& id, int length, const T(&ids)[N])
{
	auto handle = (NJS_MODEL_SADX**)GetProcAddress(chrmodels_handle, ("___" + id + "_MODELS").c_str());
	if (!handle)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		models(handle[i], ids);
	}
}

template<typename T = Uint32, size_t N>
static void objects(NJS_OBJECT* object, const T(&ids)[N])
{
	if (!object)
	{
		return;
	}

	auto model = object->getbasicdxmodel();

	if (model)
	{
		models(model, ids);
	}

	if (object->child)
	{
		objects(object->child, ids);
	}

	if (object->sibling)
	{
		objects(object->sibling, ids);
	}
}
template<typename T = Uint32, size_t N>
static void objects(const std::string& id, int length, const T(&ids)[N])
{
	auto handle = (NJS_OBJECT**)GetProcAddress(chrmodels_handle, ("___" + id + "_OBJECTS").c_str());
	if (!handle)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		objects(handle[i], ids);
	}
}

template<typename T = Uint32, size_t N>
static void actions(NJS_ACTION* action, const T(&ids)[N])
{
	if (!action)
	{
		return;
	}

	auto object = action->object;
	objects(object, ids);
}
template<typename T = Uint32, size_t N>
static void actions(const std::string& id, int length, const T(&ids)[N])
{
	auto handle = (NJS_ACTION**)GetProcAddress(chrmodels_handle, ("___" + id + "_ACTIONS").c_str());
	if (!handle)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		actions(handle[i], ids);
	}
}
#endif

void FixCharacterMaterials()
{
	auto handle = get_handle();

#if _DEBUG
	int ids[] = { 2, 8, 8, 6, 1, 0 };
	actions("SONIC", 149, ids);
	objects("SONIC", 79, ids);
	models ("SONIC", 11, ids);
	materials.clear();
#endif
	
	// Sonic's nose
	DataArray_(NJS_MATERIAL, matlist_00565C68, (0x00565C68 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_0057636C, (0x0057636C + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_0057D7BC, (0x0057D7BC + handle), 3);

	matlist_00565C68[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0057636C[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0057D7BC[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Super Sonic's jump ball
	DataArray_(NJS_MATERIAL, matlist_0062DEBC, (0x0062DEBC + handle), 1);
	matlist_0062DEBC[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Sonic's eyes (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_0057BC78, (0x0057BC78 + handle), 6);
	matlist_0057BC78[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Sonic's jump ball
	DataArray_(NJS_MATERIAL, matlist_00579C94, (0x00579C94 + handle), 1);
	matlist_00579C94[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR; // object_0057BC44 Sonic's jump ball

	// Sonic's Crystal Ring
	DataArray_(NJS_MATERIAL, matlist_00582CF4, (0x00582CF4 + handle), 2);
	matlist_00582CF4[1].attrflags |= NJD_FLAG_IGNORE_LIGHT;    // object_00583284 Sonic's Crystal Ring
	
	// Tails' shoes (8 duplicates? Are you kidding me?):
	DataArray_(NJS_MATERIAL, matlist_00420290, (0x00420290 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_004208B8, (0x004208B8 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_004219F0, (0x004219F0 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_00422018, (0x00422018 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_0042D180, (0x0042D180 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_0042D7A8, (0x0042D7A8 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_0042E8E0, (0x0042E8E0 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_0042EF08, (0x0042EF08 + handle), 2);

	matlist_00420290[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_00420290[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_004208B8[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_004208B8[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_004219F0[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_004219F0[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_00422018[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_00422018[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042D180[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042D180[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042D7A8[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042D7A8[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042E8E0[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042E8E0[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042EF08[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_0042EF08[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;

	// Tails' eye whites ([1]) and nose ([2]):
	DataArray_(NJS_MATERIAL, matlist_00426F04, (0x00426F04 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_00433DF4, (0x00433DF4 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_004414C0, (0x004414C0 + handle), 3);

	matlist_00426F04[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00433DF4[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_004414C0[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00426F04[2].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00433DF4[2].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_004414C0[2].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Tails' eye pupils
	DataArray_(NJS_MATERIAL, matlist_00426884, (0x00426884 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_00426BC4, (0x00426BC4 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_00433774, (0x00433774 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_00433AB4, (0x00433AB4 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_0043CC48, (0x0043CC48 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_0043CDD0, (0x0043CDD0 + handle), 1);
	// (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_00447098, (0x00447098 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_004473D4, (0x004473D4 + handle), 1);

	matlist_00426884[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00426BC4[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00433774[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00433AB4[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0043CC48[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0043CDD0[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	// (cutscenes)
	matlist_00447098[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_004473D4[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Tails' eye whites (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_00447718, (0x00447718 + handle), 5);
	matlist_00447718[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	DataArray_(NJS_MATERIAL, matlist_0046E048, (0x0046E048 + handle), 2);
	matlist_0046E048[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046E63C Tails' right foot (flying)

	DataArray_(NJS_MATERIAL, matlist_0046E670, (0x0046E670 + handle), 4);
	matlist_0046E670[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046EE44 Tails' left heel (jet anklet) (flying)
	matlist_0046E670[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046EE44 Tails' left heel (jet anklet) (flying)
	matlist_0046E670[2].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046EE44 Tails' left heel (jet anklet) (flying)
	matlist_0046E670[3].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046EE44 Tails' left heel (jet anklet) (flying)

	DataArray_(NJS_MATERIAL, matlist_0046EE78, (0x0046EE78 + handle), 2);
	matlist_0046EE78[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046F46C Tails' right foot (jet anklet) (flying)

	DataArray_(NJS_MATERIAL, matlist_0046F4A0, (0x0046F4A0 + handle), 4);
	matlist_0046F4A0[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046FC84 Tails' right heel (jet anklet) (flying)
	matlist_0046F4A0[1].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046FC84 Tails' right heel (jet anklet) (flying)
	matlist_0046F4A0[2].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046FC84 Tails' right heel (jet anklet) (flying)
	matlist_0046F4A0[3].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_0046FC84 Tails' right heel (jet anklet) (flying)

	// Knuckles' eye whites:
	DataArray_(NJS_MATERIAL, matlist_002DD1E4, (0x002DD1E4 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_002E9C64, (0x002E9C64 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_00327E7C, (0x00327E7C + handle), 3);
	// (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_002FC588, (0x002FC588 + handle), 3);

	matlist_002DD1E4[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_002E9C64[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	matlist_00327E7C[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
	// (cutscenes)
	matlist_002FC588[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;

	// Knuckles' nose:
	DataArray_(NJS_MATERIAL, matlist_002DD8FC, (0x002DD8FC + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_002EA37C, (0x002EA37C + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_002F8564, (0x002F8564 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_002FE25C, (0x002FE25C + handle), 2);

	matlist_002DD8FC[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_002EA37C[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_002F8564[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_002FE25C[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	DataArray_(NJS_MATERIAL, matlist_002D71A8, (0x002D71A8 + handle), 1);
	matlist_002D71A8[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002D7900 Knuckles' left foot

	DataArray_(NJS_MATERIAL, matlist_002D7934, (0x002D7934 + handle), 1);
	matlist_002D7934[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002D7FDC Knuckles' left heel

	DataArray_(NJS_MATERIAL, matlist_002D8930, (0x002D8930 + handle), 1);
	matlist_002D8930[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002D9088 Knuckles' right foot

	DataArray_(NJS_MATERIAL, matlist_002D90BC, (0x002D90BC + handle), 1);
	matlist_002D90BC[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002D9754 Knuckles' right heel

	DataArray_(NJS_MATERIAL, matlist_002E3C28, (0x002E3C28 + handle), 1);
	matlist_002E3C28[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002E4380 Knuckles' left shoe (gliding)

	DataArray_(NJS_MATERIAL, matlist_002E43B4, (0x002E43B4 + handle), 1);
	matlist_002E43B4[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002E4A5C Knuckles' left heel (gliding)

	DataArray_(NJS_MATERIAL, matlist_002E53B0, (0x002E53B0 + handle), 1);
	matlist_002E53B0[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002E5B08 Knuckles' right foot (gliding)

	DataArray_(NJS_MATERIAL, matlist_002E5B3C, (0x002E5B3C + handle), 1);
	matlist_002E5B3C[0].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR; // object_002E61D4 Knuckles' right heel (gliding)

	// Amy's nose:
	DataArray_(NJS_MATERIAL, matlist_00012358, (0x00012358 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_00018ABC, (0x00018ABC + handle), 3);

	matlist_00012358[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00018ABC[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Amy's eyes:
	DataArray_(NJS_MATERIAL, matlist_00011A00, (0x00011A00 + handle), 1);
	// (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_0001E200, (0x0001E200 + handle), 1);

	matlist_00011A00[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR; // object_00012014
	// (cutscenes)
	matlist_0001E200[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR; // object_00012014

	// Amy's eye pupils:
	DataArray_(NJS_MATERIAL, matlist_000113FC, (0x000113FC + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_000116F0, (0x000116F0 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_00015A00, (0x00015A00 + handle), 1); // object_00015D28
	DataArray_(NJS_MATERIAL, matlist_00015DF8, (0x00015DF8 + handle), 1); // object_00016120
	DataArray_(NJS_MATERIAL, matlist_0001DC00, (0x0001DC00 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_0001DEF0, (0x0001DEF0 + handle), 1);

	matlist_000113FC[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_000116F0[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00015A00[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00015DF8[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0001DC00[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0001DEF0[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Amy's headband:
	DataArray_(NJS_MATERIAL, matlist_00012048, (0x00012048 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_0001E848, (0x0001E848 + handle), 1);

	matlist_00012048[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0001E848[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Amy's bracelets
	DataArray_(NJS_MATERIAL, matlist_0000D030, (0x0000D030 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_00010B2C, (0x00010B2C + handle), 2);

	matlist_0000D030[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0000D030[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00010B2C[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00010B2C[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Big's nose:
	DataArray_(NJS_MATERIAL, matlist_0011BB58, (0x0011BB58 + handle), 7);
	DataArray_(NJS_MATERIAL, matlist_00125450, (0x00125450 + handle), 7);

	matlist_0011BB58[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_00125450[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Big's eyes
	DataArray_(NJS_MATERIAL, matlist_0011B788, (0x0011B788 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_0011B978, (0x0011B978 + handle), 1);
	// (cutscenes)
	DataArray_(NJS_MATERIAL, matlist_00129208, (0x00129208 + handle), 1);
	DataArray_(NJS_MATERIAL, matlist_001293F8, (0x001293F8 + handle), 1);

	matlist_0011B788[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0011B978[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	// (cutscenes)
	matlist_00129208[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_001293F8[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Big's fishing rod (without upgrades)
	DataArray_(NJS_MATERIAL, matlist_0011E8E0, (0x0011E8E0 + handle), 3);
	DataArray_(NJS_MATERIAL, matlist_0011E718, (0x0011E718 + handle), 2);
	DataArray_(NJS_MATERIAL, matlist_001284F0, (0x001284F0 + handle), 2);

	matlist_0011E8E0[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0011E8E0[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0011E8E0[2].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	matlist_0011E718[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_0011E718[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_001284F0[0].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
	matlist_001284F0[1].attrflags |= NJD_FLAG_IGNORE_SPECULAR;

	// Big's belt buckle
	DataArray_(NJS_MATERIAL, matlist_0011A080, (0x0011A080 + handle), 1);
	matlist_0011A080[0].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_0011A158 Big's belt buckle

	// Big's life belt buckle
	DataArray_(NJS_MATERIAL, matlist_00127554, (0x00127554 + handle), 1);
	matlist_00127554[0].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_0012766C Big's life belt buckle

	// Big's life belt
	DataArray_(NJS_MATERIAL, matlist_001276A0, (0x001276A0 + handle), 4);
	matlist_001276A0[1].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_001284BC Big's life belt

	// Big's fishing rod
	DataArray_(NJS_MATERIAL, matlist_001286B8, (0x001286B8 + handle), 3);
	matlist_001286B8[0].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00128A90 Big's fishing rod (w/ upgrades)
	matlist_001286B8[1].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00128A90 Big's fishing rod (w/ upgrades)
	matlist_001286B8[2].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00128A90 Big's fishing rod (w/ upgrades)

	DataArray_(NJS_MATERIAL, matlist_001FDBA0, (0x001FDBA0 + handle), 5);
	matlist_001FDBA0[1].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_001FDF7C Gamma's light
	matlist_001FDBA0[2].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_001FDF7C Gamma's light

	DataArray_(NJS_MATERIAL, matlist_0020052C, (0x0020052C + handle), 3);
	matlist_0020052C[1].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_0020098C Gamma's right eye
	matlist_0020052C[2].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_0020098C Gamma's uh... arrow things in between his eyes

	DataArray_(NJS_MATERIAL, matlist_002009C0, (0x002009C0 + handle), 3);
	matlist_002009C0[2].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_00200D14 Gamma's left eye

	DataArray_(NJS_MATERIAL, matlist_00200D48, (0x00200D48 + handle), 9);
	matlist_00200D48[7].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_00201AE4 Gamma's head (blue line)

	DataArray_(NJS_MATERIAL, matlist_00204EC8, (0x00204EC8 + handle), 3);
	matlist_00204EC8[2].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_0020524C Gamma's left foot

	DataArray_(NJS_MATERIAL, matlist_002054FC, (0x002054FC + handle), 2);
	matlist_002054FC[0].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00205794 Gamma's left tire
	matlist_002054FC[1].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00205794 Gamma's left tire

	DataArray_(NJS_MATERIAL, matlist_002062D8, (0x002062D8 + handle), 4);
	matlist_002062D8[3].attrflags |=  NJD_FLAG_IGNORE_LIGHT; // object_00206674 Gamma's right foot

	DataArray_(NJS_MATERIAL, matlist_00206924, (0x00206924 + handle), 2);
	matlist_00206924[0].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00206BBC Gamma's right tire
	matlist_00206924[1].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_00206BBC Gamma's right tire

	DataArray_(NJS_MATERIAL, matlist_0020B9B8, (0x0020B9B8 + handle), 5);
	matlist_0020B9B8[2].attrflags |=  NJD_FLAG_IGNORE_SPECULAR; // object_0020BCAC gamma something idk
}
