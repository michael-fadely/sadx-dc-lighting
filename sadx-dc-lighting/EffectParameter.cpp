#include "stdafx.h"
#include "lantern.h"
#include "EffectParameter.h"

template<>
bool EffectParameter<bool>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetBool(handle, current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<int>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetInt(handle, current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<float>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetFloat(handle, current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<D3DXVECTOR4>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetVector(handle, &current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<D3DXVECTOR3>::Commit(Effect effect)
{
	if (Modified())
	{
		D3DXVECTOR4 v = D3DXVECTOR4(current, 0.0f);
		effect->SetVector(handle, &v);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<D3DXCOLOR>::Commit(Effect effect)
{
	if (Modified())
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		effect->SetVector(handle, (D3DXVECTOR4*)&current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<D3DXMATRIX>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetMatrix(handle, &current);
		Clear();
		return true;
	}

	return false;
}

template<>
bool EffectParameter<Texture>::Commit(Effect effect)
{
	if (Modified())
	{
		effect->SetTexture(handle, current);
		Clear();
		return true;
	}

	return false;
}

template<>
void EffectParameter<Texture>::Release()
{
	Clear();
	current = nullptr;
	last = nullptr;
}
