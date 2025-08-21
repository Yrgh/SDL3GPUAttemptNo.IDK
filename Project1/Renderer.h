#pragma once

#include "common.h"
#include "Shader.h"
#include "ActiveCopyPass.h"
#include "ActiveRenderPass.h"

// Must be at least the max texture width * the largest supported format
// Currently: 16MiB
inline const u32 TRANSFER_BUFFER_SIZE = 1024 * 1024 * 16;

// NOTE: Compressed texture formats are not supported (yet)

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

enum class RendererCleanupExclude {
	NONE, // Used by the destructor
	INTERNALS, // Exclude internals
	DEFAULT // Exclude both shaders and internals
};

struct TextureState {
	u32 mip_levels;
	bool dirty_mip;

	SDL_GPUTextureFormat format;
	SDL_GPUTextureType type;
	SDL_GPUTextureUsageFlags usage;

	u32 width, height, depth;
};

class Renderer {
	SDL_GPUDevice *m_device = nullptr;

	std::vector<VisualShader> m_shaders;

	std::vector<SDL_GPUBuffer *> m_buffers;
	std::vector<SDL_GPUBufferCreateInfo> m_buffer_infos;

	std::vector<SDL_GPUTexture *> m_screen_textures;
	std::vector<ScreenTextureInfo> m_screen_tex_infos;

	std::vector<SDL_GPUTexture *> m_user_textures;
	std::vector<TextureState> m_texture_states;

	std::vector<SDL_GPUSampler *> m_samplers;

	SDL_Window *m_targ_window;
	int m_winw;
	int m_winh;
	SDL_GPUViewport m_viewport;

	SDL_GPUTransferBuffer *m_upload_buffer;

	friend class ActiveRenderPass;

	inline SDL_GPUGraphicsPipeline *get_shader(RID shader) {
		return m_shaders[*shader].m_rp;
	}

	inline const CompiledPipelineInfo &get_shader_info(RID shader) const {
		return m_shaders[*shader].get_info();
	}

	u32 get_unused_buffer();
	u32 get_unused_texture();

	friend class RenderRetarget;

public:
	// This does nothing and initializes nothing. NEVER use instances initialized this way.
	inline Renderer() {}

	/// <summary>
	/// Creates a new renderer
	/// </summary>
	/// <param name="window">- The window to render to</param>
	Renderer(SDL_Window *window);

	~Renderer();

	/// <summary>
	/// By default, leaves shaders and internals untouched. You can specify to exclude internals alone to delete
	/// shaders, or destroy the device prematurely by specifiyng NONE
	/// </summary>
	/// <param name="exclude">- (Optional) What items to leave untouched</param>
	void clean_resources(RendererCleanupExclude exclude = RendererCleanupExclude::DEFAULT);
	
	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;
	
	Renderer(Renderer &&) = default;
	Renderer &operator=(Renderer &&) = default;

	/// <summary>
	/// Creates a graphical shader pipeline and assigns it an RID
	/// </summary>
	/// <param name="vs">- A description of the vertex stage</param>
	/// <param name="fs">- A description of the fragment stage</param>
	/// <param name="pip">- A description of the resulting pipeline</param>
	/// <returns></returns>
	RID add_shader(ShaderStageInfo vs, ShaderStageInfo fs, PipelineInfo &&pip);

	/// <summary>
	/// To be called whenever the target window is resized. This should only be called if the size changes,
	/// and should only be called for the window assigned to it in the constructor
	/// </summary>
	/// <param name="new_w">- The width of the target window after resizing</param>
	/// <param name="new_h">- The height of the target window after resizing</param>
	void resize_window(u32 new_w, u32 new_h);
	
	/// <summary>
	/// Begins a copy pass which can be used to upload/download data from buffers and textures
	/// </summary>
	/// <returns>An ActiveCopyPass, which should be ended with end_copy_pass()</returns>
	ActiveCopyPass begin_copy_pass();

	/// <summary>
	/// Ends a copy pass and does non-pass processing
	/// </summary>
	/// <param name="pass">- The copy pass to end</param>
	void end_copy_pass(ActiveCopyPass pass);

	/// <summary>
	/// Begins a render pass. The render pass will target the window that was specified at creation
	/// </summary>
	/// <returns>An ActiveRenderPass which should be ended with end_render_pass()</returns>
	ActiveRenderPass begin_window_render_pass();

	/// <summary>
	/// Begins a render pass. The render pass will target the specified textures
	/// </summary>
	/// <param name="description">- A description of the color targets and other pipeline information</param>
	/// <returns>An ActiveRenderPass which should be ended with end_render_pass()</returns>
	ActiveRenderPass begin_custom_render_pass(CustomInfo description);

