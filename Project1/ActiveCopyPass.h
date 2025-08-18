#pragma once
#include "common.h"

class Renderer;

class ActiveCopyPass {
	Renderer *m_renderer;

	SDL_GPUCommandBuffer *m_cb;
	SDL_GPUCopyPass *m_cp;

	SDL_GPUTransferBuffer *m_upload;
	SDL_GPUTransferBuffer *m_download;

	friend class Renderer;
	friend class RenderRetarget;

	ActiveCopyPass(Renderer &);
public:
	ActiveCopyPass();

	ActiveCopyPass(const ActiveCopyPass &) = delete;
	ActiveCopyPass &operator=(const ActiveCopyPass &) = delete;

	ActiveCopyPass(ActiveCopyPass &&) noexcept = default;
	ActiveCopyPass &operator=(ActiveCopyPass &&) noexcept = default;

	void download_buffer(const byte *data, u32 length, RID source_buf);
	void upload_buffer(byte *data, u32 length, RID dest_buf);

	void upload_texture(byte *data, u32 length, RID dest_tex);

	inline bool is_valid() const { return m_renderer; }
};