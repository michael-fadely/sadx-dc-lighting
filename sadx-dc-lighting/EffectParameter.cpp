#include "stdafx.h"
#include "EffectParameter.h"

template<>
void EffectParameter<D3DXMATRIX>::operator=(const D3DXMATRIX& value)
{
	if (!modified)
	{
		modified = !!(data != value);
	}

	if (modified)
	{
		data = value;
	}
}

template<>
void EffectParameter<bool>::SetValue()
{
	if (modified)
	{
		(*effect)->SetBool(handle, data);
		ClearModified();
	}
}

template<>
void EffectParameter<int>::SetValue()
{
	if (modified)
	{
		(*effect)->SetInt(handle, data);
		ClearModified();
	}
}

template<>
void EffectParameter<float>::SetValue()
{
	if (modified)
	{
		(*effect)->SetFloat(handle, data);
		ClearModified();
	}
}

template<>
void EffectParameter<D3DXVECTOR4>::SetValue()
{
	if (modified)
	{
		(*effect)->SetVector(handle, &data);
		ClearModified();
	}
}

template<>
void EffectParameter<D3DXVECTOR3>::SetValue()
{
	if (modified)
	{
		D3DXVECTOR4 v = D3DXVECTOR4(data, 0.0f);
		(*effect)->SetVector(handle, &v);
		ClearModified();
	}
}

template<>
void EffectParameter<D3DXCOLOR>::SetValue()
{
	if (modified)
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		(*effect)->SetVector(handle, (D3DXVECTOR4*)&data);
		ClearModified();
	}
}

template<>
void EffectParameter<D3DXMATRIX>::SetValue()
{
	if (modified)
	{
		(*effect)->SetMatrix(handle, &data);
		ClearModified();
	}
}

template<>
void EffectParameter<IDirect3DTexture9*>::SetValue()
{
	if (modified)
	{
		(*effect)->SetTexture(handle, data);
		ClearModified();
	}
}
