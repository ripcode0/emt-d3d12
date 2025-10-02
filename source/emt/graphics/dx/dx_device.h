// ==============================
// File: dx_device.h
// Minimal Direct3D 12 device utilities built to pair with your dx_context_core
// Updated: unified dx_buffer type (vertex / index / constant)
// Features
//  - Upload begin/end w/ explicit command list (no lambdas)
//  - Unified dx_buffer that can represent Vertex / Index / Constant / Raw
//  - Helpers to bind IA vertex/index and root CBV
//  - Linear GPU-visible CBV/SRV/UAV heap
// ==============================
#pragma once

#include <emt/core/typedef.h>
#include "dx_config.h"

namespace emt
{

struct Texture2D
{
	ID3D12Resource* resource{};
	ID3D12Resource* upload{};
	UINT            width{};
	UINT            height{};
	DXGI_FORMAT     format{DXGI_FORMAT_R8G8B8A8_UNORM};
};

class descriptor_heap_gpu
{
public:
	descriptor_heap_gpu() = default;
	~descriptor_heap_gpu() { release(); }

	void create(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity);
	void release();

	struct alloc_out
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu{};
		D3D12_GPU_DESCRIPTOR_HANDLE gpu{};
		UINT                        index{};
	};
	alloc_out alloc(UINT n = 1);

	ID3D12DescriptorHeap* heap() const { return m_heap; }
	void                  reset() { m_next = 0; }

private:
	ID3D12Device*               m_device{};
	D3D12_DESCRIPTOR_HEAP_TYPE  m_type{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV};
	ID3D12DescriptorHeap*       m_heap{};
	UINT                        m_capacity{};
	UINT                        m_stride{};
	UINT                        m_next{};
	D3D12_CPU_DESCRIPTOR_HANDLE m_cpu{};
	D3D12_GPU_DESCRIPTOR_HANDLE m_gpu{};
};

class dx_device
{
public:
	dx_device() = default;
	~dx_device() { release(); }

	void initialize(ID3D12Device* dev, ID3D12CommandQueue* gfx_queue);
	void release();

	// Upload command list control (no lambdas)
	void                       begin_upload();
	void                       end_upload();
	ID3D12GraphicsCommandList* upload_cmdlist() { return m_cmd; }

	void create_buffer(const buffer_create_info* info, dx_buffer** pp_buffer);

	// Unified buffer creators
	// dx_buffer create_buffer_vertex(const void* data, UINT byteSize, UINT stride);
	// dx_buffer create_buffer_index(const void* data, UINT byteSize, DXGI_FORMAT fmt);        // R16_UINT or R32_UINT
	// dx_buffer create_buffer_constant(UINT byteSize, D3D12_CPU_DESCRIPTOR_HANDLE* out_cbv_cpu = nullptr);        // upload heap
	// dx_buffer create_buffer_raw(UINT byteSize, D3D12_RESOURCE_STATES initial = D3D12_RESOURCE_STATE_COMMON);

	// Textures
	Texture2D create_texture2d_rgba8(const void* pixels, UINT width, UINT height, UINT rowStride);

	// Descriptors
	D3D12_GPU_DESCRIPTOR_HANDLE create_cbv_gpu(ID3D12Resource* resource, UINT byteSize);
	D3D12_GPU_DESCRIPTOR_HANDLE create_srv_texture2d_gpu(ID3D12Resource* resource, DXGI_FORMAT format);

	ID3D12RootSignature* create_basic_root_signature(bool sampler_in_root = false);

	descriptor_heap_gpu* cbv_srv_uav_heap() { return &m_heap_cbv_srv_uav; }

	// Barrier helper
	static void transition(ID3D12GraphicsCommandList* cl, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after);

	const ID3D12Device* handle() const { return m_device; }

private:
	void            signal_and_wait();
	ID3D12Resource* create_default_buffer(UINT64 size);
	ID3D12Resource* create_upload_buffer(UINT64 size, const void* initData = nullptr);

private:
	ID3D12Device*       m_device{};
	ID3D12CommandQueue* m_queue{};

	ID3D12CommandAllocator*    m_upload_alloc{};
	ID3D12GraphicsCommandList* m_cmd{};
	ID3D12Fence*               m_fence{};
	HANDLE                     m_fence_event{};
	UINT64                     m_fence_value{};

	descriptor_heap_gpu m_heap_cbv_srv_uav;
};

}        // namespace emt