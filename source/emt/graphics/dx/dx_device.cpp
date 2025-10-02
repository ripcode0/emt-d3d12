#include "dx_device.h"
#include "dx_buffer.h"

namespace emt
{

// ===== descriptor_heap_gpu =====
void descriptor_heap_gpu::create(ID3D12Device* dev, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT capacity)
{
	release();
	m_device   = dev;
	m_type     = type;
	m_capacity = capacity;
	D3D12_DESCRIPTOR_HEAP_DESC desc{};
	desc.Type           = type;
	desc.NumDescriptors = capacity;
	desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(dev->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
	m_heap->SetName(L"HEAPS UPLOADERED");
	m_stride = dev->GetDescriptorHandleIncrementSize(type);
	m_cpu    = m_heap->GetCPUDescriptorHandleForHeapStart();
	m_gpu    = m_heap->GetGPUDescriptorHandleForHeapStart();
	m_next   = 0;
}

void descriptor_heap_gpu::release()
{
	safe_release(m_heap);
	m_device   = nullptr;
	m_capacity = 0;
	m_stride   = 0;
	m_next     = 0;
}

descriptor_heap_gpu::alloc_out descriptor_heap_gpu::alloc(UINT n)
{
	alloc_out out{};
	if((m_next + n) > m_capacity) {
		__debugbreak();
		return out;
	}
	out.index   = m_next;
	out.cpu.ptr = m_cpu.ptr + SIZE_T(m_next) * m_stride;
	out.gpu.ptr = m_gpu.ptr + UINT64(m_next) * UINT64(m_stride);
	m_next += n;
	return out;
}

// ===== dx_device =====
void dx_device::initialize(ID3D12Device* dev, ID3D12CommandQueue* gfx_queue)
{
	m_device = dev;
	m_queue  = gfx_queue;

	HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_upload_alloc)));
	HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_upload_alloc, nullptr, IID_PPV_ARGS(&m_cmd)));
	HR(m_cmd->Close());

	HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
	m_fence_event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
	m_fence_value = 0;

	m_heap_cbv_srv_uav.create(m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1024);
	m_cmd->SetName(L"Uploaded CMD");
	m_fence->SetName(L"Uploaded Fence");
}

void dx_device::release()
{
	if(m_device && m_fence) {
		signal_and_wait();
	}
	// signal_and_wait();
	if(m_fence_event) {
		::CloseHandle(m_fence_event);
		m_fence_event = nullptr;
	}
	m_heap_cbv_srv_uav.release();

	if(m_upload_alloc && m_cmd) {
		m_upload_alloc->Reset();
		m_cmd->Reset(m_upload_alloc, nullptr);
		m_cmd->ClearState(nullptr);
		m_cmd->Close();
	}
	safe_release(m_cmd);
	safe_release(m_upload_alloc);
	safe_release(m_fence);
	m_device = nullptr;
	m_queue  = nullptr;
}

void dx_device::begin_upload()
{
	HR(m_upload_alloc->Reset());
	HR(m_cmd->Reset(m_upload_alloc, nullptr));
}

void dx_device::end_upload()
{
	HR(m_cmd->Close());
	ID3D12CommandList* lists[] = {m_cmd};
	m_queue->ExecuteCommandLists(1, lists);
	signal_and_wait();
}

void dx_device::create_buffer(const buffer_create_info* info, dx_buffer** pp_buffer)
{
	ID3D12Resource* buffer         = create_default_buffer(info->size);
	ID3D12Resource* staging_buffer = create_upload_buffer(info->size, info->data);

	D3D12_RESOURCE_STATES layout = D3D12_RESOURCE_STATE_COMMON;

	switch(info->type) {
		case buffer_type::vertex:
			layout = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		case buffer_type::index:
			layout = D3D12_RESOURCE_STATE_INDEX_BUFFER;
			break;
		case buffer_type::uniform:
			layout = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			break;
		default:
			log_assert(0, "unknown type is not supported");
			break;
	}

	begin_upload();
	transition(m_cmd, buffer,
	           D3D12_RESOURCE_STATE_COMMON,
	           D3D12_RESOURCE_STATE_COPY_DEST);
	m_cmd->CopyBufferRegion(buffer, 0, staging_buffer, 0, info->size);
	transition(m_cmd, buffer,
	           D3D12_RESOURCE_STATE_COPY_DEST,
	           layout);
	end_upload();

	safe_release(staging_buffer);

	dx_buffer* p_buffer               = emt_new dx_buffer;
	p_buffer->type                    = info->type;
	p_buffer->size                    = info->size;
	p_buffer->handle                  = buffer;
	p_buffer->vtx_view.BufferLocation = buffer->GetGPUVirtualAddress();
	p_buffer->vtx_view.SizeInBytes    = info->size;
	p_buffer->gpu_addr                = buffer->GetGPUVirtualAddress();

	*pp_buffer = p_buffer;
}

