#include "stdafx.h"
#include "ShaderParameter.h"

std::vector<IShaderParameter*> IShaderParameter::values_assigned {};

template<>
bool ShaderParameter<bool>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		const auto f = current ? 1.0f : 0.0f;
		float buffer[4] = { f, f, f, f };

		device->SetVertexShaderConstantF(index, buffer, 1);
		device->SetPixelShaderConstantF(index, buffer, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<int>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		const auto f = static_cast<float>(current);
		float buffer[4] = { f, f, f, f };

		device->SetVertexShaderConstantF(index, buffer, 1);
		device->SetPixelShaderConstantF(index, buffer, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<float>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		D3DXVECTOR4 value = { current, current, current, current };

		device->SetVertexShaderConstantF(index, value, 1);
		device->SetPixelShaderConstantF(index, value, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<D3DXVECTOR4>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		device->SetVertexShaderConstantF(index, current, 1);
		device->SetPixelShaderConstantF(index, current, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<D3DXVECTOR3>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		D3DXVECTOR4 value = { current.x, current.y, current.z, 1.0f };

		device->SetVertexShaderConstantF(index, value, 1);
		device->SetPixelShaderConstantF(index, value, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<D3DXCOLOR>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		static_assert(sizeof(D3DXCOLOR) == sizeof(D3DXVECTOR4), "D3DXCOLOR size does not match D3DXVECTOR4.");
		device->SetVertexShaderConstantF(index, current, 1);
		device->SetPixelShaderConstantF(index, current, 1);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<D3DXMATRIX>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		device->SetVertexShaderConstantF(index, current, 4);
		device->SetPixelShaderConstantF(index, current, 4);
		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
bool ShaderParameter<Texture>::commit(IDirect3DDevice9* device)
{
	if (is_modified())
	{
		device->SetTexture(index, current);

		clear();
		return true;
	}

	assigned = false;
	return false;
}

template<>
void ShaderParameter<Texture>::release()
{
	clear();
	current = nullptr;
	last = nullptr;
}
