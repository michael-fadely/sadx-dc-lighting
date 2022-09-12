#include "stdafx.h"

#ifdef _DEBUG
#include <vector>
#endif

#include "d3d8types.hpp"
#include <SADXModLoader.h>
#include "../include/lanternapi.h"
#include "FixCharacterMaterials.h"

static HMODULE CHRMODELS        = GetModuleHandle(L"CHRMODELS_orig");
static HMODULE ADV00MODELS      = GetModuleHandle(L"ADV00MODELS");
static HMODULE ADV03MODELS      = GetModuleHandle(L"ADV03MODELS");
static HMODULE BOSSCHAOS0MODELS = GetModuleHandle(L"BOSSCHAOS0MODELS");

DataPointer(NJS_OBJECT, object_c7_kihon_chaos7_chaos7, 0x0139757C);
DataPointer(NJS_OBJECT, object_ev_s_t2c_body_s_t2c_body, 0x031793C0);

DataPointer(NJS_OBJECT, object_g_g0000_eggman_eggman, 0x0089C830);
DataPointer(NJS_OBJECT, object_g_g0000_eggman_kao, 0x0089B750);
DataPointer(NJS_OBJECT, object_g_g0000_eggman_hand_l, 0x00898320);
DataPointer(NJS_OBJECT, object_g_g0000_eggman_hand_r, 0x00896710);

DataPointer(NJS_OBJECT, object_gm_gm0000_eggmoble_eggmoble, 0x02EEB524);
DataPointer(NJS_OBJECT, object_gm_gm0000_eggmoble_kao, 0x02EE7808);
DataPointer(NJS_OBJECT, object_gm_gm0000_eggmoble_hand_r, 0x02EE27F0);
DataPointer(NJS_OBJECT, object_gm_gm0000_eggmoble_hand_l, 0x02EE43D8);

DataPointer(NJS_OBJECT, _object_gm_gm0000_eggmoble_eggmoble, 0x010FEF74);
DataPointer(NJS_OBJECT, _object_gm_gm0000_eggmoble_kao, 0x010FA9D0);
DataPointer(NJS_OBJECT, _object_gm_gm0000_eggmoble_hand_r, 0x010F5570);
DataPointer(NJS_OBJECT, _object_gm_gm0000_eggmoble_hand_l, 0x010F6B18);

DataPointer(NJS_OBJECT, object_egm01_egm01_1_m1_bw01, 0x0330011C);
DataPointer(NJS_OBJECT, object_egm01_egm01_1_m1_m01, 0x03302094);
DataPointer(NJS_OBJECT, object_egm01_egm01_1_m1_m02, 0x03303864);
DataPointer(NJS_OBJECT, object_egm01_egm01_1_m1_e01, 0x03300740);
DataPointer(NJS_OBJECT, object_egm01_egm01_1_egm01_1, 0x03306270);

DataPointer(NJS_OBJECT, object_egm01_egm01_m1_bw01, 0x0155AA54);
DataPointer(NJS_OBJECT, object_egm01_egm01_m1_m01, 0x0155CE84);
DataPointer(NJS_OBJECT, object_egm01_egm01_m1_m02, 0x0155EA6C);
DataPointer(NJS_OBJECT, object_egm01_egm01_m1_e01, 0x0155B140);
DataPointer(NJS_OBJECT, object_egm01_egm01_egm01, 0x01561A70);

DataPointer(NJS_OBJECT, object_egman1_body_hand_dr, 0x01562DA4);
DataPointer(NJS_OBJECT, object_egman1_body_hand_dl, 0x01563A9C);
DataPointer(NJS_OBJECT, object_egman1_body_kao, 0x015650D8);
DataPointer(NJS_OBJECT, object_egm01_egm00_egm00, 0x0155A1E4);

DataPointer(NJS_OBJECT, object_egm02_top_top, 0x0162F554);
DataPointer(NJS_OBJECT, object_egm02_egm2_body_1_top, 0x033128BC);

