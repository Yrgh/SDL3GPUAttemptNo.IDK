#include "ActiveCopyPass.h"
#include "Renderer.h"

ActiveCopyPass::ActiveCopyPass(Renderer &renderer):
	m_renderer(&renderer)
{}

ActiveCopyPass::ActiveCopyPass():
	m_renderer(nullptr)
{}

void ActiveCopyPass::upload_buffer(byte *data, u32 length, RID dest_buf) {
	SDL_GPUBuffer *buffer = m_renderer->get_buffer(dest_buf);

	u32 offset = 0;
	while (length > 0) {
		u32 size = length > TRANSFER_BUFFER_SIZE ? TRANSFER_BUFFER_SIZE : length;
		length -= size;

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

		offset += size;
	}
}

void ActiveCopyPass::upload_texture(byte *data, u32 length, RID dest_tex) {
	SDL_GPUTexture *texture = m_renderer->get_texture(dest_tex);
	auto &info = m_renderer->get_texture_info(dest_tex);
	u32 pixel_size = SDL_GPUTextureFormatTexelBlockSize(info.format);

	std::cout << "pixel_size: " << pixel_size << "  info.width: " << info.width << "\n";
	
	u32 pixels_fit = TRANSFER_BUFFER_SIZE / pixel_size;
	u32 max_rows = pixels_fit / info.width;

	std::cout << "max_rows: " << max_rows << "\n";

	u32 bytes_per_group = max_rows * info.width * pixel_size;

	
}
