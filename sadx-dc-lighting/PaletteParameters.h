#pragma once

#include <dirty_t.h>

struct PaletteIndexPair
{
	uint diffuse;
	uint specular;

	bool operator==(const PaletteIndexPair& other) const;
	bool operator!=(const PaletteIndexPair& other) const;
};

CBufferBase& operator<<(CBufferBase& buffer, const PaletteIndexPair& pair);

struct PaletteBlendFactorPair
{
	float diffuse;
	float specular;
	
	bool operator==(const PaletteBlendFactorPair& other) const;
	bool operator!=(const PaletteBlendFactorPair& other) const;
};

CBufferBase& operator<<(CBufferBase& buffer, const PaletteBlendFactorPair& pair);

struct PaletteParameters : ICBuffer, dirty_impl
{
	dirty_t<PaletteIndexPair>       base_indices;
	dirty_t<PaletteIndexPair>       blend_indices;
	dirty_t<PaletteBlendFactorPair> blend_factors;

	void write(CBufferBase& cbuf) const override;
	[[nodiscard]] bool dirty() const override;
	void clear() override;
	void mark() override;
};