DataPointer(NJS_OBJECT, object_tr1_s_t1_body_s_t1_body, 0x028B1DA0);
DataPointer(NJS_OBJECT, object_tr1_s_t1_body_s_t1_pera, 0x028AD4FC);

DataPointer(NJS_OBJECT, _object_tr1_s_t1_body_s_t1_body, 0x032611F8);
DataPointer(NJS_OBJECT, _object_tr1_s_t1_body_s_t1_pera, 0x0325C954);

DataPointer(NJS_OBJECT, object_beam_beam_tr1_s_t1_body, 0x02920F8C);
DataPointer(NJS_OBJECT, object_beam_beam_tr1_s_t1_pera, 0x0290C1DC);
DataPointer(NJS_OBJECT, object_beam_beam_tr1_s_t1_wingb, 0x0291CBFC);
DataPointer(NJS_OBJECT, object_beam_beam_tr1_s_r_f4_3, 0x02918404);
DataPointer(NJS_OBJECT, object_beam_beam_tr1_s_l_f4_3, 0x02916F9C);

DataPointer(NJS_OBJECT, object_tr2a_s_t2b_body_s_t2b_body, 0x027EB198);
DataPointer(NJS_OBJECT, object_tr2a_s_t2b_body_s_t2b_wing, 0x027E8A6C);
DataPointer(NJS_OBJECT, object_tr2a_s_t2b_body_s_t2b_pera, 0x027D8000);
DataPointer(NJS_OBJECT, object_tr2a_s_t2b_body_s_t2b_mark, 0x027D872C);

DataPointer(NJS_OBJECT, _object_tr2a_s_t2b_body_s_t2b_body, 0x02C08F40);
DataPointer(NJS_OBJECT, _object_tr2a_s_t2b_body_s_t2b_wing, 0x02C06814);
DataPointer(NJS_OBJECT, _object_tr2a_s_t2b_body_s_t2b_pera, 0x02BF5DA8);
DataPointer(NJS_OBJECT, _object_tr2a_s_t2b_body_s_t2b_mark, 0x02BF64D4);

DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_tr2rt, 0x02863E20);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_swt1e, 0x0285EFC0);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_swt1b, 0x0285E5D0);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_wat1f, 0x028585B0);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_wat1c, 0x0285A5C0);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_SONIC_2, 0x02857070);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_miles_1, 0x0284D704);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_l_f4_3, 0x02852014);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_r_f4_3, 0x0285347C);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_handa, 0x028616A4);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_engn, 0x0285D23C);
DataPointer(NJS_OBJECT, object_tr2_tf_event_s_tr2rt_s_t2tf_bjet, 0x0285C3C4);

DataPointer(NJS_OBJECT, object_tr2b_s_tru2_body_s_tru2_body, 0x0280C158);
DataPointer(NJS_OBJECT, object_tr2b_s_tru2_body_s_tru2_banr, 0x02809E44);
DataPointer(NJS_OBJECT, object_tr2b_s_tru2_body_s_tru2_wina, 0x0280976C);
DataPointer(NJS_OBJECT, object_tr2b_s_tru2_body_s_tru2_mark, 0x027F90EC);

DataPointer(NJS_OBJECT, _object_tr2b_s_tru2_body_s_tru2_body, 0x032E9D28);
DataPointer(NJS_OBJECT, _object_tr2b_s_tru2_body_s_tru2_banr, 0x032E7A14);
DataPointer(NJS_OBJECT, _object_tr2b_s_tru2_body_s_tru2_wina, 0x032E733C);
DataPointer(NJS_OBJECT, _object_tr2b_s_tru2_body_s_tru2_mark, 0x032D6CBC);

