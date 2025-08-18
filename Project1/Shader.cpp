#include "Shader.h"

VisualShader::VisualShader(ShaderStageInfo vs, ShaderStageInfo fs, PipelineInfo &&pip, SDL_GPUDevice *device):
	s_device(device)
{
	// Create vertex shader
	u32 shader_len;
	std::cout << "Reading vs\n";
	byte *vs_shader_code = read_whole_file(vs.path, &shader_len);

	if (!vs_shader_code || shader_len < 1) {
		return;
	}

	SDL_GPUShaderCreateInfo sci = {
		.code_size = shader_len,
		.code = vs_shader_code,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_samplers = vs.num_samplers,
		.num_storage_textures = vs.num_storage_textures,
		.num_storage_buffers = vs.num_storage_buffers,
		.num_uniform_buffers = vs.num_uniform_buffers,
	};

	m_vs = SDL_CreateGPUShader(s_device, &sci);
	std::cout << "Deleting vs\n";
	delete[] vs_shader_code;

	// Create fragment shader
	std::cout << "Reading vs\n";
	byte *fs_shader_code = read_whole_file(fs.path, &shader_len);

	if (!fs_shader_code || shader_len < 1) {
		return;
	}

	sci = {
		.code_size = shader_len,
		.code = fs_shader_code,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = fs.num_samplers,
		.num_storage_textures = fs.num_storage_textures,
		.num_storage_buffers = fs.num_storage_buffers,
		.num_uniform_buffers = fs.num_uniform_buffers,
	};

	m_fs = SDL_CreateGPUShader(s_device, &sci);
	std::cout << "Deleting fs\n";
	delete[] fs_shader_code;

	// Create pipeline
	SDL_GPUGraphicsPipelineCreateInfo gpci = {
		.vertex_shader = m_vs,
		.fragment_shader = m_fs,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	};

	// Get the size of each attribute and load it into SDL format
	u32 vert_slot = 0;
	u32 inst_slot = 1 - (u32) (pip.vert_attribs.empty());

	u32 total_attribs = pip.vert_attribs.size() + pip.inst_attribs.size();
	std::cout << "Allocating vertex attributes\n";
	SDL_GPUVertexAttribute *vas = new SDL_GPUVertexAttribute[total_attribs];

	u32 vert_attribs_step = 0;
	for (u32 i = 0; i < pip.vert_attribs.size(); ++i) {
		vas[i] = {
			.location = i,
			.buffer_slot = vert_slot,
			.format = pip.vert_attribs[i].format,
			.offset = vert_attribs_step
		};

		vert_attribs_step += pip.vert_attribs[i].size;
	}

	u32 inst_attribs_step = 0;
	for (u32 j = 0; j < pip.inst_attribs.size(); ++j) {
		u32 i = j + pip.vert_attribs.size();
		vas[j] = {
			.location = j,
			.buffer_slot = inst_slot,
			.format = pip.inst_attribs[i].format,
			.offset = inst_attribs_step
		};

		inst_attribs_step += pip.inst_attribs[i].size;
	}

	u32 buffer_desc_count = 0;
	if (pip.vert_attribs.size() > 0) {
		buffer_desc_count++;
	}

	if (pip.inst_attribs.size() > 0) {
		buffer_desc_count++;
	}

	// Load buffer descriptions to SDL format
	std::cout << "Allocating vertex buffer descriptions\n";
	SDL_GPUVertexBufferDescription *vbds = new SDL_GPUVertexBufferDescription[buffer_desc_count];
	if (pip.vert_attribs.size() > 0) {
		vbds[vert_slot] = {
			.slot = vert_slot,
			.pitch = vert_attribs_step,
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		};
	}

	if (pip.inst_attribs.size() > 0) {
		vbds[inst_slot] = {
			.slot = inst_slot,
			.pitch = inst_attribs_step,
			.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE,
		};
	}

	// Set up the pipeline
	gpci.vertex_input_state = {
		.vertex_buffer_descriptions = vbds,
		.num_vertex_buffers = buffer_desc_count,
		.vertex_attributes = vas,
		.num_vertex_attributes = total_attribs,
	};

	gpci.rasterizer_state = {
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = pip.cull_mode,
		.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
		.depth_bias_constant_factor = 0.0f,
		.depth_bias_clamp = 0.0f,
		.depth_bias_slope_factor = 0.0f,
		.enable_depth_bias = false,
		.enable_depth_clip = false
	};

	gpci.multisample_state = {
		.sample_count = SDL_GPU_SAMPLECOUNT_1
	};

	gpci.depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_LESS,
		.back_stencil_state = SDL_GPU_STENCILOP_REPLACE,
		.front_stencil_state = SDL_GPU_STENCILOP_REPLACE,
		.compare_mask = 0xFF,
		.write_mask = 0xFF,
		.enable_depth_test = pip.depth_test,
		.enable_depth_write = pip.depth_test,
		.enable_stencil_test = pip.stencil_test
	};

	std::cout << "Allocating color target descriptions\n";
	SDL_GPUColorTargetDescription *ctds = new SDL_GPUColorTargetDescription[pip.targets.size()];
	for (u32 i = 0; i < pip.targets.size(); ++i) {
		ctds[i] = {
			.format = pip.targets[0].format,
			.blend_state = {
				.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.color_blend_op = SDL_GPU_BLENDOP_ADD,
				.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
				.enable_blend = pip.targets[0].alpha_blending,
				.enable_color_write_mask = false,
			}
		};
	}

	gpci.target_info = {
		.color_target_descriptions = ctds,
		.num_color_targets = (u32) pip.targets.size(),
		.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		.has_depth_stencil_target = true
	};

	m_rp = SDL_CreateGPUGraphicsPipeline(s_device, &gpci);

	// Save the info for later
	if (pip.vert_attribs.size() > 0)
		m_pipinfo.vert_slot_offset = 0;
	else
		m_pipinfo.vert_slot_offset = U32_BAD;

	if (pip.inst_attribs.size() > 0)
		m_pipinfo.inst_slot_offset = vert_attribs_step;
	else
		m_pipinfo.inst_slot_offset = U32_BAD;
	
	m_pipinfo.target_formats.resize(pip.targets.size());
	for (u32 i = 0; i < pip.targets.size(); ++i) {
		m_pipinfo.target_formats[i] = pip.targets[i].format;
	}

	std::cout << "Deallocating VAs, VBDs, and CTDs\n";
	delete[] vas;
	delete[] vbds;
	delete[] ctds;
}

VisualShader::~VisualShader() {
	if (m_rp) {
		SDL_ReleaseGPUGraphicsPipeline(s_device, m_rp);
	}

	if (m_fs) {
		SDL_ReleaseGPUShader(s_device, m_fs);
	}

	if (m_vs) {
		SDL_ReleaseGPUShader(s_device, m_vs);
	}
}

const CompiledPipelineInfo &VisualShader::get_info() const {
	return m_pipinfo;
}
