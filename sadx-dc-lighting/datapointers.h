#pragma once

#include <ModLoader/MemAccess.h>
#include <d3d8types.hpp>
#include <ninja.h>

DataPointer(HWND, WindowHandle, 0x03D0FD30);
DataPointer(D3DLIGHT8, Direct3D_CurrentLight, 0x03ABDB50);
DataPointer(NJS_TEXLIST*, Direct3D_CurrentTexList, 0x03D0FA24);
DataPointer(Uint32, _nj_constant_and_attr, 0x03D0F840);
DataPointer(Uint32, _nj_constant_or_attr, 0x03D0F9C4);
DataPointer(Uint32, _nj_control_3d, 0x03D0F9C8);
