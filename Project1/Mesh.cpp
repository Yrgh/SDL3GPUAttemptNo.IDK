#include "Mesh.h"

Mesh Mesh::parse_tg_model(Renderer &renderer, const tg::Model &model, const AttributeList &attributes) {
	u32 attrib_stride = attribute_list_size(attributes);

	std::vector<byte> data;
	std::vector<u32> indices;

	if (model.meshes.size() == 1 && model.meshes[0].primitives.size() == 1) {
		const tg::Primitive &prim = model.meshes[0].primitives[0];

		std::cout << "Mesh has attributes:\n";
		for (const auto &attr : prim.attributes) {
			std::cout << "\t" << attr.first << "\n";
		}

		for (u32 i = 0; i < attributes.size(); ++i) {
			if (!prim.attributes.contains(attributes[i].second)) continue;

			const tg::Accessor &acc = model.accessors[prim.attributes.at(attributes[i].second)];

			if (i == 0) {
				data.resize(acc.count * attrib_stride);
			}

			const tg::BufferView &view = model.bufferViews[acc.bufferView];
			const tg::Buffer &buf = model.buffers[view.buffer];

			u32 type_size;
			switch (acc.componentType) {
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				type_size = 1;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				type_size = 2;
				break;
			case TINYGLTF_COMPONENT_TYPE_INT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				type_size = 4;
				break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
				type_size = 8;
				break;
			default:
				type_size = 0;
			}

			switch (acc.type) {
			case TINYGLTF_TYPE_VEC2:
				type_size *= 2;
				break;
			case TINYGLTF_TYPE_VEC3:
				type_size *= 3;
				break;
			case TINYGLTF_TYPE_VEC4:
			case TINYGLTF_TYPE_MAT2:
				type_size *= 4;
				break;
			case TINYGLTF_TYPE_MAT3:
				type_size *= 9;
				break;
			case TINYGLTF_TYPE_MAT4:
				type_size *= 16;
				break;
			case TINYGLTF_TYPE_SCALAR:
			default:
				break;
			}

			u32 stride = view.byteStride > 0 ? view.byteStride : type_size;

			u32 src_pos = view.byteOffset + acc.byteOffset;
			u32 dst_pos = attribute_list_offset(attributes, i);
			for (u32 j = 0; j < acc.count; ++j) {
				memmove(data.data() + dst_pos, buf.data.data() + src_pos, type_size);
				src_pos += stride;
				dst_pos += attrib_stride;
			}
		}

		const tg::Accessor &acc = model.accessors[prim.indices];
		const tg::BufferView &view = model.bufferViews[acc.bufferView];
		const tg::Buffer &buf = model.buffers[view.buffer];

		indices.reserve(acc.count);

		u32 type_size;
		switch (acc.componentType) {
		case TINYGLTF_COMPONENT_TYPE_BYTE:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			type_size = 1;
			break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			type_size = 2;
			break;
		case TINYGLTF_COMPONENT_TYPE_INT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			type_size = 4;
			break;
		default:
			type_size = 0;
		}

		u32 stride = view.byteStride > 0 ? view.byteStride : type_size;

		u32 src_pos = view.byteOffset + acc.byteOffset;
		for (u32 j = 0; j < acc.count; ++j) {
			union {
				uint8_t u8t;
				int8_t i8t;
				uint16_t u16t;
				int16_t i16t;
				uint32_t u32t;
				int32_t i32t;
			} item;

			switch (acc.componentType) {
			case TINYGLTF_COMPONENT_TYPE_BYTE:
				memmove(&item.i8t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.i8t);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				memmove(&item.u8t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.u8t);
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
				memmove(&item.i16t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.i16t);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				memmove(&item.u16t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.u16t);
				break;
			case TINYGLTF_COMPONENT_TYPE_INT:
				memmove(&item.i32t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.i32t);
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				memmove(&item.u32t, buf.data.data() + src_pos, type_size);
				indices.push_back(item.u32t);
				break;
			default:
				break;
			}

			src_pos += stride;
		}
	} else {
		std::cout << "Model has " << model.meshes.size() << " meshes and " << model.meshes[0].primitives.size() << " primitives in the first mesh\n";
	}

	return Mesh(renderer, std::move(data), std::move(indices));
}

Mesh Mesh::load_glb_memory(Renderer &renderer, const byte *data, u32 length, const AttributeList &attributes) {
	TinyGLTF loader;
	tg::Model model;
	std::string message;

	if (!loader.LoadBinaryFromMemory(&model, &message, &message, data, length)) {
		std::cout << "GLB Load Error: " << message << "\n";
	} else if (!message.empty()) {
		std::cout << "GLB Load Warning: " << message << "\n";
	}

	return parse_tg_model(renderer, model, attributes);
}

Mesh Mesh::load_gltf_string(Renderer &renderer, const std::string &data, const AttributeList &attributes) {
	TinyGLTF loader;
	tg::Model model;
	std::string message;

	loader.LoadASCIIFromString(&model, &message, &message, data.c_str(), data.length(), "");

	return parse_tg_model(renderer, model, attributes);
}

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
