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

template <typename T>
class ShaderParameter final : public IShaderParameter
{
	const int index_;
	const Type::T type_;

	bool reset_ = false;
	bool assigned_ = false;
	T last_;
	T current_;

public:
	ShaderParameter(const int index, const T& default_value, const Type::T type)
		: index_(index),
		  type_(type),
		  last_(default_value),
		  current_(default_value)
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
	return reset_ || (assigned_ && last_ != current_);
}

template <typename T>
void ShaderParameter<T>::clear()
{
	reset_ = false;
	assigned_ = false;
	last_ = current_;
}

template <typename T>
bool ShaderParameter<T>::commit_now(IDirect3DDevice9* device)
{
	reset_ = true;
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
	return current_;
}

template <typename T>
ShaderParameter<T>& ShaderParameter<T>::operator=(const T& value)
{
	if (!assigned_)
	{
		values_assigned.push_back(this);
	}

	assigned_ = true;
	current_ = value;
	return *this;
}

template <typename T>
ShaderParameter<T>& ShaderParameter<T>::operator=(const ShaderParameter<T>& value)
{
	*this = value.current_;
	return *this;
}

template <> bool ShaderParameter<bool>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<int>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<float>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<D3DXVECTOR4>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<D3DXVECTOR3>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<D3DXVECTOR2>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<D3DXCOLOR>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<D3DXMATRIX>::commit(IDirect3DDevice9* device);
template <> bool ShaderParameter<Texture>::commit(IDirect3DDevice9* device);
template <> void ShaderParameter<Texture>::release();
