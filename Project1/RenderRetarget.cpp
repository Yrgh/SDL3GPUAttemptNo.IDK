#include "RenderRetarget.h"

RenderRetarget::RenderRetarget(Renderer &renderer, SDL_Window *window) :
	m_renderer(&renderer), m_targ_window(window)
{
	SDL_ClaimWindowForGPUDevice(m_renderer->m_device, m_targ_window);
}

RenderRetarget::~RenderRetarget() {
	release();
}

void RenderRetarget::release() {
	if (!m_renderer) return;

	for (u32 i = 0; i < m_screen_textures.size(); i++) {
		if (m_screen_textures[i])
			SDL_ReleaseGPUTexture(m_renderer->m_device, m_screen_textures[i]);
	}

	SDL_ReleaseWindowFromGPUDevice(m_renderer->m_device, m_targ_window);
	m_targ_window = nullptr;
	m_renderer = nullptr;
}

mat4x4 RenderRetarget::generate_perspective(float fov_rad) {
	return glm::perspectiveFov(fov_rad, m_viewport.w, m_viewport.h, 0.01f, 4096.0f);
}

ActiveRenderPass RenderRetarget::begin_window_render_pass() {
	SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(m_renderer->m_device);

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

	auto arp = ActiveRenderPass(*m_renderer);
	arp.m_cb = cb;
	arp.m_rp = rp;

	return arp;
}

RID RenderRetarget::create_screen_texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage) {
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

	m_screen_textures.push_back(SDL_CreateGPUTexture(m_renderer->m_device, &ci));
	m_screen_tex_infos.push_back({
		.format = format,
		.usage = usage
		});

	return RID(m_screen_textures.size() - 1);
}

void RenderRetarget::destroy_screen_texture(RID texture) {
	SDL_ReleaseGPUTexture(m_renderer->m_device, m_screen_textures[*texture]);

	m_screen_textures[*texture] = nullptr;
}