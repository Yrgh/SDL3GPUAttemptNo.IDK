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

	ActiveCopyPass(Renderer &);
public:
	ActiveCopyPass();

	ActiveCopyPass(const ActiveCopyPass &) = delete;
	ActiveCopyPass &operator=(const ActiveCopyPass &) = delete;

	ActiveCopyPass(ActiveCopyPass &&) noexcept = default;
	ActiveCopyPass &operator=(ActiveCopyPass &&) noexcept = default;

	void download_buffer(const byte *data, u32 length, SDL_GPUBuffer *source_buf, u32 offset = 0);
	void upload_buffer(byte *data, u32 length, SDL_GPUBuffer *dest_buf, u32 offset = 0);

	inline bool is_valid() const { return m_renderer; }
};