DataPointer(NJS_OBJECT, object_tr2_big_s_tru2_body_s_tru2_body, 0x010E8A78);
DataPointer(NJS_OBJECT, object_tr2_big_s_tru2_body_s_tru2_banr, 0x010E6724);
DataPointer(NJS_OBJECT, object_tr2_big_s_tru2_body_s_tru2_wina, 0x010E604C);
DataPointer(NJS_OBJECT, object_tr2_big_s_tru2_body_s_tru2_mark, 0x010E55EC);

#ifdef _DEBUG
static std::vector<NJS_MATERIAL*> materials;

template <typename T = Uint32, size_t N>
static void models(NJS_MODEL_SADX* model, const T (&ids)[N])
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

	const auto it = find(materials.begin(), materials.end(), mats);

	if (it != materials.end())
	{
		return;
	}

	for (int i = 0; i < N; i++)
	{
		if (mats[i].attr_texId != ids[i])
		{
			return;
		}
	}

	materials.push_back(mats);
	PrintDebug("HIT: 0x%08X\n", (int)mats - (int)CHRMODELS);
}

template <typename T = Uint32, size_t N>
static void models(const std::string& id, int length, const T (&ids)[N])
{
	const auto handle = reinterpret_cast<NJS_MODEL_SADX**>(GetProcAddress(CHRMODELS, ("___" + id + "_MODELS").c_str()));
	if (!handle)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		models(handle[i], ids);
	}
}

template <typename T = Uint32, size_t N>
static void objects(NJS_OBJECT* object, const T (&ids)[N])
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

template <typename T = Uint32, size_t N>
static void objects(const std::string& id, int length, const T (&ids)[N])
{
	const auto handle = reinterpret_cast<NJS_OBJECT**>(GetProcAddress(CHRMODELS, ("___" + id + "_OBJECTS").c_str()));
	if (!handle)
	{
		return;
	}

	for (int i = 0; i < length; i++)
	{
		objects(handle[i], ids);
	}
}

template <typename T = Uint32, size_t N>
static void actions(NJS_ACTION* action, const T (&ids)[N])
{
	if (!action)
	{
		return;
	}

	auto object = action->object;
	objects(object, ids);
}

