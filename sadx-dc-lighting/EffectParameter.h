#pragma once

#include <string>
#include <vector>
#include <atlbase.h>
#include <d3d9.h>
#include <d3dx9effect.h>

using Texture = CComPtr<IDirect3DTexture9>;
using Effect = CComPtr<ID3DXEffect>;

class IEffectParameter
{
public:
	static std::vector<IEffectParameter*> values_assigned;

	virtual ~IEffectParameter() = default;
	virtual void UpdateHandle(Effect effect) = 0;
	virtual bool Modified() = 0;
	virtual void Clear() = 0;
	virtual bool Commit(Effect effect) = 0;
	virtual void Release() = 0;
};

template<typename T>
class EffectParameter : public IEffectParameter
{
	const char* name;
	D3DXHANDLE handle;
	bool reset;
	bool assigned;
	T last;
	T current;

public:
	explicit EffectParameter(const char* name, const T& defaultValue)
		: name(name), handle(nullptr), reset(false), assigned(false), last(defaultValue), current(defaultValue)
	{
	}

	void UpdateHandle(Effect effect) override;
	bool Modified() override;
	void Clear() override;
	bool Commit(Effect effect) override;
	void Release() override;
	void operator=(const T& value);
	void operator=(const EffectParameter<T>& value);
};

template <typename T>
bool EffectParameter<T>::Modified()
{
	return reset || assigned && last != current;
}

template <typename T>
void EffectParameter<T>::UpdateHandle(Effect effect)
{
	reset = handle != nullptr;
	handle = effect->GetParameterByName(nullptr, name);
	Commit(effect);
}

template <typename T>
void EffectParameter<T>::Clear()
{
	reset = false;
	assigned = false;
	last = current;
}

template <typename T>
void EffectParameter<T>::Release()
{
	Clear();
}

template <typename T>
void EffectParameter<T>::operator=(const T& value)
{
	if (!assigned)
	{
		values_assigned.push_back(this);
	}

	assigned = true;
	current = value;
}

template <typename T>
void EffectParameter<T>::operator=(const EffectParameter<T>& value)
{
	*this = value.current;
}

template<> bool EffectParameter<bool>::Commit(Effect effect);
template<> bool EffectParameter<int>::Commit(Effect effect);
template<> bool EffectParameter<float>::Commit(Effect effect);
template<> bool EffectParameter<D3DXVECTOR4>::Commit(Effect effect);
template<> bool EffectParameter<D3DXVECTOR3>::Commit(Effect effect);
template<> bool EffectParameter<D3DXCOLOR>::Commit(Effect effect);
template<> bool EffectParameter<D3DXMATRIX>::Commit(Effect effect);
template<> bool EffectParameter<Texture>::Commit(Effect effect);
template<> void EffectParameter<Texture>::Release();