void dx_device::signal_and_wait()
{
	const UINT64 v = ++m_fence_value;
	HR(m_queue->Signal(m_fence, v));
	HR(m_fence->SetEventOnCompletion(v, m_fence_event));
	::WaitForSingleObject(m_fence_event, INFINITE);
}

ID3D12Resource* dx_device::create_default_buffer(UINT64 size)
{
	ID3D12Resource*       res{};
	D3D12_HEAP_PROPERTIES hp{};
	hp.Type                 = D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask     = 1;
	hp.VisibleNodeMask      = 1;
	D3D12_RESOURCE_DESC rd  = CD3DX12_RESOURCE_DESC::Buffer(size);
	HR(m_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
	                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&res)));
	return res;
}

ID3D12Resource* dx_device::create_upload_buffer(UINT64 size, const void* initData)
{
	ID3D12Resource*       res{};
	D3D12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC   rd = CD3DX12_RESOURCE_DESC::Buffer(size);
	HR(m_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
	                                     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&res)));
	if(initData) {
		void*         p;
		CD3DX12_RANGE range(0, 0);
		HR(res->Map(0, &range, &p));
		std::memcpy(p, initData, (size_t)size);
		res->Unmap(0, nullptr);
	}
	return res;
}

void dx_device::transition(ID3D12GraphicsCommandList* cl, ID3D12Resource* res, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	if(before == after)
		return;
	D3D12_RESOURCE_BARRIER b{};
	b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource   = res;
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = before;
	b.Transition.StateAfter  = after;
	cl->ResourceBarrier(1, &b);
}

// ---- Unified buffer creators ----

// dx_buffer dx_device::create_buffer_vertex(const void* data, UINT byteSize, UINT stride)
// {
// 	dx_buffer out{};
// 	out.type               = buffer_type::vertex;
// 	out.size               = byteSize;
// 	out.stride             = stride;
// 	out.resource           = create_default_buffer(byteSize);
// 	ID3D12Resource* upload = create_upload_buffer(byteSize, data);

// 	begin_upload();
// 	transition(m_upload_cmdlist, out.resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
// 	m_upload_cmdlist->CopyBufferRegion(out.resource, 0, upload, 0, byteSize);
// 	transition(m_upload_cmdlist, out.resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
// 	end_upload();

// 	safe_release(upload);
// 	out.state              = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
// 	out.vbv.BufferLocation = out.resource->GetGPUVirtualAddress();
// 	out.vbv.SizeInBytes    = byteSize;
// 	out.vbv.StrideInBytes  = stride;
// 	return out;
// }

// dx_buffer dx_device::create_buffer_index(const void* data, UINT byteSize, DXGI_FORMAT fmt)
// {
// 	dx_buffer out{};
// 	out.type               = buffer_type::index;
// 	out.size               = byteSize;
// 	out.index_format       = fmt;
// 	out.resource           = create_default_buffer(byteSize);
// 	ID3D12Resource* upload = create_upload_buffer(byteSize, data);

// 	begin_upload();
// 	transition(m_upload_cmdlist, out.resource,
// 	           D3D12_RESOURCE_STATE_COMMON,
// 	           D3D12_RESOURCE_STATE_COPY_DEST);
// 	m_upload_cmdlist->CopyBufferRegion(out.resource, 0, upload, 0, byteSize);
// 	transition(m_upload_cmdlist, out.resource,
// 	           D3D12_RESOURCE_STATE_COPY_DEST,
// 	           D3D12_RESOURCE_STATE_INDEX_BUFFER);
// 	end_upload();

// 	safe_release(upload);
// 	out.state              = D3D12_RESOURCE_STATE_INDEX_BUFFER;
// 	out.ibv.BufferLocation = out.resource->GetGPUVirtualAddress();
// 	out.ibv.SizeInBytes    = byteSize;
// 	out.ibv.Format         = fmt;
// 	return out;
// }

