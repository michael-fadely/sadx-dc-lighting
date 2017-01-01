#pragma once

#include <string>
#include <d3d9.h>
#include <d3dx9effect.h>

class IEffectParameter
{
public:
	virtual ~IEffectParameter() = default;
	virtual void UpdateHandle() = 0;
	virtual void Clear() = 0;
	virtual void Commit() = 0;
};

template<typename T>
class EffectParameter : public IEffectParameter
{
	const std::string name;
	ID3DXEffect** effect;
	D3DXHANDLE handle;
	bool assigned;
	T last;
	T current;

public:
	explicit EffectParameter(ID3DXEffect** effect, const std::string& name, const T& defaultValue)
		: name(name), effect(effect), handle(nullptr), assigned(false), last(defaultValue), current(defaultValue)
	{
	}

	void UpdateHandle() override;
	void Clear() override;
	void Commit() override;
	void operator=(const T& value);
};

template <typename T>
void EffectParameter<T>::UpdateHandle()
{
	bool was_null = handle == nullptr;
	handle = (*effect)->GetParameterByName(nullptr, name.c_str());

	if (!was_null)
	{
		Commit();
	}
}

template <typename T>
void EffectParameter<T>::Clear()
{
	assigned = false;
	last = current;
}

template <typename T>
void EffectParameter<T>::operator=(const T& value)
{
	assigned = true;
	current = value;
}

template<> void EffectParameter<bool>::Commit();
template<> void EffectParameter<int>::Commit();
template<> void EffectParameter<float>::Commit();
template<> void EffectParameter<D3DXVECTOR4>::Commit();
template<> void EffectParameter<D3DXVECTOR3>::Commit();
template<> void EffectParameter<D3DXCOLOR>::Commit();
template<> void EffectParameter<D3DXMATRIX>::Commit();
template<> void EffectParameter<IDirect3DTexture9*>::Commit();
