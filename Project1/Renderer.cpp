#include "Renderer.h"

u32 Renderer::get_unused_buffer() {
	for (u32 i = 0; i < m_buffers.size(); ++i) {
		if (!m_buffers[i]) {
			return i;
		}
	}
	return U32_BAD;
}

u32 Renderer::get_unused_texture() {
	for (u32 i = 0; i < m_user_textures.size(); ++i) {
		if (!m_user_textures[i]) {
			return i;
		}
	}
	return U32_BAD;
}

Renderer::Renderer(SDL_Window *window):
	m_targ_window(window)
{
	m_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

	SDL_GetWindowSizeInPixels(window, &m_winw, &m_winh);

	SDL_ClaimWindowForGPUDevice(m_device, window);

	create_screen_texture(
		SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET);

	m_viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = (float) m_winw,
		.h = (float) m_winh,
		.min_depth = 0.0f,
		.max_depth = 1.0f
	};

	SDL_GPUTransferBufferCreateInfo utbci = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = TRANSFER_BUFFER_SIZE
	};

	m_upload_buffer = SDL_CreateGPUTransferBuffer(m_device, &utbci);

	SDL_GPUTransferBufferCreateInfo dtbci = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
		.size = TRANSFER_BUFFER_SIZE
	};

	m_download_buffer = SDL_CreateGPUTransferBuffer(m_device, &dtbci);
}

Renderer::~Renderer() {
	clean_resources(RendererCleanupExclude::NONE);
}

void Renderer::clean_resources(RendererCleanupExclude exclude) {
	if (!m_device) return;

	bool exclude_internals = true;
	bool exclude_shaders = true;
	switch (exclude) {
	case RendererCleanupExclude::NONE:
		exclude_internals = false;
	case RendererCleanupExclude::INTERNALS:
		exclude_shaders = false;
	case RendererCleanupExclude::DEFAULT:
		break;
	}
	
	for (u32 i = 0; i < m_buffers.size(); ++i) {
		if (m_buffers[i]) {
			SDL_ReleaseGPUBuffer(m_device, m_buffers[i]);
		}
	}
	m_buffers.clear();
	m_buffer_infos.clear();

	// Unless we don't exclude internals, exclude the first screen texture (the depth texture)
	if (exclude_internals) {
		for (u32 i = 1; i < m_screen_textures.size(); ++i) {
			if (m_screen_textures[i]) {
				SDL_ReleaseGPUTexture(m_device, m_screen_textures[i]);
			}
		}
		m_screen_textures.resize(1);
		m_screen_tex_infos.resize(1);
	} else {
		for (u32 i = 0; i < m_screen_textures.size(); ++i) {
			if (m_screen_textures[i]) {
				SDL_ReleaseGPUTexture(m_device, m_screen_textures[i]);
			}
		}
		m_screen_textures.clear();
		m_screen_tex_infos.clear();
	}


	for (u32 i = 0; i < m_user_textures.size(); ++i) {
		if (m_user_textures[i]) {
			SDL_ReleaseGPUTexture(m_device, m_user_textures[i]);
		}
	}
	m_user_textures.clear();
	m_texture_states.clear();

	for (u32 i = 0; i < m_samplers.size(); ++i) {
		if (m_samplers[i]) {
			SDL_ReleaseGPUSampler(m_device, m_samplers[i]);
		}
	}
	m_samplers.clear();

	if (!exclude_shaders) {
		m_shaders.clear();
	}

	if (!exclude_internals) {
		SDL_ReleaseGPUTransferBuffer(m_device, m_upload_buffer);
		m_upload_buffer = nullptr;
		SDL_ReleaseGPUTransferBuffer(m_device, m_download_buffer);
		m_download_buffer = nullptr;

		SDL_ReleaseWindowFromGPUDevice(m_device, m_targ_window);
		m_targ_window = nullptr;

		SDL_DestroyGPUDevice(m_device);
		m_device = nullptr;
	}
}

RID Renderer::add_shader(ShaderStageInfo vs, ShaderStageInfo fs, PipelineInfo &&pip) {
	m_shaders.emplace_back(vs, fs, std::move(pip), m_device);
	return RID(m_shaders.size() - 1);
}

void Renderer::resize_window(u32 new_w, u32 new_h) {
	m_winw = new_w;
	m_winh = new_h;

	m_viewport.w = (float) new_w;
	m_viewport.h = (float) new_h;

	for (u32 i = 0; i < m_screen_textures.size(); ++i) {
		if (!m_screen_textures[i]) continue;

		SDL_ReleaseGPUTexture(m_device, m_screen_textures[i]);

		SDL_GPUTextureCreateInfo ci = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = m_screen_tex_infos[i].format,
			.usage = m_screen_tex_infos[i].usage,
			.width = (u32) m_winw,
			.height = (u32) m_winh,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1
		};

		m_screen_textures[i] = SDL_CreateGPUTexture(m_device, &ci);
	}
}

