#pragma once

#include <ninja.h>

#pragma region enums

enum QueuedModelType : __int8
{
	QueuedModelType_00         = 0x0,
	QueuedModelType_BasicModel = 0x1,
	QueuedModelType_Sprite2D   = 0x2,
	QueuedModelType_Sprite3D   = 0x3,
	QueuedModelType_04         = 0x4,
	QueuedModelType_05         = 0x5,
	QueuedModelType_06         = 0x6,
	QueuedModelType_07         = 0x7,
	QueuedModelType_08         = 0x8,
	QueuedModelType_Rect       = 0x9,
	QueuedModelType_10         = 0xA,
	QueuedModelType_11         = 0xB,
	QueuedModelType_Callback   = 0xC,
	QueuedModelType_13         = 0xD,
	QueuedModelType_14         = 0xE,
	QueuedModelType_15         = 0xF,
};

enum QueuedModelFlags
{
	QueuedModelFlags_ZTestWrite = 0x10,
	QueuedModelFlags_FogEnabled = 0x20,
	QueuedModelFlags_02         = 0x40,
	QueuedModelFlags_03         = 0x80,
};

enum QueuedModelFlagsB : __int8
{
	QueuedModelFlagsB_EnableZWrite     = 0x1,
	QueuedModelFlagsB_NoColor          = 0x2,
	QueuedModelFlagsB_SomeTextureThing = 0x4,
	QueuedModelFlagsB_3                = 0x8,
	QueuedModelFlagsB_AlwaysShow       = 0x10,
};

#pragma endregion

#pragma region structs

#pragma pack(push, 8)
struct QueuedModelNode
{
	QueuedModelNode *Next;
	float Depth;
	QueuedModelType Type;
	char BlendMode;
	__int16 TexNum;
	NJS_TEXLIST *TexList;
	NJS_ARGB Color;
	int Control3D;
	int ConstantAndAttr;
	int ConstantOrAttr;
};

static_assert(sizeof(QueuedModelNode) == 0x2C, "NOPE");

struct __declspec(align(8)) QueuedModelCallback
{
	QueuedModelNode Node;
	__declspec(align(8)) NJS_MATRIX Transform;
	void* UserFunction;
	void* UserData;
};

struct QueuedModelRect
{
	QueuedModelNode Node;
	float field_2C;
	float Left;
	float Top;
	float Right;
	float Bottom;
	float Depth;
	NJS_COLOR Color;
};

struct QueuedModelSprite
{
	QueuedModelNode Node;
	__declspec(align(8)) NJS_SPRITE Sprite;
	Uint32 SpriteFlags;
	float SpritePri;
	NJS_MATRIX Transform;
};

struct __declspec(align(4)) QueuedModelPointer
{
	QueuedModelNode Node;
	NJS_MODEL_SADX *Model;
	NJS_MATRIX Transform;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct QueuedModelParticle
{
	QueuedModelNode Node;
	char idk[36];
};

struct QueuedModel
{
	NJS_MODEL_SADX model;
	NJS_MESHSET_SADX meshset;
};

static_assert(sizeof(QueuedModel) == 0x48, "NOPE");

#pragma pack(pop)

#pragma pack(push, 8)
struct __declspec(align(4)) DrawQueueHead
{
	void* next;
	void* prev;
};
#pragma pack(pop)

union QueuedModelUnion
{
	QueuedModelNode Node;
	QueuedModelPointer BasicModel;
	QueuedModelSprite Sprite;
	QueuedModelRect Rect;
	QueuedModelCallback Callback;
};

struct QueuedModelUnion_t
{
	QueuedModelUnion _;
};

#pragma endregion