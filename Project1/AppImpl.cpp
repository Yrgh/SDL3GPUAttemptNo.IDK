#include "AppImpl.h"

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
			.z =  0.0f
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
			.z =  0.0f
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
			.z =  0.0f
		},
		.col = {
			.r = 0.0f,
			.g = 0.0f,
			.b = 1.0f
		}
	},
};

void AppImpl::process_tick() {

	ActiveCopyPass acp = m_renderer.begin_copy_pass();
	if (acp.is_valid()) {
		acp.upload_buffer((byte *) vertex_buffer, sizeof(vertex_buffer), m_renderer.get_buffer(m_vb), 0);
	}
	m_renderer.end_copy_pass(std::move(acp));

	ActiveRenderPass arp = m_renderer.begin_window_render_pass();
	if (arp.is_valid()) {
		this_tick_ms = SDL_GetTicks();
		
		std::cout << "Frame: " << frame_num << "  FPS: " << 1000.0 / (float) (this_tick_ms - last_tick_ms) << "\n";

		frame_num++;
		last_tick_ms = this_tick_ms;

		arp.use_shader(m_shader0);

		// Upload uniforms
		float bd0f[32] = { 0 };

		// Generate projection & eye matrix, then copy to uniform buffer
		mat4x4 eye = glm::identity<mat4x4>();
		eye = glm::translate(eye, vec3(cos(this_tick_ms / 1000.0f), sin(this_tick_ms / 1000.0f), 0.0f));
		
		
		mat4x4 eyeproj = m_renderer.generate_perspective(deg_to_rad(90.0)) * glm::inverse(eye);
		memmove((byte *) (bd0f + 0), (void *) &eyeproj, sizeof(eyeproj));

		// Same for world matrix, but invert first
		mat4x4 world = glm::identity<mat4x4>();
		world = glm::translate(world, vec3(0.0f, 0.0f, -2.0f));

		world *= glm::mat4x4(glm::angleAxis(this_tick_ms / 1000.0f, vec3(0.0, 1.0, 0.0)));
		memmove((byte *) (bd0f + 16), (void *) &world, sizeof(world));

		arp.upload_vertex_uniform_buffer(0, (void *) bd0f, sizeof(bd0f));

		arp.bind_vertex_buffers(m_renderer.get_buffer(m_vb));
		arp.draw(3, 1);
	}
	m_renderer.end_render_pass(std::move(arp));
}

void AppImpl::process_sdl_event(SDL_Event &event) {
	switch (event.type) {
	case SDL_EVENT_WINDOW_RESIZED: {
		m_renderer.resize_window((u32) event.window.data1, (u32) event.window.data1);
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
		.num_samplers = 0,
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
			{ // Color
				.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
				.size = 4 * 3
			},
		},
		.inst_attribs = {},
		.cull_mode = SDL_GPU_CULLMODE_NONE
	};

	m_shader0 = m_renderer.add_shader(vert_stage, frag_stage, std::move(pip_info));

	m_vb = m_renderer.create_buffer(SDL_GPU_BUFFERUSAGE_VERTEX, sizeof(vertex_buffer));
}

void AppImpl::any_close() {}

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