ActiveCopyPass Renderer::begin_copy_pass() {
	SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(m_device);

	SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);

	auto acp = ActiveCopyPass(*this);
	acp.m_cb = cb;
	acp.m_cp = cp;
	acp.m_download = m_download_buffer;
	acp.m_upload = m_upload_buffer;

	return acp;
}

void Renderer::end_copy_pass(ActiveCopyPass acp) {
	if (!acp.is_valid()) { return; }
	acp.m_renderer = nullptr;

	SDL_EndGPUCopyPass(acp.m_cp);

	// Do non-pass processing
	for (u32 i = 0; i < m_user_textures.size(); ++i) {
		if (m_user_textures[i] && m_texture_states[i].dirty_mip) {
			SDL_GenerateMipmapsForGPUTexture(acp.m_cb, m_user_textures[i]);
		}
	}

	SDL_SubmitGPUCommandBuffer(acp.m_cb);
}

ActiveRenderPass Renderer::begin_window_render_pass() {
	SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(m_device);

	SDL_GPUTexture *color_target_tex = NULL;

	if (!SDL_AcquireGPUSwapchainTexture(cb, m_targ_window, &color_target_tex, NULL, NULL)) {
		SDL_CancelGPUCommandBuffer(cb);
		return ActiveRenderPass();
	}

	// NOTE: Use SDL_SubmitGPUCommandBuffer from here ======

	// Not a bad thing. Just means the window cannot display anything more at this point
	if (!color_target_tex) {
		SDL_SubmitGPUCommandBuffer(cb);
		return ActiveRenderPass();
	}

	SDL_GPUColorTargetInfo ctis[1] = {
		{
			.texture = color_target_tex,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {0.0f, 0.0f, 0.0f, 0.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true
		}
	};

	SDL_GPUDepthStencilTargetInfo dsti = {
		.texture = m_screen_textures[0],
		.clear_depth = 1.0,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
		.cycle = true,
		.clear_stencil = 0,
	};


	SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cb, ctis, 1, &dsti);

	SDL_SetGPUViewport(rp, &m_viewport);

	auto arp = ActiveRenderPass(*this);
	arp.m_cb = cb;
	arp.m_rp = rp;

	return arp;
}

ActiveRenderPass Renderer::begin_custom_render_pass(CustomInfo description) {
	SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(m_device);

	SDL_GPUColorTargetInfo *ctis = new SDL_GPUColorTargetInfo[description.color_targets.size()];
	for (u32 i = 0; i < description.color_targets.size(); ++i) {
		ctis[i] = {
			.texture = description.color_targets[i].texture,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = description.color_targets[i].clear_color,
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true
		};
	}

	SDL_GPUDepthStencilTargetInfo dsti;
	SDL_GPUDepthStencilTargetInfo *dstip = nullptr;
	if (description.depth_enabled) {
		dsti = {
		   .texture = description.depth_texture,
		   .clear_depth = 1.0,
		   .load_op = SDL_GPU_LOADOP_CLEAR,
		   .store_op = SDL_GPU_STOREOP_STORE,
		   .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		   .stencil_store_op = SDL_GPU_STOREOP_STORE,
		   .cycle = true,
		   .clear_stencil = 0,
		};

		dstip = &dsti;
	}

	SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cb, ctis, description.color_targets.size(), dstip);

	SDL_SetGPUViewport(rp, &description.viewport);

	auto arp = ActiveRenderPass(*this);
	arp.m_cb = cb;
	arp.m_rp = rp;

	return arp;
}

void Renderer::end_render_pass(ActiveRenderPass arp) {
	if (!arp.is_valid()) { return; }
	arp.m_renderer = nullptr;

	SDL_EndGPURenderPass(arp.m_rp);

	SDL_SubmitGPUCommandBuffer(arp.m_cb);
}

mat4x4 Renderer::generate_perspective(float fov_rad) {
	return glm::perspectiveFov(fov_rad, m_viewport.w, m_viewport.h, 0.01f, 4096.0f);
}

RID Renderer::create_buffer(SDL_GPUBufferUsageFlags usage, u32 size) {
	SDL_GPUBufferCreateInfo ci = {
		.usage = usage,
		.size = size
	};

	u32 location = get_unused_buffer();
	
	if (location == U32_BAD) {
		m_buffers.push_back(SDL_CreateGPUBuffer(m_device, &ci));
		m_buffer_infos.push_back(ci);

		return RID(m_buffers.size() - 1);
	} else {
		m_buffers[location] = SDL_CreateGPUBuffer(m_device, &ci);
		m_buffer_infos[location] = ci;

		return RID(location);
	}
}

