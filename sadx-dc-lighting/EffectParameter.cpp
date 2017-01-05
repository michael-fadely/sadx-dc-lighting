#include "stdafx.h"
#include "EffectParameter.h"

template<>
void EffectParameter<IDirect3DTexture9*>::operator=(IDirect3DTexture9* const& value)
{
	// Comparing pointers is almost certainly faster
	// than just making the add/release calls.
	if (value != current)
	{
		if (value)
		{
			value->AddRef();
		}

		if (current)
		{
			current->Release();
		}
	}

	assigned = true;
	current = value;
}

template<>
void EffectParameter<bool>::Commit()
{
	if (Modified())
	{
		(*effect)->SetBool(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<int>::Commit()
{
	if (Modified())
	{
		(*effect)->SetInt(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<float>::Commit()
{
	if (Modified())
	{
		(*effect)->SetFloat(handle, current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR4>::Commit()
{
	if (Modified())
	{
		(*effect)->SetVector(handle, &current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXVECTOR3>::Commit()
{
	if (Modified())
	{
		D3DXVECTOR4 v = D3DXVECTOR4(current, 0.0f);
		(*effect)->SetVector(handle, &v);
		Clear();
	}
}

template<>
void EffectParameter<D3DXCOLOR>::Commit()
{
	if (Modified())
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		(*effect)->SetVector(handle, (D3DXVECTOR4*)&current);
		Clear();
	}
}

template<>
void EffectParameter<D3DXMATRIX>::Commit()
{
	if (Modified())
	{
		(*effect)->SetMatrix(handle, &current);
		Clear();
	}
}

template<>
void EffectParameter<IDirect3DTexture9*>::Commit()
{
	if (Modified())
	{
		(*effect)->SetTexture(handle, current);
		Clear();
	}
}
