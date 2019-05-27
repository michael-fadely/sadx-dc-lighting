#pragma once

#include <ninja.h>

struct DirLightData
{
	char LevelID;
	char Act;
	NJS_VECTOR LightDirection;
	float R;
	float G;
	float B;
	float Specular;
	float Diffuse;
	float Ambient;
};

struct DirLightData_hlsl
{
	NJS_VECTOR direction;
	NJS_VECTOR color;
	float      specular_m;
	float      diffuse_m;
	float      ambient_m;

	bool operator==(const DirLightData_hlsl& other) const;
	bool operator!=(const DirLightData_hlsl& other) const;
};
