#pragma once

#include <ModLoader/MemAccess.h>
#include <d3d8types.hpp>
#include <ninja.h>

DataPointer(HWND, WindowHandle, 0x03D0FD30);
DataPointer(D3DLIGHT8, Direct3D_CurrentLight, 0x03ABDB50);
DataPointer(NJS_TEXLIST*, Direct3D_CurrentTexList, 0x03D0FA24);
DataPointer(Uint32, _nj_constant_attr_and_, 0x03D0F840);
DataPointer(Uint32, _nj_constant_attr_or_, 0x03D0F9C4);
DataPointer(Uint32, _nj_control_3d_flag_, 0x03D0F9C8);
DataArray(HMODULE, ModuleHandles, 0x03CA6E60, 4);
DataPointer(EntityData1*, Camera_Data1, 0x03B2CBB0);
