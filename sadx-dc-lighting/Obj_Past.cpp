#include "stdafx.h"

#include "d3d.h"
#include <SADXModLoader.h>
#include <Trampoline.h>

#include "../include/lanternapi.h"

VoidFunc(sub_541990, 0x541990);
VoidFunc(sub_543F20, 0x543F20);

static const auto sub_541FC0 = reinterpret_cast<void*>(0x00541FC0);

static Trampoline* Obj_Past_t = nullptr;

static void __cdecl Obj_Past_Delete_r(ObjectMaster* _this)
{
	// Disable blending in the shader so it doesn't do extra work.
	set_shader_flags(ShaderFlags_Blend, false);

	// Reset blend indices.
	set_blend(-1, -1);

	Obj_Past_Delete(_this);
}

static void __cdecl Obj_Past_r(ObjectMaster *_this)
{
	auto entity = _this->Data1;
	switch (entity->Action)
	{
		case 0:
			entity->InvulnerableTime = CurrentAct;
			sub_543F20();
			memset(reinterpret_cast<void*>(0x3C63690), 0, sizeof(ObjectMaster*) * 4);
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
					QueueSound_DualEntity(1108, entity, 1, nullptr, 2);
				}
				else if (CurrentAct == 2 && !d3d::shaders_null())
				{
					entity->Rotation.x += NJM_DEG_ANG(4.561875f);
					entity->Rotation.x %= 65536;
					auto f = (njSin(entity->Rotation.x) + 1.0f) / 2.0f;

					// Enables palette blending in the shader.
					set_shader_flags(ShaderFlags_Blend, true);

					// Blend both diffuse and specular index 0 to index 5.
					set_blend(0, 5);
					// Set the blend factor, where 0 is the source index and 1 is the target.
					set_blend_factor(f);
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

void Past_Init()
{
	Obj_Past_t = new Trampoline(0x005420C0, 0x005420C5, Obj_Past_r);
}
