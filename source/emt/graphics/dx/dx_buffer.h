#pragma once

#include <emt/core/typedef.h>
#include "dx_config.h"

namespace emt
{
// Vertex Buffer
// size / stride / state STATE_VERTEX_AND_CONSTANT_BUFFER
// view.BufferLocation = gpu_addr
struct dx_buffer
{
	buffer_type               type;
	ID3D12Resource*           handle{};
	D3D12_GPU_VIRTUAL_ADDRESS gpu_addr{};
	uint64_t                  size{};

	union
	{
		D3D12_VERTEX_BUFFER_VIEW vtx_view;
		D3D12_INDEX_BUFFER_VIEW  idx_view;
	};

	~dx_buffer() { release(); }
	void release() { safe_release(handle); };
};

}        // namespace emt
