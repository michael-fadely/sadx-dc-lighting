#include "stdafx.h"

#include "d3d.h"
#include <SADXModLoader.h>

#include "lantern.h"
#include "globals.h"

VoidFunc(sub_541990, 0x541990);
VoidFunc(sub_543F20, 0x543F20);
FunctionPointer(Sint32, QueueSound_DualEntity, (int, void*, int, int, int), 0x00423F50);
ObjectFunc(Obj_Past_Delete, 0x005419C0);
static constexpr auto sub_541FC0 = (void*)0x00541FC0;

static void __cdecl Obj_Past_Delete_r(ObjectMaster* _this)
{
	LanternInstance::SetBlendFactor(0.0f);
	Obj_Past_Delete(_this);
}

void __cdecl Obj_Past_r(ObjectMaster *_this)
{
	auto entity = _this->Data1;
	switch (entity->Action)
	{
		case 0:
			entity->InvulnerableTime = CurrentAct;
			sub_543F20();
			memset((void*)0x3C63690, 0, sizeof(ObjectMaster*) * 4);
			_this->DeleteSub = Obj_Past_Delete_r;
			entity->Action = 1;
			break;

		case 1:
			// teeny tiny usercall function. I ain't writin' no wrapper for that.
			__asm
			{
				mov eax, [_this]
				call sub_541FC0
			}
			entity->Action = 2;
			break;

		case 2:
			if (CurrentAct == entity->InvulnerableTime)
			{
				if (!entity->InvulnerableTime)
				{
					QueueSound_DualEntity(1108, entity, 1, 0, 2);
				}
				else if (CurrentAct == 2 && d3d::effect)
				{
					entity->Rotation.x += NJM_DEG_ANG(2.8125f);
					entity->Rotation.x %= 65536;
					auto f = (njSin(entity->Rotation.x) + 1.0f) / 2.0f;
					LanternInstance::SetBlendFactor(f);
					globals::palettes.SetSelfBlend(0, 5, 5);
				}
			}
			else
			{
				entity->Action = 3;
			}
			break;

		case 3:
			sub_541990();
			entity->Action = 1;
			break;

		default:
			break;
	}
}
