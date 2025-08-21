#pragma once
#include "common.h"
#include "MeshAttributes.h"

struct ShaderStageInfo {
	std::string path;

	u32 num_samplers;
	u32 num_storage_textures;
	u32 num_storage_buffers;
	u32 num_uniform_buffers;
};

struct ColorTargetInfo {
	SDL_GPUTextureFormat format;
	bool alpha_blending;
};

struct PipelineInfo {
	// If true, enables depth testing and the depth color target
	bool depth_test;
	bool stencil_test;

	std::vector<ColorTargetInfo> targets;

	AttributeList vert_attribs;
	AttributeList inst_attribs;

	SDL_GPUCullMode cull_mode;
};

struct CompiledPipelineInfo {
	// -1 means not available
	u32 vert_slot_offset;
	// -1 means not available
	u32 inst_slot_offset;

	std::vector<SDL_GPUTextureFormat> target_formats;
};

class VisualShader {
	SDL_GPUShader *m_vs;
	SDL_GPUShader *m_fs;

	SDL_GPUGraphicsPipeline *m_rp;

	SDL_GPUDevice *m_device;

	CompiledPipelineInfo m_pipinfo;

	friend class Renderer;
public:
	VisualShader() = default;

	// Create a brand-new pipeline from a pair of SPIR-V shader files
	VisualShader(ShaderStageInfo vs, ShaderStageInfo fs, PipelineInfo &&pip, SDL_GPUDevice *device);

	// Duplication is unsupported, use moves, alternate storage methods, or references.
	VisualShader(const VisualShader &) = delete;
	VisualShader &operator=(const VisualShader &) = delete;

	VisualShader(VisualShader &&) noexcept = default;
	VisualShader &operator=(VisualShader &&) noexcept = default;

	~VisualShader();

	const CompiledPipelineInfo &get_info() const;
};