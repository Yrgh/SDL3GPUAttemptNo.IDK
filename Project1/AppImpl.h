#pragma once

#include "Application.h"
#include "Renderer.h"

#include <SDL3/SDL_gpu.h>

class AppImpl : public Application {
	SDL_Window *m_main_window;

	RID m_vb;
	RID m_ib;

	RID m_texture0;
	
	RID m_linear_wrap_sampler;

	Renderer m_renderer;

	RID m_shader0;

	size_t frame_num = 0;
	u32 last_tick_ms = 0;
	u32 this_tick_ms = 0;

	void any_close();

protected:
	void process_tick() override;
	void process_sdl_event(SDL_Event &event) override;

	void init() override;
public:
	void _on_fail() override;
	void _on_success() override;

};

