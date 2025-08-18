#include "AppImpl.h"
#include <thread>

#include <SDL3/SDL_timer.h>

#include "MiniLibs/lodepng.h"

struct Vertex {
	struct {
		float x;
		float y;
		float z;
	} pos;
	struct {
		float u;
		float v;
	} uv;
};

const Vertex vertex_buffer[4] = {
	{
		.pos = { // Bottom-left (0)
			.x = -1.0f,
			.y = -1.0f,
			.z =  0.0f
		},
		.uv = {
			.u = 0.0f,
			.v = 1.0f,
		}
	},
	{ // Bottom-right (1)
		.pos = {
			.x =  1.0f,
			.y = -1.0f,
			.z =  0.0f
		},
		.uv = {
			.u = 1.0f,
			.v = 1.0f,
		}
	},
	{ // Top-right (2)
		.pos = {
			.x =  1.0f,
			.y =  1.0f,
			.z =  0.0f
		},
		.uv = {
			.u = 1.0f,
			.v = 0.0f,
		}
	},
	{ // Top-left (3)
		.pos = {
			.x = -1.0f,
			.y =  1.0f,
			.z =  0.0f
		},
		.uv = {
			.u = 0.0f,
			.v = 0.0f,
		}
	},
};

const u32 index_buffer[6] = {
	0, 1, 2,
	0, 2, 3
};

const byte texture[16] = {
	0  , 0  , 0  , 255, /**/ 127, 255, 255, 255,
	255, 255, 255, 255, /**/ 127, 0  , 0  , 255,
};

void AppImpl::process_tick() {
	ActiveCopyPass acp = m_renderer.begin_copy_pass();
	if (acp.is_valid()) {
		/*acp.upload_buffer((byte *) vertex_buffer, sizeof(vertex_buffer), m_vb);
		acp.upload_buffer((byte *) index_buffer, sizeof(index_buffer), m_ib);*/

		m_mesh0.upload(acp);

		acp.upload_texture(m_texturedata0.data(), m_texturedata0.size(), m_texture0);
	}
	m_renderer.end_copy_pass(std::move(acp));

	ActiveRenderPass arp = m_renderer.begin_window_render_pass();
	if (arp.is_valid()) {
		m_this_tick_ms = SDL_GetTicks();
		
		std::cout << "Frame: " << m_frame_num << "  FPS: " << 1000.0 / (float) (m_this_tick_ms - m_last_tick_ms) << "\n";

		m_frame_num++;
		m_last_tick_ms = m_this_tick_ms;

		arp.use_shader(m_shader0);

		//arp.bind_mesh_indexed(6, m_ib, m_vb);
		m_mesh0.bind(arp);

		arp.bind_frag_samplers(0, {m_quality_sampler}, {m_texture0});

		// Upload uniforms
		float bd0f[32] = { 0 };

		// Generate projection & eye matrix, then copy to uniform buffer
		mat4x4 eye = glm::identity<mat4x4>();
		eye = glm::translate(eye, vec3(cos(m_this_tick_ms / 1000.0f), sin(m_this_tick_ms / 1000.0f), 0.0f));

		mat4x4 eyeproj = m_renderer.generate_perspective(deg_to_rad(90.0)) * glm::inverse(eye);
		memmove((byte *) (bd0f + 0), (void *) &eyeproj, sizeof(eyeproj));

		// Same for world matrix, but invert first
		mat4x4 world = glm::identity<mat4x4>();
		world = glm::translate(world, vec3(0.0f, 0.0f, -2.0f));

		world *= glm::mat4x4(glm::angleAxis(m_this_tick_ms / 1000.0f, vec3(0.0, 1.0, 0.0)));
		memmove((byte *) (bd0f + 16), (void *) &world, sizeof(world));

		arp.upload_vertex_uniform_buffer(0, (void *) bd0f, sizeof(bd0f));

		arp.draw();

		// Reupload uniforms
		world = glm::identity<mat4x4>();
		world = glm::translate(world, vec3(0.0f, -4.0f, -4.0f));
		world = glm::scale(world, vec3(10.0));
		world *= glm::mat4x4(glm::angleAxis(deg_to_rad(-90.0), vec3(1.0, 0.0, 0.0)));
		memmove((byte *) (bd0f + 16), (void *) &world, sizeof(world));

		arp.upload_vertex_uniform_buffer(0, (void *) bd0f, sizeof(bd0f));

		arp.draw();
	}
	m_renderer.end_render_pass(std::move(arp));

	std::this_thread::sleep_for(std::chrono::milliseconds(15));
}

void AppImpl::process_sdl_event(SDL_Event &event) {
	switch (event.type) {
	case SDL_EVENT_WINDOW_RESIZED: {
		m_renderer.resize_window((u32) event.window.data1, (u32) event.window.data2);
	} break;
	case SDL_EVENT_WINDOW_CLOSE_REQUESTED: {
		if (event.window.windowID == SDL_GetWindowID(m_main_window)) {
			request_close();
		}
	} break;
	}

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

	// I'm not even going to try to deal with the bugginess. "Placement new, go!"
	new (&m_renderer) Renderer(m_main_window);

	ShaderStageInfo vert_stage = {
			.path = "shader0.vert.spv",
			.num_samplers = 0,
			.num_storage_textures = 0,
			.num_storage_buffers = 0,
			.num_uniform_buffers = 1,
	};

	ShaderStageInfo frag_stage = {
		.path = "shader0.frag.spv",
		.num_samplers = 1,
		.num_storage_textures = 0,
		.num_storage_buffers = 0,
		.num_uniform_buffers = 0,
	};

	PipelineInfo pip_info = {
		.depth_test = true,
		.stencil_test = false,
		.targets = {
			{
				.format = SDL_GetGPUSwapchainTextureFormat(m_renderer.get_device(), m_main_window),
				.alpha_blending = true
			}
		},
		.vert_attribs = {
			{ // Position
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.size = 4 * 3
			},
			{ // UV
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
				.size = 4 * 2
			},
		},
		.inst_attribs = {},
		.cull_mode = SDL_GPU_CULLMODE_NONE
	};

	m_shader0 = m_renderer.add_shader(vert_stage, frag_stage, std::move(pip_info));

	/*m_vb = m_renderer.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertex_buffer));
	m_ib = m_renderer.create_buffer(SDL_GPU_BUFFERUSAGE_INDEX, sizeof(index_buffer));*/

	new (&m_mesh0) Mesh(m_renderer,
		std::vector<byte>((byte *) vertex_buffer, (byte *) vertex_buffer + sizeof(vertex_buffer)),
		std::vector<u32>(index_buffer, index_buffer + LENGTHOF(index_buffer)));

	u32 td0bl;
	byte *t0db= read_whole_file("texture0.png", &td0bl);

	std::vector<byte> t0dbv(t0db, t0db + td0bl);
	
	unsigned int w, h;
	lodepng::decode(m_texturedata0, w, h, t0dbv, LCT_RGBA, 8);

	SDL_GPUTextureCreateInfo t0ci = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = w,
		.height = h,
		.layer_count_or_depth = 1,
		.num_levels = 0,
	};

	m_texture0 = m_renderer.create_texture(&t0ci);

	m_quality_sampler = m_renderer.create_sampler(true, false, 4.0f);
	m_precise_sampler = m_renderer.create_sampler(false, true);
}

void AppImpl::any_close() {
	m_mesh0.destroy();

	m_renderer.clean_resources(RendererCleanupExclude::NONE);

	SDL_DestroyWindow(m_main_window);
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
