#pragma once

#include "dx_config.h"

namespace emt
{
struct dx_descriptor_handle
{
	D3D12_CPU_DESCRIPTOR_HANDLE cpu;
	D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	uint32_t                    index;
};

struct dx_descriptor_pool
{
	void initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity);
	void release();
	void reset_frame();

	dx_descriptor_handle alloc_handle(uint32_t count = 1);

	ID3D12DescriptorHeap*       heap{};
	uint32_t                    stride{};
	D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
	uint32_t                    next{};
	uint32_t                    cap{};
};

}        // namespace emt
