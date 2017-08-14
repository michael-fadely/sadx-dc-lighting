#pragma once

#include <string>
#include <vector>
#include <atlbase.h>
#include <d3d9.h>
#include <d3dx9effect.h>

using VertexShader = CComPtr<IDirect3DVertexShader9>;
using PixelShader  = CComPtr<IDirect3DPixelShader9>;
using Buffer       = CComPtr<ID3DXBuffer>;
using Texture      = CComPtr<IDirect3DTexture9>;

class IShaderParameter
{
public:
	static std::vector<IShaderParameter*> values_assigned;

	virtual ~IShaderParameter() = default;
	virtual bool Modified() = 0;
	virtual void Clear() = 0;
	virtual bool Commit(IDirect3DDevice9* device) = 0;
	virtual void Release() = 0;
};

template<typename T>
class ShaderParameter : public IShaderParameter
{
	enum ShaderParameterType
	{
		SHPARAM_VS = 1 << 0,
		SHPARAM_PS = 1 << 1
	};

	// TODO:
	const int type = SHPARAM_VS | SHPARAM_PS;

	int index;
	bool reset;
	bool assigned;
	T last;
	T current;

public:
	explicit ShaderParameter(int index, const T& defaultValue)
		: index(index),
		  reset(false),
		  assigned(false),
		  last(defaultValue),
		  current(defaultValue)
	{
	}

	bool Modified() override;
	void Clear() override;
	bool Commit(IDirect3DDevice9* device) override;
	void Release() override;
	T Value() const;
	void operator=(const T& value);
	void operator=(const ShaderParameter<T>& value);
};

template <typename T>
bool ShaderParameter<T>::Modified()
{
	return reset || assigned && last != current;
}

template <typename T>
void ShaderParameter<T>::Clear()
{
	reset = false;
	assigned = false;
	last = current;
}

template <typename T>
void ShaderParameter<T>::Release()
{
	Clear();
}
template <typename T>
T ShaderParameter<T>::Value() const
{
	return current;
}

template <typename T>
void ShaderParameter<T>::operator=(const T& value)
{
	if (!assigned)
	{
		values_assigned.push_back(this);
	}

	assigned = true;
	current = value;
}

template <typename T>
void ShaderParameter<T>::operator=(const ShaderParameter<T>& value)
{
	*this = value.current;
}

template<> bool ShaderParameter<bool>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<int>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<float>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXVECTOR4>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXVECTOR3>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXCOLOR>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXMATRIX>::Commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<Texture>::Commit(IDirect3DDevice9* device);
template<> void ShaderParameter<Texture>::Release();
