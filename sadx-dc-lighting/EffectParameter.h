#pragma once

#include <string>
#include <d3d9.h>
#include <d3dx9effect.h>

class IEffectParameter
{
public:
	virtual ~IEffectParameter() = default;
	virtual void UpdateHandle() = 0;
	virtual void ClearModified() = 0;
	virtual void SetValue() = 0;
};

template<typename T>
class EffectParameter : public IEffectParameter
{
	const std::string name;
	ID3DXEffect** effect;
	D3DXHANDLE handle;
	bool modified;
	T last;
	T data;

public:
	explicit EffectParameter(ID3DXEffect** effect, const std::string& name, const T& defaultValue)
		: name(name), effect(effect), handle(nullptr), modified(false), last(defaultValue), data(defaultValue)
	{
	}

	EffectParameter<T>& operator=(EffectParameter<T>&& inst) noexcept;

	void UpdateHandle() override;
	void ClearModified() override;
	void SetValue() override;
	void operator=(const T& value);
};


template <typename T>
EffectParameter<T>& EffectParameter<T>::operator=(EffectParameter<T>&& inst) noexcept
{
	name     = inst.name;
	effect   = inst.effect;
	handle   = inst.handle;
	modified = inst.modified;
	last     = inst.last;
	data     = inst.data;

	inst.effect = nullptr;
	return *this;
}

template <typename T>
void EffectParameter<T>::UpdateHandle()
{
	bool was_null = handle == nullptr;
	handle = (*effect)->GetParameterByName(nullptr, name.c_str());

	if (!was_null)
	{
		modified = true;
		SetValue();
	}
}

template <typename T>
void EffectParameter<T>::ClearModified()
{
	last = data;
	modified = false;
}

template <typename T>
void EffectParameter<T>::operator=(const T& value)
{
	modified = !!(last != value);
	data = value;
}

template<> void EffectParameter<D3DXMATRIX>::operator=(const D3DXMATRIX& value);

template<> void EffectParameter<bool>::SetValue();
template<> void EffectParameter<int>::SetValue();
template<> void EffectParameter<float>::SetValue();
template<> void EffectParameter<D3DXVECTOR4>::SetValue();
template<> void EffectParameter<D3DXVECTOR3>::SetValue();
template<> void EffectParameter<D3DXCOLOR>::SetValue();
template<> void EffectParameter<D3DXMATRIX>::SetValue();
template<> void EffectParameter<IDirect3DTexture9*>::SetValue();
