#include "ActiveCopyPass.h"
#include "Renderer.h"

ActiveCopyPass::ActiveCopyPass(Renderer &renderer):
	m_renderer(&renderer)
{}

ActiveCopyPass::ActiveCopyPass():
	m_renderer(nullptr)
{}

void ActiveCopyPass::download_buffer(const byte *data, u32 length, RID source_buf, u32 offset) {}

void ActiveCopyPass::upload_buffer(byte *data, u32 length, RID dest_buf, u32 offset) {
	u32 size = length > TRANSFER_BUFFER_SIZE ? TRANSFER_BUFFER_SIZE : length;

	SDL_GPUBuffer *buffer = m_renderer->get_buffer(dest_buf);

	memmove(SDL_MapGPUTransferBuffer(m_renderer->get_device(), m_upload, true), data, size);

	SDL_UnmapGPUTransferBuffer(m_renderer->get_device(), m_upload);

	SDL_GPUTransferBufferLocation source = {
		.transfer_buffer = m_upload,
		.offset = offset
	};

	SDL_GPUBufferRegion dest = {
		.buffer = buffer,
		.offset = offset,
		.size = size
	};

	SDL_UploadToGPUBuffer(m_cp, &source, &dest, true);

	if (length > TRANSFER_BUFFER_SIZE) {
		upload_buffer(data + TRANSFER_BUFFER_SIZE, length - TRANSFER_BUFFER_SIZE, dest_buf, offset + TRANSFER_BUFFER_SIZE);
	}
}
