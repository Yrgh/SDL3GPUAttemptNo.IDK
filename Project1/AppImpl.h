#pragma once

#include "Application.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_gpu.h>

class AppImpl: public Application {
	SDL_Window *m_main_window;
	uint32_t m_main_window_maxw = 0;
	uint32_t m_main_window_maxh = 0;

	SDL_GPUDevice *m_main_gpu_device;

	SDL_GPUShader *m_vs0;
	SDL_GPUShader *m_fs0;

	SDL_GPUGraphicsPipeline *m_rp0;

	SDL_GPUTexture *m_depth_tex;

	SDL_GPUViewport m_viewport;

	SDL_GPUBuffer *m_vb; SDL_GPUTransferBuffer *m_vb_transfer;

	size_t frame_num = 0;
	uint32_t last_tick_ms = 0;
	uint32_t this_tick_ms = 0;

	void create_shaders();
	void create_render_pipeline();

	void draw_objects(SDL_GPUCommandBuffer *cb, SDL_GPURenderPass *rp);

	void any_close();

protected:
	void process_tick() override;
	void process_sdl_event(SDL_Event &event) override;

	void init() override;
public:
	void _on_fail() override;
	void _on_success() override;

};

