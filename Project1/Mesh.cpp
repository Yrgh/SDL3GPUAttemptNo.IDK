#include "Mesh.h"

Mesh::Mesh(Renderer &renderer, std::vector<byte> vertices, std::vector<u32> indices, std::vector<byte> instances):
	m_renderer(&renderer), m_vertices(vertices), m_indices(indices), m_instances(instances), m_dirtiness(DIRTY_VERTICES)
{
	m_vertbuf = renderer.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, vertices.size());

	if (indices.size() > 0) {
		m_indexbuf = renderer.create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, indices.size() * sizeof(u32));
		m_dirtiness |= DIRTY_INDICES;
	}

	if (instances.size() > 0) {
		m_instbuf = renderer.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, instances.size());
		m_dirtiness |= DIRTY_INSTANCES;
	}

}

void Mesh::destroy() {
	if (!m_renderer) return;

	m_renderer->destroy_buffer(m_vertbuf);

	if (*m_indexbuf != U32_BAD) {
		m_renderer->destroy_buffer(m_indexbuf);
	}

	if (*m_instbuf != U32_BAD) {
		m_renderer->destroy_buffer(m_instbuf);
	}

	m_vertbuf = U32_BAD;
	m_indexbuf = U32_BAD;
	m_instbuf = U32_BAD;

	m_renderer = nullptr;
}

void Mesh::update_vertices(const std::vector<byte> &vertices) {
	if (vertices.size() != m_vertices.size()) {
		m_renderer->resize_buffer(m_vertbuf, vertices.size());
	}

	m_vertices = vertices;
	m_dirtiness |= DIRTY_VERTICES;
}

void Mesh::update_instances(const std::vector<byte> &instances) {
	if (*m_instbuf == U32_BAD) return; // BAD: Instances aren't supported for this mesh

	if (instances.size() != m_instances.size()) {
		m_renderer->resize_buffer(m_instbuf, instances.size());
	}

	m_instances = instances;
	m_dirtiness |= DIRTY_INSTANCES;
}

void Mesh::upload(ActiveCopyPass &acp) {
	if (m_dirtiness & DIRTY_VERTICES) acp.upload_buffer(m_vertices.data(), m_vertices.size(), m_vertbuf);
	if (m_dirtiness & DIRTY_INDICES && *m_indexbuf != U32_BAD) acp.upload_buffer((byte *) m_indices.data(), m_indices.size() * sizeof(u32), m_indexbuf);
	if (m_dirtiness & DIRTY_INSTANCES && *m_instbuf != U32_BAD) acp.upload_buffer(m_instances.data(), m_instances.size(), m_instbuf);

	m_dirtiness = DIRTY_NONE;
}

void Mesh::bind(ActiveRenderPass &arp) {
	if (*m_indexbuf != U32_BAD) {
		arp.bind_mesh_indexed(m_indices.size(), m_indexbuf, m_vertbuf, m_instbuf);
	} else {
		arp.bind_mesh(m_vertices.size(), m_vertbuf, m_instbuf);
	}
}