template <typename T = Uint32, size_t N>
static void actions(const std::string& id, int length, const T (&ids)[N])
{
	const auto handle = reinterpret_cast<NJS_ACTION**>(GetProcAddress(CHRMODELS, ("___" + id + "_ACTIONS").c_str()));
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

void ObjectRemoveSpecular(NJS_OBJECT* obj, bool recursive, bool hierarchy = false)
{
	if (!obj)
		return;
	if (obj->basicdxmodel)
	{
		for (int k = 0; k < obj->basicdxmodel->nbMat; ++k)
		{
			obj->basicdxmodel->mats[k].attrflags |= NJD_FLAG_IGNORE_SPECULAR;
		}
	}
	if (recursive)
	{
		if (obj->child)
			ObjectRemoveSpecular(obj->child, true, true);
		if (hierarchy && obj->sibling)
			ObjectRemoveSpecular(obj->sibling, true, true);
	}
}

void ObjectForceSpecular(NJS_OBJECT* obj, bool recursive, bool hierarchy = false)
{
	if (!obj)
		return;
	if (obj->basicdxmodel)
	{
		for (int k = 0; k < obj->basicdxmodel->nbMat; ++k)
		{
			obj->basicdxmodel->mats[k].specular.color = 0xFFFFFFFF;
			if (obj->basicdxmodel->mats[k].attrflags & NJD_FLAG_IGNORE_SPECULAR)
			{
				obj->basicdxmodel->mats[k].attrflags &= ~NJD_FLAG_IGNORE_SPECULAR;
			}
		}
	}
	if (recursive)
	{
		if (obj->child)
			ObjectForceSpecular(obj->child, true, true);
		if (hierarchy && obj->sibling)
			ObjectForceSpecular(obj->sibling, true, true);
	}
}

void ObjectSetIgnoreLight(NJS_OBJECT* obj, int matid)
{
	obj->basicdxmodel->mats[matid].attrflags |= NJD_FLAG_IGNORE_LIGHT;
}

void FixCharacterMaterials()
{
	// Sonic
	ObjectSetIgnoreLight(SONIC_OBJECTS[64], 0); // Crystal Ring
	// Tails
	ObjectForceSpecular(MILES_OBJECTS[34], true); // Tails left shoe
	ObjectForceSpecular(MILES_OBJECTS[32], true); // Tails right shoe
	ObjectForceSpecular(MILES_OBJECTS[52], true); // Tails right shoe (flying)
	ObjectForceSpecular(MILES_OBJECTS[54], true); // Tails left shoe (flying)
	ObjectForceSpecular(MILES_OBJECTS[64], true); // Tails right shoe (Jet Anklet)
	ObjectForceSpecular(MILES_OBJECTS[65], true); // Tails left shoe (Jet Anklet)
	ObjectForceSpecular(MILES_OBJECTS[2]->getnode(51), true); // Tails left shoe (jumpball)
	ObjectForceSpecular(MILES_OBJECTS[2]->getnode(41), true); // Tails right shoe (jumpball)
	// Tails test flight crashing (Sonic story cutscene)
	ObjectForceSpecular(&object_ev_s_t2c_body_s_t2c_body, true);
	// Knuckles
	ObjectForceSpecular(KNUCKLES_OBJECTS[49]->child, false); // Knuckles' eyes
	ObjectForceSpecular(KNUCKLES_OBJECTS[50]->child, false); // Knuckles' eyes (gliding)
	ObjectForceSpecular(KNUCKLES_OBJECTS[56]->child, false); // Knuckles' eyes (talking)
	ObjectForceSpecular(KNUCKLES_OBJECTS[18], true); // Knuckles' left shoe
	ObjectForceSpecular(KNUCKLES_OBJECTS[16], true); // Knuckles' right shoe
	ObjectForceSpecular(KNUCKLES_OBJECTS[40], true); // Knuckles' left shoe (gliding)
	ObjectForceSpecular(KNUCKLES_OBJECTS[38], true); // Knuckles' right shoe (gliding)
	ObjectForceSpecular(KNUCKLES_ACTIONS[61]->object->getnode(49), true); // Knuckles' left shoe (jumpball)
	ObjectForceSpecular(KNUCKLES_ACTIONS[61]->object->getnode(39), true); // Knuckles' right shoe (jumpball)
	ObjectForceSpecular(KNUCKLES_ACTIONS[61]->object->getnode(73), true); // Knuckles' eyes (jumpball)
	// Amy
	ObjectForceSpecular(AMY_OBJECTS[4]->child, false); // Amy headband
	ObjectForceSpecular(AMY_OBJECTS[32]->child, false); // Amy headband (talking head)
	// Big
	ObjectSetIgnoreLight(BIG_OBJECTS[22]->getnode(4), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[22]->getnode(5), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[22]->getnode(6), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[22]->getnode(7), 0); // Eyes
	// Big talking head
	ObjectSetIgnoreLight(BIG_OBJECTS[1]->getnode(2), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[1]->getnode(3), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[1]->getnode(5), 0); // Eyes
	ObjectSetIgnoreLight(BIG_OBJECTS[1]->getnode(6), 0); // Eyes
	// Gamma
	ObjectSetIgnoreLight(E102_ACTIONS[50]->object, 3); // Laser Scope red dots
	ObjectSetIgnoreLight(E102_ACTIONS[50]->object->getnode(2), 4); // Laser Scope
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(75), 1); // Chest
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(56), 1); // Right eye
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(56), 2); // Red dots
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(55), 2); // Left eye
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(54), 7); // Head back (blue light)
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(12), 3); // Right foot
	ObjectSetIgnoreLight(E102_OBJECTS[0]->getnode(24), 2); // Left foot	
	// Perfect Chaos
	ObjectForceSpecular(&object_c7_kihon_chaos7_chaos7, true);
	// Eggman
	ObjectForceSpecular(&object_g_g0000_eggman_eggman, true); // Whole model
	ObjectRemoveSpecular(&object_g_g0000_eggman_kao, true); // Eggman head
	ObjectRemoveSpecular(&object_g_g0000_eggman_hand_l, true); // Eggman left hand
	ObjectRemoveSpecular(&object_g_g0000_eggman_hand_r, true); // Eggman right hand
	// Eggmobile
	ObjectForceSpecular(&object_gm_gm0000_eggmoble_eggmoble, true); // Whole model (Eggmobile)
	ObjectRemoveSpecular(&object_gm_gm0000_eggmoble_kao, true); // Eggman head (Eggmobile)
	ObjectRemoveSpecular(&object_gm_gm0000_eggmoble_hand_l, true); // Eggman left hand (Eggmobile)
	ObjectRemoveSpecular(&object_gm_gm0000_eggmoble_hand_r, true); // Eggman right hand (Eggmobile)
	// Eggmobile NPC
	ObjectForceSpecular(&_object_gm_gm0000_eggmoble_eggmoble, true); // Whole model (Eggmobile NPC)	
	ObjectRemoveSpecular(&_object_gm_gm0000_eggmoble_kao, true); // Eggman head (Eggmobile NPC)
	ObjectRemoveSpecular(&_object_gm_gm0000_eggmoble_hand_l, true); // Eggman left hand (Eggmobile NPC)
	ObjectRemoveSpecular(&_object_gm_gm0000_eggmoble_hand_r, true); // Eggman right hand (Eggmobile NPC)
	// Egg Hornet boss
	ObjectRemoveSpecular(&object_egm01_egm01_m1_bw01, false);
	ObjectRemoveSpecular(&object_egm01_egm01_m1_m01, false);
	ObjectRemoveSpecular(&object_egm01_egm01_m1_m02, false);
	ObjectRemoveSpecular(&object_egm01_egm01_m1_bw01, false);
	ObjectForceSpecular(&object_egm01_egm01_m1_e01, false);
	ObjectForceSpecular(&object_egm01_egm01_egm01, false);
	// Egg Hornet event
	ObjectRemoveSpecular(&object_egm01_egm01_1_m1_bw01, false);
	ObjectRemoveSpecular(&object_egm01_egm01_1_m1_m01, false);
	ObjectRemoveSpecular(&object_egm01_egm01_1_m1_m02, false);
	ObjectRemoveSpecular(&object_egm01_egm01_1_m1_bw01, false);
	ObjectForceSpecular(&object_egm01_egm01_1_m1_e01, false);
	ObjectForceSpecular(&object_egm01_egm01_1_egm01_1, false);
	// Egg Hornet Eggman
	ObjectRemoveSpecular(&object_egman1_body_hand_dr, true);
	ObjectRemoveSpecular(&object_egman1_body_hand_dl, true);
	ObjectRemoveSpecular(&object_egman1_body_kao, true);
	// Egg Hornet Eggmobile
	ObjectForceSpecular(&object_egm01_egm00_egm00, false);
	// Egg Walker
	ObjectForceSpecular(&object_egm02_top_top, true); // Seat
	ObjectForceSpecular(&object_egm02_egm2_body_1_top, true); // Seat (event model)
	// Tornado 1
	ObjectForceSpecular(&object_tr1_s_t1_body_s_t1_body, false); // Tornado body
	ObjectForceSpecular(&object_tr1_s_t1_body_s_t1_pera, false); // Tornado propeller
	// Tornado 1 event
	ObjectForceSpecular(&_object_tr1_s_t1_body_s_t1_body, false); // Tornado body
	ObjectForceSpecular(&_object_tr1_s_t1_body_s_t1_pera, false); // Tornado propeller
	// Tornado 1 hit
	ObjectForceSpecular(&object_beam_beam_tr1_s_t1_body, false); // Tornado body
	ObjectForceSpecular(&object_beam_beam_tr1_s_t1_pera, false); // Tornado propeller
	ObjectForceSpecular(&object_beam_beam_tr1_s_t1_wingb, false); // Tornado wing that breaks off
	ObjectForceSpecular(&object_beam_beam_tr1_s_l_f4_3, true); // Sonic shoe left
	ObjectForceSpecular(&object_beam_beam_tr1_s_r_f4_3, true); // Sonic shoe right
	// Tornado 2
	ObjectForceSpecular(&object_tr2a_s_t2b_body_s_t2b_body, false); // Tornado body
	ObjectForceSpecular(&object_tr2a_s_t2b_body_s_t2b_wing, false); // Tornado wing
	ObjectForceSpecular(&object_tr2a_s_t2b_body_s_t2b_pera, false); // Tornado propeller
	ObjectForceSpecular(&object_tr2a_s_t2b_body_s_t2b_mark, false); // Tornado logos
	// Tornado 2 event
	ObjectForceSpecular(&_object_tr2a_s_t2b_body_s_t2b_body, false); // Tornado body
	ObjectForceSpecular(&_object_tr2a_s_t2b_body_s_t2b_wing, false); // Tornado wing
	ObjectForceSpecular(&_object_tr2a_s_t2b_body_s_t2b_pera, false); // Tornado propeller
	ObjectForceSpecular(&_object_tr2a_s_t2b_body_s_t2b_mark, false); // Tornado logos
	// Tornato 2 Big
	ObjectForceSpecular(&object_tr2_big_s_tru2_body_s_tru2_body, false); // Tornado body
	ObjectForceSpecular(&object_tr2_big_s_tru2_body_s_tru2_banr, false); // Tornado wing
	ObjectForceSpecular(&object_tr2_big_s_tru2_body_s_tru2_wina, false); // Tornado propeller
	ObjectForceSpecular(&object_tr2_big_s_tru2_body_s_tru2_mark, false); // Tornado logos
	// Tornado 2 transformation
	ObjectForceSpecular(&object_tr2_tf_event_s_tr2rt_s_tr2rt, true); // Tornado body
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_swt1e, false); // Wing
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_swt1b, false); // Wing
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_wat1f, false); // Wing
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_wat1c, false); // Wing
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_SONIC_2, true); // Sonic
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_miles_1, true); // Tails
	ObjectForceSpecular(&object_tr2_tf_event_s_tr2rt_s_l_f4_3, true); // Sonic left shoe
	ObjectForceSpecular(&object_tr2_tf_event_s_tr2rt_s_r_f4_3, true); // Sonic right shoe
	ObjectForceSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_handa, true); // Grabber
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_engn, false); // Engine bottom
	ObjectRemoveSpecular(&object_tr2_tf_event_s_tr2rt_s_t2tf_bjet, false); // Propeller front
	// Tornado 2 transformed
	ObjectForceSpecular(&object_tr2b_s_tru2_body_s_tru2_body, false); // Tornado body
	ObjectForceSpecular(&object_tr2b_s_tru2_body_s_tru2_banr, false); // Tornado bottom engine
	ObjectForceSpecular(&object_tr2b_s_tru2_body_s_tru2_wina, false); // Tornado wings
	ObjectForceSpecular(&object_tr2b_s_tru2_body_s_tru2_mark, false); // Tornado logos
	// Tornado 2 transformed event
	ObjectForceSpecular(&_object_tr2b_s_tru2_body_s_tru2_body, false); // Tornado body
	ObjectForceSpecular(&_object_tr2b_s_tru2_body_s_tru2_banr, false); // Tornado bottom engine
	ObjectForceSpecular(&_object_tr2b_s_tru2_body_s_tru2_wina, false); // Tornado wings
	ObjectForceSpecular(&_object_tr2b_s_tru2_body_s_tru2_mark, false); // Tornado logos
}