	void end_render_pass(ActiveRenderPass pass);

	/// <summary>
	/// Generates a perspective projection matrix
	/// </summary>
	/// <param name="fov_rad">- The FOV, in radians</param>
	/// <returns>A 4x4 GLM matrix</returns>
	mat4x4 generate_perspective(float fov_rad);

	/// <summary>
	/// Creates a buffer. Slots of deleted buffers will be resused
	/// </summary>
	/// <param name="usage">- SDL flags OR'ed together that describe its purpose</param>
	/// <param name="size">- The length in bytes of the buffer</param>
	/// <returns>An RID representing the buffer</returns>
	RID create_buffer(SDL_GPUBufferUsageFlags usage, u32 size);

	/// <summary>
	/// Resizes a buffer, reusing the usage flagse.
	/// </summary>
	/// <param name="buffer">- An RID representing the buffer</param>
	/// <param name="size">- The new size of the buffer</param>
	void resize_buffer(RID buffer, u32 size);

	/// <summary>
	/// Deletes a buffer and allows a new one to be allocated in its stead.
	/// </summary>
	/// <param name="buffer">- An RID representing the buffer</param>
	void destroy_buffer(RID buffer);

	// Returns the SDL handle for the RID representing a buffer. This handle should only be acquired from
	// create_buffer()
	inline SDL_GPUBuffer *get_buffer(RID buffer) {
		return *buffer == U32_BAD ? nullptr : m_buffers[*buffer];
	}
	
	// Returns true if the RID is a valid index and the buffer has not been deleted
	bool is_buffer_valid(RID buffer);
	
	/// <summary>
	/// Creates a 2D texture that resizes to the screen size. Once freed, the slot will not be reused, so
	/// allocate them wisely
	/// </summary>
	/// <param name="format">- The SDL format of the texture</param>
	/// <param name="usage">- The SDL flags describing its purpose</param>
	/// <returns>An RID representing the texture</returns>
	RID create_screen_texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage);

	// Returns the SDL handle for the RID representing the texture. This handle should only be acquired from
	// create_screen_texture()
	inline SDL_GPUTexture *get_screen_texture(RID texture) {
		return *texture == U32_BAD ? nullptr : m_screen_textures[*texture];
	}
	
	/// <summary>
	/// Deletes the texture, but does not free it for replacement.
	/// </summary>
	/// <param name="texture">- An RID representing the texture</param>
	void destroy_screen_texture(RID texture);
	
	/// <summary>
	/// Creates a texture. Slots of deleted textures will be reused.
	/// 
	/// Additionally, if 0 is passed for the mip levels, a value will be assigned automatically
	/// </summary>
	/// <param name="info">- The info that will be provided directly to SDL</param>
	/// <returns>An RID representing the texture</returns>
	RID create_texture(const SDL_GPUTextureCreateInfo *info);
	
	// Returns the SDL handle for the RID representing the texture. This handle should only be acquired from
	// create_texture()
	inline SDL_GPUTexture *get_texture(RID texture) {
		return *texture == U32_BAD ? nullptr : m_user_textures[*texture];
	}

	inline TextureState &get_texture_info(RID texture) {
		return m_texture_states[*texture];
	}
	
	// Destroys the texture, allowing a new one to replace its position
	void destroy_texture(RID texture);
	
	// Returns true if the RID is a valid index and the texture has not been deleted
	bool is_texture_valid(RID texture);

	/// <summary>
	/// Creates a sampler that can be used in shaders. Once freed, the slot will not be reused, so
	/// create your samplers beforehand and reuse them between textures
	/// </summary>
	/// <param name="linear_sample">- Whether or not to use linear sampling</param>
	/// <param name="clamp_uv">- Whether to clamp coordinates or wrap them</param>
	/// <param name="anisotropy">- If >0, enables anisotropy on the specified level (eg. 4x = 4.0f)</param>
	/// <returns></returns>
	RID create_sampler(bool linear_sample, bool clamp_uv, float anisotropy = 0.0f);
	
	// Returns the SDL handle for the RID representing the sampler. This handle should only be acquired from
	// create_sampler()
	inline SDL_GPUSampler *get_sampler(RID sampler) {
		return *sampler == U32_BAD ? nullptr : m_samplers[*sampler];
	}

	// Returns the SDL handle used for GPU operations
	inline SDL_GPUDevice *get_device() { return m_device; }
};

