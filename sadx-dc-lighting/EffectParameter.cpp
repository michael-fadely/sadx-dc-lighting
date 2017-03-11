#include "stdafx.h"
#include "lantern.h"
#include "EffectParameter.h"

template<>
void EffectParameter<bool>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetBool(handle, current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<int>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetInt(handle, current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<float>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetFloat(handle, current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR4>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetVector(handle, &current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR3>::Commit(Effect effect)
{
	if (Modified())
	{
		D3DXVECTOR4 v = D3DXVECTOR4(current, 0.0f);
		effect->SetVector(handle, &v);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR2>::Commit(Effect effect)
{
	if (Modified())
	{
		D3DXVECTOR4 v = D3DXVECTOR4(current.x, current.y, 0.0f, 0.0f);
		effect->SetVector(handle, &v);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<D3DXCOLOR>::Commit(Effect effect)
{
	if (Modified())
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		effect->SetVector(handle, (D3DXVECTOR4*)&current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<D3DXMATRIX>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetMatrix(handle, &current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<Texture>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetTexture(handle, current);
		effect->CommitChanges();
		Clear();
	}
}

template<>
void EffectParameter<Texture>::Release()
{
	Clear();
	current = nullptr;
	last = nullptr;
}
