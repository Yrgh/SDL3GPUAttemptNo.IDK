#pragma once

#include "Application.h"
#include "Renderer.h"
#include "Mesh.h"

#include <SDL3/SDL_gpu.h>

class AppImpl : public Application {
	SDL_Window *m_main_window;

	RID m_vb;
	RID m_ib;

	Mesh m_mesh0;
	Mesh m_mesh1;

	RID m_texture0;
	
	RID m_quality_sampler;
	RID m_precise_sampler;

	Renderer m_renderer;

	AttributeList m_mesh_attributes;

	RID m_shader0;

	u32 m_frame_num = 0;
	u32 m_last_tick_ms = 0;
	u32 m_this_tick_ms = 0;

	std::vector<byte> m_texturedata0;

	void any_close();

protected:
	void process_tick() override;
	void process_sdl_event(SDL_Event &event) override;

	void init() override;
public:
	void _on_fail() override;
	void _on_success() override;

};

