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
	bool cycle_buffer = true;

	SDL_GPUTransferBufferLocation source = {
		.transfer_buffer = m_upload,
		.offset = 0
	};
	
	SDL_GPUBufferRegion dest = {
		.buffer = buffer,
	};

	u32 offset = 0;
	while (length > 0) {
		u32 size = length > TRANSFER_BUFFER_SIZE ? TRANSFER_BUFFER_SIZE : length;
		length -= size;

		memmove(SDL_MapGPUTransferBuffer(m_renderer->get_device(), m_upload, true), data, size);
		SDL_UnmapGPUTransferBuffer(m_renderer->get_device(), m_upload);
		
		dest.offset = offset;
		dest.size = size;

		SDL_UploadToGPUBuffer(m_cp, &source, &dest, cycle_buffer);
		// Only cycle the first time
		cycle_buffer = false;

		offset += size;
	}
}

void ActiveCopyPass::upload_texture(byte *data, u32 length, RID dest_tex) {
	SDL_GPUTexture *texture = m_renderer->get_texture(dest_tex);
	auto &info = m_renderer->get_texture_info(dest_tex);

	u32 pixel_size = SDL_GPUTextureFormatTexelBlockSize(info.format);
	u32 bytes_per_row = pixel_size * info.width;

	// TODO: bytes_per_row currently cannot exceed TRANSFER_BUFFER_SIZE
	// FIX: Break rows up too. *Should* be easy
	
	// The number of rows per group
	u32 max_rows = TRANSFER_BUFFER_SIZE / bytes_per_row;
	u32 extra_rows = info.height % max_rows;
	u32 num_groups = info.height / max_rows;

	u32 bytes_per_layer = bytes_per_row * info.height;

	SDL_GPUTextureTransferInfo ti = {
		.transfer_buffer = m_upload,
		.offset = 0,
		.pixels_per_row = info.width,
		.rows_per_layer = info.height,
	};

	SDL_GPUTextureRegion region = {
		.texture = texture,
		.mip_level = 0,
		.layer = 0,
		.x = 0,
		.y = 0,
		.z = 0,
		.w = info.width,
		/*Height omitted*/
		.d = 1
	};

	// Gotta love APIs
	bool use_layer_field;
	switch (info.type) {
	case SDL_GPU_TEXTURETYPE_2D:
		use_layer_field = true;
		break;
	case SDL_GPU_TEXTURETYPE_2D_ARRAY:
		use_layer_field = true;
		break;
	case SDL_GPU_TEXTURETYPE_3D:
		use_layer_field = false;
		break;
	case SDL_GPU_TEXTURETYPE_CUBE:
		use_layer_field = false;
		break;
	case SDL_GPU_TEXTURETYPE_CUBE_ARRAY:
		use_layer_field = true;
		break;
	default:
		use_layer_field = true;
		break;
	}

	bool cycle_texture = true;

	for (u32 layer = 0; layer < info.depth; ++layer) {
		if (use_layer_field) {
			region.layer = layer;
		} else {
			region.z = layer;
		}

		region.h = max_rows;

		for (u32 i = 0; i < num_groups; ++i) {
			u32 offset = layer * bytes_per_layer + max_rows * i * bytes_per_row;

			region.y = i * max_rows;

			memmove(SDL_MapGPUTransferBuffer(m_renderer->get_device(), m_upload, true), data + offset, bytes_per_row * max_rows);
			SDL_UnmapGPUTransferBuffer(m_renderer->get_device(), m_upload);

			SDL_UploadToGPUTexture(m_cp, &ti, &region, cycle_texture);
			// Only cycle the first time
			cycle_texture = false;
		}

		if (extra_rows == 0) {
			continue;
		}

		u32 offset = layer * bytes_per_layer + num_groups * bytes_per_row * max_rows;

		region.y = num_groups * max_rows;
		region.h = extra_rows;

		memmove(SDL_MapGPUTransferBuffer(m_renderer->get_device(), m_upload, true), data + offset, bytes_per_row * extra_rows);
		SDL_UnmapGPUTransferBuffer(m_renderer->get_device(), m_upload);

		SDL_UploadToGPUTexture(m_cp, &ti, &region, cycle_texture);
		cycle_texture = false;
	}

	info.dirty_mip = info.mip_levels > 1;
}