void Renderer::resize_buffer(RID buffer, u32 size) {
	SDL_ReleaseGPUBuffer(m_device, m_buffers[*buffer]);

	SDL_GPUBufferCreateInfo &ci = m_buffer_infos[*buffer];
	ci.size = size;

	m_buffers[*buffer] = SDL_CreateGPUBuffer(m_device, &ci);
}

void Renderer::destroy_buffer(RID buffer) {
	SDL_ReleaseGPUBuffer(m_device, m_buffers[*buffer]);
	m_buffers[*buffer] = nullptr;
}

bool Renderer::is_buffer_valid(RID buffer) {
	if (*buffer < 0 || *buffer > m_buffers.size()) return false;
	return m_buffers[*buffer];
}

RID Renderer::create_screen_texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage) {
	SDL_GPUTextureCreateInfo ci = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = format,
		.usage = usage,
		.width = (u32) m_winw,
		.height = (u32) m_winh,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1
	};

	m_screen_textures.push_back(SDL_CreateGPUTexture(m_device, &ci));
	m_screen_tex_infos.push_back({
		.format = format,
		.usage = usage
	});

	return RID(m_screen_textures.size() - 1);
}

void Renderer::destroy_screen_texture(RID texture) {
	SDL_ReleaseGPUTexture(m_device, m_screen_textures[*texture]);

	m_screen_textures[*texture] = nullptr;
}

RID Renderer::create_texture(const SDL_GPUTextureCreateInfo *info) {
	u32 location = get_unused_texture();

	SDL_GPUTextureCreateInfo mut_info = *info;

	// Get Po2 of the largest dimension
	if (mut_info.num_levels == 0) {
		mut_info.num_levels = 0;
		u32 dim = mut_info.width > mut_info.height ? mut_info.width : mut_info.height;

		if (mut_info.type == SDL_GPU_TEXTURETYPE_3D && mut_info.layer_count_or_depth > dim) {
			dim = mut_info.layer_count_or_depth;
		}

		// Divide by two until we get to 1, or we hit the max
		while ((dim >>= 1) > 0) mut_info.num_levels++;
	} else {
		mut_info.num_levels = mut_info.num_levels;
	}

	std::cout << mut_info.width << "x" << mut_info.height << ": Mip levels: " << mut_info.num_levels << "\n";

	if (location == U32_BAD) {
		m_user_textures.push_back(SDL_CreateGPUTexture(m_device, &mut_info));
		m_texture_states.push_back(TextureState{
			.mip_levels = mut_info.num_levels,
			.dirty_mip = mut_info.num_levels > 1,
			.format = mut_info.format,
			.type = mut_info.type,
			.usage = mut_info.usage,
			.width = mut_info.width,
			.height = mut_info.height,
			.depth = mut_info.layer_count_or_depth
		});
		return RID(m_user_textures.size() - 1);
	} else {
		m_user_textures[location] = SDL_CreateGPUTexture(m_device, &mut_info);
		m_texture_states[location] = TextureState{
			.mip_levels = mut_info.num_levels,
			.dirty_mip = mut_info.num_levels > 1,
			.format = mut_info.format,
			.type = mut_info.type,
			.usage = mut_info.usage,
			.width = mut_info.width,
			.height = mut_info.height,
			.depth = mut_info.layer_count_or_depth
		};

		return RID(location);
	}
}

void Renderer::destroy_texture(RID texture) {
	SDL_ReleaseGPUTexture(m_device, m_user_textures[*texture]);
	m_user_textures[*texture] = nullptr;
}

bool Renderer::is_texture_valid(RID texture) {
	if (*texture < 0 || *texture > m_user_textures.size()) return false;
	return m_user_textures[*texture];
}

RID Renderer::create_sampler(bool linear_sample, bool clamp_uv, float anisotropy) {
	SDL_GPUSamplerCreateInfo ci = {
		.min_filter = linear_sample ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST,
		.mag_filter = linear_sample ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = linear_sample ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = clamp_uv ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_v = clamp_uv ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.address_mode_w = clamp_uv ? SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE : SDL_GPU_SAMPLERADDRESSMODE_REPEAT,
		.mip_lod_bias = 0.0f,
		.max_anisotropy = anisotropy,
		//.compare_op = SDL_GPU_COMPAREOP_ALWAYS
		.min_lod = 0.0f,
		.max_lod = FLT_MAX,
		.enable_anisotropy = anisotropy > 0.5f,
		.enable_compare = false,
	};

	m_samplers.push_back(SDL_CreateGPUSampler(m_device, &ci));
	return RID(m_samplers.size() - 1);
}
