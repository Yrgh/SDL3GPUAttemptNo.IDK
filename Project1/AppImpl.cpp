#include "AppImpl.h"
#include "ShaderTools.h"

#include <SDL3/SDL_timer.h>

struct Vertex {
	struct {
		float x;
		float y;
		float z;
	} pos;
	struct {
		float r;
		float g;
		float b;
	} col;
};

const Vertex vertex_buffer[3] = {
	{
		.pos = {
			.x =  0.0f,
			.y = -0.5f,
			.z =  0.5f
		},
		.col = {
			.r = 1.0f,
			.g = 0.0f,
			.b = 0.0f
		}
	},
	{
		.pos = {
			.x =  0.5f,
			.y =  0.5f,
			.z =  0.5f
		},
		.col = {
			.r = 0.0f,
			.g = 1.0f,
			.b = 0.0f
		}
	},
	{
		.pos = {
			.x = -0.5f,
			.y =  0.5f,
			.z =  0.5f
		},
		.col = {
			.r = 0.0f,
			.g = 0.0f,
			.b = 1.0f
		}
	},
};

void AppImpl::process_tick() {
	SDL_GPUCommandBuffer *cb = SDL_AcquireGPUCommandBuffer(m_main_gpu_device);

	SDL_GPUTexture *color_target_tex = NULL;

	if (!SDL_AcquireGPUSwapchainTexture(cb, m_main_window, &color_target_tex, NULL, NULL)) {
		SDL_CancelGPUCommandBuffer(cb);
		fail(); return;
	}

	// NOTE: Use SDL_SubmitGPUCommandBuffer from here ======

	// Not a bad thing. Just means the window cannot display anything more at this point
	if (!color_target_tex) {
		SDL_SubmitGPUCommandBuffer(cb);
		return;
	}

	uint32_t this_tick_ms = SDL_GetTicks();
	
	std::cout << "Frame: " << frame_num << "  FPS: " << 1000.0 / (float) (this_tick_ms - last_tick_ms) << "\n";

	frame_num++;
	last_tick_ms = this_tick_ms;

	// Copy pass ------------------

	SDL_GPUCopyPass *cp = SDL_BeginGPUCopyPass(cb);

	// Send vertex data to GPU
	// - Put it on the transfer buffer
	void *tp = SDL_MapGPUTransferBuffer(m_main_gpu_device, m_vb_transfer, true);

	memmove(tp, vertex_buffer, sizeof(vertex_buffer));

	SDL_UnmapGPUTransferBuffer(m_main_gpu_device, m_vb_transfer);

	// - Upload from transfer buffer
	SDL_GPUBufferRegion vbr = {
		.buffer = m_vb,
		.offset = 0,
		.size = sizeof(vertex_buffer)
	};

	SDL_GPUTransferBufferLocation vbtl = {
		.transfer_buffer = m_vb_transfer,
		.offset = 0
	};

	SDL_UploadToGPUBuffer(cp, &vbtl, &vbr, true);

	SDL_EndGPUCopyPass(cp);

	// Render pass ------------------

	// Define color targets
	SDL_GPUColorTargetInfo ctis[1] = {
		{
			.texture = color_target_tex,
			.mip_level = 0,
			.layer_or_depth_plane = 0,
			.clear_color = {0.0f, 0.0f, 0.25f, 0.0f},
			.load_op = SDL_GPU_LOADOP_CLEAR,
			.store_op = SDL_GPU_STOREOP_STORE,
			.cycle = true
		}
	};

	SDL_GPUDepthStencilTargetInfo dsti = {
		.texture = m_depth_tex,
		.clear_depth = 1.0,
		.load_op = SDL_GPU_LOADOP_CLEAR,
		.store_op = SDL_GPU_STOREOP_STORE,
		.stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		.stencil_store_op = SDL_GPU_STOREOP_STORE,
		.cycle = true,
		.clear_stencil = 0,
	};


	SDL_GPURenderPass *rp = SDL_BeginGPURenderPass(cb, ctis, 1, &dsti);

	// Bind pipeline, viewport, etc.
	SDL_BindGPUGraphicsPipeline(rp, m_rp0);
	SDL_SetGPUViewport(rp, &m_viewport);

	SDL_PushGPUVertexUniformData(cb, 0, NULL, 0);

	// Bind vertex buffer
	SDL_GPUBufferBinding bindings[1] = {
		{
			.buffer = m_vb,
			.offset = 0
		}
	};

	SDL_BindGPUVertexBuffers(rp, 0, bindings, 1);

	// Draw it
	SDL_DrawGPUPrimitives(rp, 3, 1, 0, 0);

	// Off to the GPU we go
	SDL_EndGPURenderPass(rp);

	FATALIZE_SDL(SDL_SubmitGPUCommandBuffer(cb), { fail(); })
}

void AppImpl::process_sdl_event(SDL_Event &event) {
	switch (event.type) {
	case SDL_EVENT_WINDOW_RESIZED: {
		SDL_ReleaseGPUTexture(m_main_gpu_device, m_depth_tex);

		SDL_GPUTextureCreateInfo ci = {
			.type = SDL_GPU_TEXTURETYPE_2D,
			.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
			.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
			.width = (Uint32) event.window.data1,
			.height = (Uint32) event.window.data2,
			.layer_count_or_depth = 1,
			.num_levels = 1,
			.sample_count = SDL_GPU_SAMPLECOUNT_1
		};

		m_depth_tex = SDL_CreateGPUTexture(m_main_gpu_device, &ci);

		m_viewport.w = (float) event.window.data1;
		m_viewport.h = (float) event.window.data2;
	} break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
		if (event.window.windowID == SDL_GetWindowID(m_main_window)) {
			request_close();
		}
	} break;
	}

}

