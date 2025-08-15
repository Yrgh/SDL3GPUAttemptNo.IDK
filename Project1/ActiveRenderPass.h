#pragma once
#include "common.h"

class Renderer;

class ActiveRenderPass {
	Renderer *m_renderer;

	RID m_active_shader = u32(-1);

	SDL_GPUCommandBuffer *m_cb;
	SDL_GPURenderPass *m_rp;

	u32 m_vertex_count = 0;
	bool m_indexed = false;

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

	/// <summary>
	/// Assigns mesh data to subsequent draw calls
	/// </summary>
	/// <param name="vertex_count">- The number of vertices to draw</param>
	/// <param name="vbuf1">- The primary data buffer</param>
	/// <param name="vbuf2">
	/// - (Optional) The secondary data buffer. If there are only per-vertex or per-instance attributes,
	/// don't specify this buffer. If data is stored in a single buffer, leave this as default.
	/// </param>
	void bind_mesh(u32 vertex_count, RID vbuf1, RID vbuf2 = U32_BAD);
	/// <summary>
	/// Assigns indexed mesh data to subsequent draw calls
	/// </summary>
	/// <param name="vertex_count">
	/// - The number of indexed vertices to draw. (eg. A square has 4 vertices, but two vertices are reused
	/// in the index buffer, so there are six.
	/// </param>
	/// <param name="vbuf1">- The primary data buffer</param>
	/// <param name="vbuf2">
	/// - (Optional) The secondary data buffer. If there are only per-vertex or per-instance attributes,
	/// don't specify this buffer. If data is stored in a single buffer, leave this as default.
	/// </param>
	void bind_mesh_indexed(u32 vertex_count, RID indices, RID vbuf1, RID vbuf2 = U32_BAD);

	/// <summary>
	/// Draws the previously assigned mesh data, indexed or not.
	/// </summary>
	/// <param name="num_instances">
	/// - The number of instances to draw. If there are no instance attributes, leave this at default
	/// </param>
	void draw(u32 num_instances = 1);

	inline bool is_valid() const { return m_renderer; }
};

