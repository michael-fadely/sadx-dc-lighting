#include "stdafx.h"
#include "ShaderParameter.h"

std::vector<IShaderParameter*> IShaderParameter::values_assigned {};

template <>
bool ShaderParameter<bool>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		const auto f = current_ ? 1.0f : 0.0f;
		const float buffer[4] = { f, f, f, f };

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, buffer, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, buffer, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<int>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		const auto f = static_cast<float>(current_);
		const float buffer[4] = { f, f, f, f };

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, buffer, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, buffer, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<float>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		D3DXVECTOR4 value = { current_, current_, current_, current_ };

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, value, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, value, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<D3DXVECTOR4>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, current_, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, current_, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<D3DXVECTOR3>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		D3DXVECTOR4 value = { current_.x, current_.y, current_.z, 0.0f };

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, value, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, value, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<D3DXVECTOR2>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		D3DXVECTOR4 value = { current_.x, current_.y, 0.0f, 1.0f };

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, value, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, value, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<D3DXCOLOR>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");

		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, current_, 1);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, current_, 1);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<D3DXMATRIX>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		if (type_ & Type::vertex)
		{
			device->SetVertexShaderConstantF(index_, current_, 4);
		}

		if (type_ & Type::pixel)
		{
			device->SetPixelShaderConstantF(index_, current_, 4);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
bool ShaderParameter<Texture>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		if (type_ & Type::vertex)
		{
			device->SetTexture(D3DVERTEXTEXTURESAMPLER0 + index_, current_);
		}

		if (type_ & Type::pixel)
		{
			device->SetTexture(index_, current_);
		}

		clear();
		return true;
	}

	assigned_ = false;
	return false;
}

template <>
void ShaderParameter<Texture>::release()
{
	clear();
	current_ = nullptr;
	last_ = nullptr;
}