void AppImpl::create_shaders() {
	// Create vertex shader
	size_t shader_len;
	byte *shader_code = read_whole_file("shader0.vert.spv", shader_len);

	if (!shader_code || shader_len < 1) {
		std::cout << "File not read!\n";
		fail();
		return;
	}

	SDL_GPUShaderCreateInfo sci = {
		.code_size = shader_len,
		.code = shader_code,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_samplers = 0,
		.num_storage_textures = 0,
		.num_storage_buffers = 0,
		.num_uniform_buffers = 0,
	};

	m_vs0 = SDL_CreateGPUShader(m_main_gpu_device, &sci);

	std::cout << "Yay! A vertex shader!\n";

	// Create fragment shader
	shader_code = read_whole_file("shader0.frag.spv", shader_len);

	if (!shader_code || shader_len < 1) {
		std::cout << "File not read!\n";
		fail();
		return;
	}

	sci = {
		.code_size = shader_len,
		.code = shader_code,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = 0,
		.num_storage_textures = 0,
		.num_storage_buffers = 0,
		.num_uniform_buffers = 0,
	};

	m_fs0 = SDL_CreateGPUShader(m_main_gpu_device, &sci);

	std::cout << "Yay! A fragment shader!\n";
}

void AppImpl::create_render_pipeline() {
	std::cout << "Creating pipeline\n";

	SDL_GPUGraphicsPipelineCreateInfo gpci = {
		.vertex_shader = m_vs0,
		.fragment_shader = m_fs0,
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	};

	SDL_GPUVertexBufferDescription vbds[1] = {
		{
			.slot = 0,
			.pitch = 4 * (3 + 3),
			.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		}
	};

	SDL_GPUVertexAttribute vas[2] = {
		{ // Position
			.location = 0,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 0
		},
		{ // Color
			.location = 1,
			.buffer_slot = 0,
			.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
			.offset = 4 * 3
		},
	};

	gpci.vertex_input_state = {
		.vertex_buffer_descriptions = vbds,
		.num_vertex_buffers = 1,
		.vertex_attributes = vas,
		.num_vertex_attributes = 2,
	};

	gpci.rasterizer_state = {
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = SDL_GPU_CULLMODE_BACK,
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
		.enable_depth_test = true,
		.enable_depth_write = true,
		.enable_stencil_test = false
	};

	SDL_GPUColorTargetDescription ctds[1] = {
		{
			.format = SDL_GetGPUSwapchainTextureFormat(m_main_gpu_device, m_main_window),
			.blend_state = {
				.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.color_blend_op = SDL_GPU_BLENDOP_ADD,
				.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
				.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
				.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
				.enable_blend = true,
				.enable_color_write_mask = false,
			}
		}
	};

	gpci.target_info = {
		.color_target_descriptions = ctds,
		.num_color_targets = 1,
		.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		.has_depth_stencil_target = true
	};

	m_rp0 = SDL_CreateGPUGraphicsPipeline(m_main_gpu_device, &gpci);

	std::cout << "Pipeline created :D\n";
}

void AppImpl::init() {
	std::cout << "Created\n";

	int window_w = 1152;
	int window_h = 648;

	m_main_window = SDL_CreateWindow(
		"Project1 (Main window)",
		window_w, window_h,
		SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN
	);

	SDL_GetWindowSize(m_main_window, &window_w, &window_h);

	m_main_window_maxw = window_w;
	m_main_window_maxh = window_h;

	m_main_gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);

	FATALIZE_SDL(SDL_ClaimWindowForGPUDevice(m_main_gpu_device, m_main_window), fail(); return; );

	SDL_GPUTextureCreateInfo ci = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
		.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
		.width = (Uint32) window_w,
		.height = (Uint32) window_h,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1
	};

	m_depth_tex = SDL_CreateGPUTexture(m_main_gpu_device, &ci);

	m_viewport = {
		.x = 0.0f,
		.y = 0.0f,
		.w = (float) window_w,
		.h = (float) window_h,
		.min_depth = 0.01f,
		.max_depth = 1.0f
	};

	create_shaders();
	create_render_pipeline();

	SDL_GPUBufferCreateInfo vbci = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(vertex_buffer),
	};

	m_vb = SDL_CreateGPUBuffer(m_main_gpu_device, &vbci);

	SDL_GPUTransferBufferCreateInfo vbtransci = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(vertex_buffer)
	};

	m_vb_transfer = SDL_CreateGPUTransferBuffer(m_main_gpu_device, &vbtransci);
}

void AppImpl::any_close() {
	SDL_ReleaseGPUBuffer(m_main_gpu_device, m_vb);
	SDL_ReleaseGPUTransferBuffer(m_main_gpu_device, m_vb_transfer);

	SDL_ReleaseGPUGraphicsPipeline(m_main_gpu_device, m_rp0);
	SDL_ReleaseGPUShader(m_main_gpu_device, m_vs0);
	SDL_ReleaseGPUShader(m_main_gpu_device, m_fs0);

	SDL_ReleaseGPUTexture(m_main_gpu_device, m_depth_tex);

	if (m_main_window) {
		if (m_main_gpu_device) {
			SDL_ReleaseWindowFromGPUDevice(m_main_gpu_device, m_main_window);
		}

		SDL_DestroyWindow(m_main_window);
		m_main_window = NULL;
	}

	if (m_main_gpu_device) {
		SDL_DestroyGPUDevice(m_main_gpu_device);
		m_main_gpu_device = NULL;
	}
}

void AppImpl::_on_fail() {
	std::cout << "Failure registered...\n";

	any_close();

	std::cout << "Failed\n";
}

void AppImpl::_on_success() {
	std::cout << "Close registered...\n";

	any_close();

	std::cout << "Success\n";
}
