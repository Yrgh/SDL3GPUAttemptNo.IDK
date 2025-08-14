#include "ActiveRenderPass.h"
#include "Renderer.h"

ActiveRenderPass::ActiveRenderPass(Renderer &renderer):
	m_renderer(&renderer)
{}

ActiveRenderPass::ActiveRenderPass():
	m_renderer(nullptr)
{}

void ActiveRenderPass::use_shader(RID shader) {
	if (!is_valid()) return;

	SDL_BindGPUGraphicsPipeline(m_rp, m_renderer->get_shader(shader));
	m_active_shader = shader;
}

void ActiveRenderPass::upload_vertex_uniform_buffer(u32 slot, const void *data, u32 length) {
	SDL_PushGPUVertexUniformData(m_cb, slot, data, length);
}

void ActiveRenderPass::bind_vertex_buffers(SDL_GPUBuffer *buf1, SDL_GPUBuffer *buf2) {
	auto &pi = m_renderer->get_shader_info(m_active_shader);

	bool per_vert_used = pi.vert_slot_offset != U32_BAD;
	bool per_inst_used = pi.inst_slot_offset != U32_BAD;
	if (per_vert_used && per_inst_used) {
		SDL_GPUBufferBinding bindings[2] = {
			{
				.buffer = buf1,
				.offset = pi.vert_slot_offset
			},
			{
				.buffer = buf2 ? buf2 : buf1,
				.offset = pi.inst_slot_offset
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 2);
	} else if (per_vert_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = buf1,
				.offset = pi.vert_slot_offset
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	} else if (per_inst_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = buf1,
				.offset = pi.inst_slot_offset
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	}
}

void ActiveRenderPass::draw(u32 num_vertices, u32 num_instances) {
	SDL_DrawGPUPrimitives(m_rp, num_vertices, num_instances, 0, 0);
}
