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

void ActiveRenderPass::bind_mesh(u32 vertex_count, RID vbuf1, RID vbuf2) {
	auto &pi = m_renderer->get_shader_info(m_active_shader);

	SDL_GPUBuffer *vbuffer1 = m_renderer->get_buffer(vbuf1);
	SDL_GPUBuffer *vbuffer2 = m_renderer->get_buffer(vbuf2);

	bool per_vert_used = pi.vert_slot_offset != U32_BAD;
	bool per_inst_used = pi.inst_slot_offset != U32_BAD;
	if (per_vert_used && per_inst_used) {
		SDL_GPUBufferBinding bindings[2] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			},
			{
				.buffer = vbuffer2 ? vbuffer2 : vbuffer1,
				.offset = vbuffer2 ? 0 : pi.inst_slot_offset
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 2);
	} else if (per_vert_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	} else if (per_inst_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	}

	m_vertex_count = vertex_count;
	m_indexed = false;
}

void ActiveRenderPass::bind_mesh_indexed(u32 index_count, RID indices, RID vbuf1, RID vbuf2) {
	auto &pi = m_renderer->get_shader_info(m_active_shader);

	SDL_GPUBuffer *index_buffer = m_renderer->get_buffer(indices);
	SDL_GPUBuffer *vbuffer1 = m_renderer->get_buffer(vbuf1);
	SDL_GPUBuffer *vbuffer2 = m_renderer->get_buffer(vbuf2);

	bool per_vert_used = pi.vert_slot_offset != U32_BAD;
	bool per_inst_used = pi.inst_slot_offset != U32_BAD;
	if (per_vert_used && per_inst_used) {
		SDL_GPUBufferBinding bindings[2] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			},
			{
				.buffer = vbuffer2 ? vbuffer2 : vbuffer1,
				.offset = vbuffer2 ? 0 : pi.inst_slot_offset
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 2);
	} else if (per_vert_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	} else if (per_inst_used) {
		SDL_GPUBufferBinding bindings[1] = {
			{
				.buffer = vbuffer1,
				.offset = 0
			}
		};

		SDL_BindGPUVertexBuffers(m_rp, 0, bindings, 1);
	}

	m_vertex_count = index_count;
	m_indexed = true;

	SDL_GPUBufferBinding ibb = {
		.buffer = index_buffer,
		.offset = 0
	};

	SDL_BindGPUIndexBuffer(m_rp, &ibb, SDL_GPU_INDEXELEMENTSIZE_32BIT);
}

void ActiveRenderPass::bind_vert_samplers(u32 first_slot, const std::vector<RID> &samplers, const std::vector<RID> &textures) {
	assert(samplers.size() == textures.size());

	auto bindings = new SDL_GPUTextureSamplerBinding[samplers.size()];
	for (u32 i = 0; i < samplers.size(); i++) {
		bindings[i] = {
			.texture = m_renderer->get_texture(textures[i]),
			.sampler = m_renderer->get_sampler(samplers[i])
		};
	}

	SDL_BindGPUFragmentSamplers(m_rp, first_slot, bindings, samplers.size());
	delete[] bindings;
}

void ActiveRenderPass::bind_frag_samplers(u32 first_slot, const std::vector<RID> &samplers, const std::vector<RID> &textures) {
	assert(samplers.size() == textures.size());

	auto bindings = new SDL_GPUTextureSamplerBinding[samplers.size()];
	for (u32 i = 0; i < samplers.size(); i++) {
		bindings[i] = {
			.texture = m_renderer->get_texture(textures[i]),
			.sampler = m_renderer->get_sampler(samplers[i])
		};
	}

	SDL_BindGPUFragmentSamplers(m_rp, first_slot, bindings, samplers.size());
	delete[] bindings;
}

void ActiveRenderPass::draw(u32 num_instances) {
	if (m_indexed) {
		SDL_DrawGPUIndexedPrimitives(m_rp, m_vertex_count, num_instances, 0, 0, 0);
	} else {
		SDL_DrawGPUPrimitives(m_rp, m_vertex_count, num_instances, 0, 0);
	}
}
