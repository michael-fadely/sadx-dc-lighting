#pragma once

#include <string>
#include <atlbase.h>
#include <d3d9.h>
#include <d3dx9effect.h>

using Texture = CComPtr<IDirect3DTexture9>;

class IEffectParameter
{
public:
	virtual ~IEffectParameter() = default;
	virtual void UpdateHandle() = 0;
	virtual bool Modified() = 0;
	virtual void Clear() = 0;
	virtual void Commit() = 0;
};

template<typename T>
class EffectParameter : public IEffectParameter
{
	const std::string name;
	ID3DXEffect** effect;
	D3DXHANDLE handle;
	bool reset;
	bool assigned;
	T last;
	T current;

public:
	explicit EffectParameter(ID3DXEffect** effect, const std::string& name, const T& defaultValue)
		: name(name), effect(effect), handle(nullptr), reset(false), assigned(false), last(defaultValue), current(defaultValue)
	{
	}

	void UpdateHandle() override;
	bool Modified() override;
	void Clear() override;
	void Commit() override;
	void operator=(const T& value);
	void operator=(const EffectParameter<T>& value);
};

template <typename T>
bool EffectParameter<T>::Modified()
{
	return reset || assigned && last != current;
}

template <typename T>
void EffectParameter<T>::UpdateHandle()
{
	reset = handle != nullptr;
	handle = (*effect)->GetParameterByName(nullptr, name.c_str());
	Commit();
}

template <typename T>
void EffectParameter<T>::Clear()
{
	reset = false;
	assigned = false;
	last = current;
}

template <typename T>
void EffectParameter<T>::operator=(const T& value)
{
	assigned = true;
	current = value;
}

template <typename T>
void EffectParameter<T>::operator=(const EffectParameter<T>& value)
{
	*this = value.current;
}

template<> void EffectParameter<bool>::Commit();
template<> void EffectParameter<int>::Commit();
template<> void EffectParameter<float>::Commit();
template<> void EffectParameter<D3DXVECTOR4>::Commit();
template<> void EffectParameter<D3DXVECTOR3>::Commit();
template<> void EffectParameter<D3DXCOLOR>::Commit();
template<> void EffectParameter<D3DXMATRIX>::Commit();
template<> void EffectParameter<Texture>::Commit();
