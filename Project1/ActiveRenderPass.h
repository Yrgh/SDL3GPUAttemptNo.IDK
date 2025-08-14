#pragma once
#include "common.h"

class Renderer;

class ActiveRenderPass {
	Renderer *m_renderer;

	RID m_active_shader = u32(-1);

	SDL_GPUCommandBuffer *m_cb;
	SDL_GPURenderPass *m_rp;

	friend class Renderer;

	ActiveRenderPass(Renderer &);
public:
	ActiveRenderPass();
	ActiveRenderPass(const ActiveRenderPass &) = delete;
	ActiveRenderPass &operator=(const ActiveRenderPass &) = delete;

	ActiveRenderPass(ActiveRenderPass &&) noexcept = default;
	ActiveRenderPass &operator=(ActiveRenderPass &&) noexcept = default;

	void use_shader(RID shader);

	void upload_vertex_uniform_buffer(u32 slot, const void *data, u32 length);

	// Omit the second argument if
	// - Only per-vertex attributes exist
	// - Only per-instance attributes exist
	// - The same buffer is used
	void bind_vertex_buffers(SDL_GPUBuffer *buf1, SDL_GPUBuffer *buf2 = nullptr);

	void draw(u32 num_vertices, u32 num_instances);

	inline bool is_valid() const { return m_renderer; }
};

