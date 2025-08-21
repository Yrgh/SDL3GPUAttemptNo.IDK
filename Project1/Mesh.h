#pragma once

#include "common.h"
#include "Renderer.h"
#include "MeshAttributes.h"

#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_EXCEPTION
#include "MiniLibs/tiny_gltf.h"

namespace tg = tinygltf;
using tg::TinyGLTF;

class Mesh {
	enum DirtyState {
		DIRTY_NONE = 0,
		DIRTY_VERTICES = 1,
		DIRTY_INDICES = 2,
		DIRTY_INSTANCES = 4
	};

	Renderer *m_renderer = nullptr;

	RID m_vertbuf = U32_BAD, m_indexbuf = U32_BAD, m_instbuf = U32_BAD;

	std::vector<byte> m_vertices = {}, m_instances = {};
	std::vector<u32> m_indices = {};

	u32 m_dirtiness = DIRTY_NONE;

	static Mesh parse_tg_model(Renderer &renderer, const tg::Model &model, const AttributeList &attributes);

public:
	static Mesh load_glb_memory(Renderer &renderer, const byte *data, u32 length, const AttributeList &attributes);
	static Mesh load_gltf_string(Renderer &renderer, const std::string &data, const AttributeList &attributes);

	// This does nothing and initializes nothing. NEVER use instances initialized this way.
	inline Mesh() {}

	/// <summary>
	/// Creates a new mesh from prexisting data
	/// </summary>
	/// <param name="renderer">- The Renderer to create the mesh on</param>
	/// <param name="vertices">- The initial per-vertex data</param>
	/// <param name="indices">- The index data. If this is empty, the mesh will not be indexed</param>
	/// <param name="instances">- (Optional) The initial per-instance data. If this is empty, the mesh will not support instancing</param>
	Mesh(Renderer &renderer, std::vector<byte> vertices, std::vector<u32> indices, std::vector<byte> instances = {});

	Mesh(const Mesh &) = delete;
	Mesh &operator=(const Mesh &) = delete;

	Mesh(Mesh &&) = default;
	Mesh &operator=(Mesh &&) = default;

	inline ~Mesh() { destroy(); }

	void destroy();

	void update_vertices(const std::vector<byte> &vertices);
	void update_instances(const std::vector<byte> &instances);
	
	// No support for updating indices

	void upload(ActiveCopyPass &acp);
	void bind(ActiveRenderPass &arp);
};