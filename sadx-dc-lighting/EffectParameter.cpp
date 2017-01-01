#include "stdafx.h"
#include "EffectParameter.h"

template<>
void EffectParameter<bool>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetBool(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<int>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetInt(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<float>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetFloat(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR4>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetVector(handle, &current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR3>::Commit()
{
	if (assigned && last != current)
	{
		D3DXVECTOR4 v = D3DXVECTOR4(current, 0.0f);
		(*effect)->SetVector(handle, &v);
		Clear();
	}
}

template<>
void EffectParameter<D3DXCOLOR>::Commit()
{
	if (assigned && last != current)
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		(*effect)->SetVector(handle, (D3DXVECTOR4*)&current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXMATRIX>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetMatrix(handle, &current);
		Clear();
	}
}

template<>
void EffectParameter<IDirect3DTexture9*>::Commit()
{
	if (assigned && last != current)
	{
		(*effect)->SetTexture(handle, current);
		Clear();
	}
}
