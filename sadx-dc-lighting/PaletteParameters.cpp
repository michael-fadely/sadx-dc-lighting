#include "stdafx.h"
#include "PaletteParameters.h"

bool PaletteIndexPair::operator==(const PaletteIndexPair& other) const
{
	return diffuse.data() == other.diffuse.data() &&
	       specular.data() == other.specular.data();
}

bool PaletteIndexPair::operator!=(const PaletteIndexPair& other) const
{
	return !(*this == other);
}

bool PaletteIndexPair::dirty() const
{
	return diffuse.dirty() || specular.dirty();
}

void PaletteIndexPair::clear()
{
	diffuse.clear();
	specular.clear();
}

void PaletteIndexPair::mark()
{
	diffuse.mark();
	specular.mark();
}

CBufferBase& operator<<(CBufferBase& buffer, const PaletteIndexPair& pair)
{
	return buffer << pair.diffuse << pair.specular;
}

bool PaletteBlendFactorPair::operator==(const PaletteBlendFactorPair& other) const
{
	return diffuse.data() == other.diffuse.data() &&
	       specular.data() == other.specular.data();
}

bool PaletteBlendFactorPair::operator!=(const PaletteBlendFactorPair& other) const
{
	return !(*this == other);
}

bool PaletteBlendFactorPair::dirty() const
{
	return diffuse.dirty() || specular.dirty();
}

void PaletteBlendFactorPair::clear()
{
	diffuse.clear();
	specular.clear();
}

void PaletteBlendFactorPair::mark()
{
	diffuse.mark();
	specular.mark();
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
