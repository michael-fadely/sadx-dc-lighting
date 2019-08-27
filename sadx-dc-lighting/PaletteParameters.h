#pragma once

#include <dirty_t.h>
#include <CBufferWriter.h>

struct PaletteIndexPair : dirty_impl
{
	dirty_t<uint> diffuse;
	dirty_t<uint> specular;

	bool operator==(const PaletteIndexPair& other) const;
	bool operator!=(const PaletteIndexPair& other) const;

	[[nodiscard]] bool dirty() const override;
	void clear() override;
	void mark() override;
};

CBufferBase& operator<<(CBufferBase& buffer, const PaletteIndexPair& pair);

struct PaletteBlendFactorPair : dirty_impl
{
	dirty_t<float> diffuse;
	dirty_t<float> specular;
	
	bool operator==(const PaletteBlendFactorPair& other) const;
	bool operator!=(const PaletteBlendFactorPair& other) const;

	[[nodiscard]] bool dirty() const override;
	void clear() override;
	void mark() override;
};

CBufferBase& operator<<(CBufferBase& buffer, const PaletteBlendFactorPair& pair);

struct PaletteParameters : ICBuffer, dirty_impl
{
	PaletteIndexPair       base_indices;
	PaletteIndexPair       blend_indices;
	PaletteBlendFactorPair blend_factors;

	void write(CBufferBase& cbuf) const override;
	[[nodiscard]] bool dirty() const override;
	void clear() override;
	void mark() override;
};
