#pragma once

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

	struct Type
	{
		enum T
		{
			vertex = 0b01,
			pixel = 0b10,
			both = 0b11
		};
	};

	static std::vector<IShaderParameter*> values_assigned;

	virtual ~IShaderParameter() = default;
	virtual bool is_modified() = 0;
	virtual void clear() = 0;
	virtual bool commit(IDirect3DDevice9* device) = 0;
	virtual bool commit_now(IDirect3DDevice9* device) = 0;
	virtual void release() = 0;
};

template<typename T>
class ShaderParameter : public IShaderParameter
{
	const int index;
	const Type::T type;

	bool reset = false;
	bool assigned = false;
	T last;
	T current;

public:
	ShaderParameter(const int index, const T& default_value, const Type::T type) :
		index(index),
		type(type),
		last(default_value),
		current(default_value)
	{
	}

	bool is_modified() override;
	void clear() override;
	bool commit(IDirect3DDevice9* device) override;
	bool commit_now(IDirect3DDevice9* device) override;
	void release() override;
	T value() const;
	ShaderParameter<T>& operator=(const T& value);
	ShaderParameter<T>& operator=(const ShaderParameter<T>& value);
};

template <typename T>
bool ShaderParameter<T>::is_modified()
{
	return reset || assigned && last != current;
}

template <typename T>
void ShaderParameter<T>::clear()
{
	reset = false;
	assigned = false;
	last = current;
}

template <typename T>
bool ShaderParameter<T>::commit_now(IDirect3DDevice9* device)
{
	reset = true;
	return commit(device);
}

template <typename T>
void ShaderParameter<T>::release()
{
	clear();
}
template <typename T>
T ShaderParameter<T>::value() const
{
	return current;
}

template <typename T>
ShaderParameter<T>& ShaderParameter<T>::operator=(const T& value)
{
	if (!assigned)
	{
		values_assigned.push_back(this);
	}

	assigned = true;
	current = value;
	return *this;
}

template <typename T>
ShaderParameter<T>& ShaderParameter<T>::operator=(const ShaderParameter<T>& value)
{
	*this = value.current;
	return *this;
}

template<> bool ShaderParameter<bool>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<int>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<float>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXVECTOR4>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXVECTOR3>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXVECTOR2>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXCOLOR>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<D3DXMATRIX>::commit(IDirect3DDevice9* device);
template<> bool ShaderParameter<Texture>::commit(IDirect3DDevice9* device);
template<> void ShaderParameter<Texture>::release();
