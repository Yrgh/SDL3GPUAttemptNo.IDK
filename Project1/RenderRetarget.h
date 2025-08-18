#pragma once

#include "common.h"
#include "Renderer.h"

/// <summary>
/// Adds additional window targetting to a Renderer without requiring an additional Renderer and its resources to be created.
/// Most functions will require the original Renderer. Exceptions are those that rely on the target window, such as begin_window_render_pass()
/// </summary>
class RenderRetarget {
	Renderer *m_renderer;

	SDL_Window *m_targ_window;
	int m_winw;
	int m_winh;
	SDL_GPUViewport m_viewport;

	std::vector<SDL_GPUTexture *> m_screen_textures;
	std::vector<ScreenTextureInfo> m_screen_tex_infos;

public:
	// This does nothing and initializes nothing. NEVER use instances initialized this way.
	inline RenderRetarget() {}

	RenderRetarget(Renderer &renderer, SDL_Window *window);
	~RenderRetarget();

	RenderRetarget(const RenderRetarget &) = delete;
	RenderRetarget &operator=(const RenderRetarget &) = delete;

	RenderRetarget(RenderRetarget &&) = default;
	RenderRetarget &operator=(RenderRetarget &&) = default;

	void release();

	/// <summary>
	/// To be called whenever the target window is resized. This should only be called if the size changes,
	/// and should only be called for the window assigned to it in the constructor
	/// </summary>
	/// <param name="new_w">- The width of the target window after resizing</param>
	/// <param name="new_h">- The height of the target window after resizing</param>
	void resize_window(u32 new_w, u32 new_h);

	/// <summary>
	/// Generates a perspective projection matrix
	/// </summary>
	/// <param name="fov_rad">- The FOV, in radians</param>
	/// <returns>A 4x4 GLM matrix</returns>
	mat4x4 generate_perspective(float fov_rad);

	/// <summary>
	/// Begins a render pass. The render pass will target the window that was specified at creation
	/// </summary>
	/// <returns>An ActiveRenderPass which should be ended with end_render_pass() on the original Renderer</returns>
	ActiveRenderPass begin_window_render_pass();

	/// <summary>
	/// Creates a 2D texture that resizes to the screen size. Once freed, the slot will not be reused, so
	/// allocate them wisely
	/// </summary>
	/// <param name="format">- The SDL format of the texture</param>
	/// <param name="usage">- The SDL flags describing its purpose</param>
	/// <returns>An RID representing the texture</returns>
	RID create_screen_texture(SDL_GPUTextureFormat format, SDL_GPUTextureUsageFlags usage);

	// Returns the SDL handle for the RID representing the texture. This handle should only be acquired from
	// create_screen_texture(), and only from this instance
	inline SDL_GPUTexture *get_screen_texture(RID texture) {
		return *texture == U32_BAD ? nullptr : m_screen_textures[*texture];
	}

	/// <summary>
	/// Deletes the texture, but does not free it for replacement.
	/// </summary>
	/// <param name="texture">- An RID representing the texture</param>
	void destroy_screen_texture(RID texture);

	/// <summary>
	/// If you have access to the original Renderer, it would be cleaner to use that instead.
	/// </summary>
	/// <returns>The Renderer this class was created with</returns>
	inline Renderer &get_renderer() { return *m_renderer; }
};

