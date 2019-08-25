#include "stdafx.h"
#include "PaletteParameters.h"

bool PaletteIndexPair::operator==(const PaletteIndexPair& other) const
{
	return diffuse == other.diffuse &&
	       specular == other.specular;
}

bool PaletteIndexPair::operator!=(const PaletteIndexPair& other) const
{
	return !(*this == other);
}

CBufferBase& operator<<(CBufferBase& buffer, const PaletteIndexPair& pair)
{
	return buffer << pair.diffuse << pair.specular;
}

bool PaletteBlendFactorPair::operator==(const PaletteBlendFactorPair& other) const
{
	return diffuse == other.diffuse &&
	       specular == other.specular;
}

bool PaletteBlendFactorPair::operator!=(const PaletteBlendFactorPair& other) const
{
	return !(*this == other);
}

CBufferBase& operator<<(CBufferBase& buffer, const PaletteBlendFactorPair& pair)
{
	return buffer << pair.diffuse << pair.specular;
}

void PaletteParameters::write(CBufferBase& cbuf) const
{
	cbuf << base_indices << blend_indices << blend_factors;
}

bool PaletteParameters::dirty() const
{
	return base_indices.dirty() ||
	       blend_indices.dirty() ||
	       blend_factors.dirty();
}

void PaletteParameters::mark()
{
	base_indices.mark();
	blend_indices.mark();
	blend_factors.mark();
}

void PaletteParameters::clear()
{
	base_indices.clear();
	blend_indices.clear();
	blend_factors.clear();
}
