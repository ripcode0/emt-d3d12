#include "dx_descriptor_pool.h"

namespace emt
{
void dx_descriptor_pool::initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t capacity)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type           = type;
	desc.NumDescriptors = capacity;
	desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	HR(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));

	cap    = capacity;
	stride = device->GetDescriptorHandleIncrementSize(type);
	cpu    = heap->GetCPUDescriptorHandleForHeapStart();
	gpu    = heap->GetGPUDescriptorHandleForHeapStart();
	next   = 0;
}

void dx_descriptor_pool::release()
{
	safe_release(heap);
	cap = stride = next = 0;
	cpu                 = {};
	gpu                 = {};
}

void dx_descriptor_pool::reset_frame()
{
	next = 0;
}

dx_descriptor_handle dx_descriptor_pool::alloc_handle(uint32_t count)
{
	dx_descriptor_handle handle{};
	if(!heap || next + count > cap) {
		return handle;
	}

	uint32_t offset = next * stride;
	handle.cpu.ptr  = cpu.ptr + offset;
	handle.gpu.ptr  = gpu.ptr + offset;
	handle.index    = next;
	next += count;

	return handle;
}

}        // namespace emt
