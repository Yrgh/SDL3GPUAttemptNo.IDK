#pragma once

#include "common.h"

// TODO: Add more of these
enum MeshAttribute {
	MESHATTRIBUTE_INVALID = U32_BAD,

	MESHATTRIBUTE_FLOAT = 0,
	MESHATTRIBUTE_FLOAT2,
	MESHATTRIBUTE_FLOAT3,
	MESHATTRIBUTE_FLOAT4,

	MESHATTRIBUTE_MAX
};

inline constexpr u32 mesh_attribute_sizes[MESHATTRIBUTE_MAX] = {
	4,
	8,
	12,
	16,
};

inline constexpr MeshAttribute SDL_GPUVertexElementFormat_to_common(SDL_GPUVertexElementFormat attribute) {
	switch (attribute) {
	case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT:
		return MESHATTRIBUTE_FLOAT;
	case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2:
		return MESHATTRIBUTE_FLOAT2;
	case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3:
		return MESHATTRIBUTE_FLOAT3;
	case SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4:
		return MESHATTRIBUTE_FLOAT4;
	default:
		return MESHATTRIBUTE_INVALID;
	}
}

inline constexpr SDL_GPUVertexElementFormat common_to_SDL_GPUVertexElementFormat(MeshAttribute attribute) {
	switch (attribute) {
	case MESHATTRIBUTE_FLOAT:
		return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	case MESHATTRIBUTE_FLOAT2:
		return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
	case MESHATTRIBUTE_FLOAT3:
		return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	case MESHATTRIBUTE_FLOAT4:
		return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
	default:
		return SDL_GPU_VERTEXELEMENTFORMAT_INVALID;
	}
}

typedef std::vector<std::pair<MeshAttribute, std::string>> AttributeList;

inline constexpr u32 attribute_list_size(const AttributeList &list) {
	u32 result = 0;
	for (const auto &attr : list) result += mesh_attribute_sizes[attr.first];
	return result;
}

inline constexpr u32 attribute_list_offset(const AttributeList &list, u32 index) {
	u32 result = 0;
	for (u32 i = 0; i < index; i++) result += mesh_attribute_sizes[list[i].first];
	return result;
}