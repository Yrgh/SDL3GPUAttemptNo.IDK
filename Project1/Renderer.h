#pragma once

#include "common.h"
#include "Shader.h"
#include "ActiveCopyPass.h"
#include "ActiveRenderPass.h"

inline const size_t TRANSFER_BUFFER_SIZE = 1024;

struct CustomTargetInfo {
	SDL_GPUTexture *texture;
	SDL_FColor clear_color = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct CustomInfo {
	std::vector<CustomTargetInfo> color_targets;

	bool depth_enabled = true;
	SDL_GPUTexture *depth_texture; // Ignore if depth_enabled is false

	SDL_GPUViewport viewport;
};

struct ScreenTextureInfo {
	SDL_GPUTextureFormat format;
	SDL_GPUTextureUsageFlags usage;
};

class Renderer {
	SDL_GPUDevice *m_device;

	std::vector<VisualShader> m_shaders;

	std::vector<SDL_GPUBuffer *> m_buffers;
	std::vector<SDL_GPUBufferCreateInfo> m_buffer_infos;

	std::vector<SDL_GPUTexture *> m_screen_textures;
	std::vector<ScreenTextureInfo> m_screen_tex_infos;

	// Removed. Replaced with generic screen textures. Screen texture 0 is always this one
	//SDL_GPUTexture *m_depth_tex;

	SDL_Window *m_targ_window;
	int m_winw;
	int m_winh;
	SDL_GPUViewport m_viewport;

	SDL_GPUTransferBuffer *m_upload_buffer;
	SDL_GPUTransferBuffer *m_download_buffer;

	friend class ActiveRenderPass;

	inline SDL_GPUGraphicsPipeline *get_shader(RID shader) {
		return m_shaders[*shader].m_rp;
	}

	inline const CompiledPipelineInfo &get_shader_info(RID shader) const {
		return m_shaders[*shader].get_info();
	}

	u32 get_unused_buffer();
public:
	// This does nothing and initializes nothing. NEVER use instances initialized this way.
	inline Renderer() {}

	// You don't need to specify a device; one can be created for you
	Renderer(SDL_Window *window, SDL_GPUDevice *device = nullptr);
	~Renderer();

	void destroy();
	
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;
	
	Renderer(Renderer &&) = default;
	Renderer &operator=(Renderer &&) = default;

	RID add_shader(ShaderStageInfo vs, ShaderStageInfo fs, PipelineInfo &&pip);

	void resize_window(u32 new_w, u32 new_h);
	
	ActiveCopyPass begin_copy_pass();
	void end_copy_pass(ActiveCopyPass);

	// Begins a render pass that targets the window
	ActiveRenderPass begin_window_render_pass();
	// Begins a render pass with custom targets
	ActiveRenderPass begin_custom_render_pass(CustomInfo description);
	void end_render_pass(ActiveRenderPass);

	mat4x4 generate_perspective(float fov_rad);

	// Reuses the RID for a delete buffer if possible
	RID create_buffer(SDL_GPUBufferUsageFlags usage, u32 size);
	void resize_buffer(RID buffer, u32 size);
	void destroy_buffer(RID buffer);
	inline SDL_GPUBuffer *get_buffer(RID buffer) { return m_buffers[*buffer]; }
	// Returns true if the RID is a valid index and the buffer has not been deleted
	bool is_buffer_valid(RID buffer);

	// Unlike buffers, does not reuse old slots
	RID create_screen_texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage);
	inline SDL_GPUTexture *get_screen_texture(RID texture) { return m_screen_textures[*texture]; }
	void destroy_screen_texture(RID texture);

	inline SDL_GPUDevice *get_device() { return m_device; }
};