// dx_buffer dx_device::create_buffer_constant(UINT byteSize, D3D12_CPU_DESCRIPTOR_HANDLE* out_cbv_cpu)
// {
// 	dx_buffer out{};
// 	out.type     = buffer_type::uniform;
// 	UINT aligned = (byteSize + 255) & ~255u;
// 	out.size     = aligned;
// 	out.resource = create_upload_buffer(aligned);
// 	out.state    = D3D12_RESOURCE_STATE_GENERIC_READ;
// 	if(out_cbv_cpu) {
// 		auto                            a = m_heap_cbv_srv_uav.alloc();
// 		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
// 		cbv.BufferLocation = out.resource->GetGPUVirtualAddress();
// 		cbv.SizeInBytes    = aligned;
// 		m_device->CreateConstantBufferView(&cbv, a.cpu);
// 		*out_cbv_cpu = a.cpu;
// 	}
// 	return out;
// }

// dx_buffer dx_device::create_buffer_raw(UINT byteSize, D3D12_RESOURCE_STATES initial)
// {
// 	dx_buffer out{};
// 	out.type     = buffer_type::raw;
// 	out.size     = byteSize;
// 	out.resource = create_default_buffer(byteSize);
// 	out.state    = initial;
// 	return out;
// }

// ---- Textures ----
Texture2D dx_device::create_texture2d_rgba8(const void* pixels, UINT width, UINT height, UINT rowStride)
{
	Texture2D out{};
	out.width  = width;
	out.height = height;
	out.format = DXGI_FORMAT_R8G8B8A8_UNORM;

	D3D12_HEAP_PROPERTIES hp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC   rd = CD3DX12_RESOURCE_DESC::Tex2D(out.format, width, height, 1, 1);
	HR(m_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
	                                     D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&out.resource)));

	const UINT64 uploadSize = GetRequiredIntermediateSize(out.resource, 0, 1);
	out.upload              = create_upload_buffer(uploadSize);

	D3D12_SUBRESOURCE_DATA src{};
	src.pData      = pixels;
	src.RowPitch   = (LONG_PTR)rowStride;
	src.SlicePitch = src.RowPitch * height;

	begin_upload();
	transition(m_cmd, out.resource, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	UpdateSubresources(m_cmd, out.resource, out.upload, 0, 0, 1, &src);
	transition(m_cmd, out.resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	end_upload();

	safe_release(out.upload);
	return out;
}

// ---- Descriptor helpers ----
D3D12_GPU_DESCRIPTOR_HANDLE dx_device::create_cbv_gpu(ID3D12Resource* resource, UINT byteSize)
{
	auto                            a       = m_heap_cbv_srv_uav.alloc();
	UINT                            aligned = (byteSize + 255) & ~255u;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv{};
	cbv.BufferLocation = resource->GetGPUVirtualAddress();
	cbv.SizeInBytes    = aligned;
	m_device->CreateConstantBufferView(&cbv, a.cpu);
	return a.gpu;
}

D3D12_GPU_DESCRIPTOR_HANDLE dx_device::create_srv_texture2d_gpu(ID3D12Resource* resource, DXGI_FORMAT format)
{
	auto                            a = m_heap_cbv_srv_uav.alloc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srv{};
	srv.Format                    = format;
	srv.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv.Shader4ComponentMapping   = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv.Texture2D.MipLevels       = 1;
	srv.Texture2D.MostDetailedMip = 0;
	m_device->CreateShaderResourceView(resource, &srv, a.cpu);
	return a.gpu;
}

ID3D12RootSignature* dx_device::create_basic_root_signature(bool sampler_in_root)
{
	CD3DX12_DESCRIPTOR_RANGE range{};
	range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER params[2];
	params[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
	params[1].InitAsDescriptorTable(1, &range, D3D12_SHADER_VISIBILITY_PIXEL);

	D3D12_STATIC_SAMPLER_DESC samp{};
	samp.Filter   = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samp.AddressU = samp.AddressV = samp.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samp.ShaderRegister                           = 0;
	samp.RegisterSpace                            = 0;
	samp.ShaderVisibility                         = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rsd{};
	rsd.NumParameters = 2;
	rsd.pParameters   = params;
	rsd.Flags         = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	if(sampler_in_root) {
		rsd.NumStaticSamplers = 1;
		rsd.pStaticSamplers   = &samp;
	}

	ID3DBlob* blob{};
	ID3DBlob* err{};
	HR(D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &err));

	ID3D12RootSignature* rs{};
	HR(m_device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rs)));
	safe_release(blob);
	safe_release(err);
	return rs;
}

}        // namespace emt
