#include "dx_context_core.h"
#include "dx_shader.h"

namespace emt
{

static inline DXGI_SAMPLE_DESC sample1()
{
	DXGI_SAMPLE_DESC s{};
	s.Count   = 1;
	s.Quality = 0;
	return s;
}

dx_context_core::dx_context_core(uint32_t cx, uint32_t cy, HWND hwnd) :
    context(cx, cy, hwnd)
{
	m_width  = cx;
	m_height = cy;
	initialize_core();
	create_swapchain(hwnd, m_width, m_height, 3);
	dx_shader_cache::initialize();
}

dx_context_core::~dx_context_core()
{
	release();
}

void dx_context_core::initialize_core()
{
	create_device_and_queue();

	m_allow_tearing    = check_tearing_support();
	m_frames_in_flight = MAX_SYNC_FRAME;

	create_frame_resources(m_frames_in_flight);

	// 공용 graphics command list
	HR(m_device->CreateCommandList(
	    0, D3D12_COMMAND_LIST_TYPE_DIRECT,
	    m_frames[0].allocator, /*PSO*/ nullptr,
	    IID_PPV_ARGS(&m_cmdlist)));
	m_cmdlist->Close();        // Reset 전에 닫아둠

	HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_idle_fence)));
	m_idle_fence->SetName(L"IDLE FENCE");
	m_fence_event = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);

	m_graphic_device.initialize(m_device, m_queue);
}

void dx_context_core::release()
{
	wait_idle();
	dx_shader_cache::deinitialize();
	m_graphic_device.release();

	safe_release(m_idle_fence);
	destroy_swapchain_resources();
	safe_release(m_swapchain);

	destroy_frame_resources();

	safe_release(m_cmdlist);

	safe_release(m_queue);

	// #ifdef _DEBUG
	// 	report_live_objects();
	// #endif
	safe_release(m_device);
	safe_release(m_factory);

	if(m_fence_event) {
		::CloseHandle(m_fence_event);
		m_fence_event = nullptr;
	}
#ifdef _DEBUG
	report_live_objects();
#endif
}

void dx_context_core::resize_frame(uint32_t cx, uint32_t cy)
{
	if(!m_swapchain || cx == 0 || cy == 0)
		return;
	wait_idle();

	log_debug("resize context %d : %d", cx, cy);

	m_width  = cx;
	m_height = cy;

	destroy_swapchain_resources();

	UINT flags = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	HR(m_swapchain->ResizeBuffers(
	    m_backbuffer_count, m_width, m_height, DXGI_FORMAT_R8G8B8A8_UNORM, flags));

	// RTV heap
	m_rtv_desc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
	rtvDesc.NumDescriptors = m_backbuffer_count;
	rtvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HR(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtv_heap)));

	m_backbuffers = new ID3D12Resource* [m_backbuffer_count] {};
	for(UINT i = 0; i < m_backbuffer_count; ++i) {
		HR(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backbuffers[i])));
		D3D12_CPU_DESCRIPTOR_HANDLE h = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		h.ptr += SIZE_T(i) * m_rtv_desc_size;
		m_device->CreateRenderTargetView(m_backbuffers[i], nullptr, h);
	}

	m_backbuffer_index = m_swapchain->GetCurrentBackBufferIndex();
}

void dx_context_core::begin_frame()
{
	if(m_frame_latency_waitable) {
		log_debug("wait for frame latency");
		::WaitForSingleObject(m_frame_latency_waitable, INFINITE);
	}
	frame_resources& fr = m_frames[m_frame_index];

	if(fr.fence && fr.fence->GetCompletedValue() < fr.fence_value) {
		HR(fr.fence->SetEventOnCompletion(fr.fence_value, m_fence_event));
		::WaitForSingleObject(m_fence_event, INFINITE);
	}

	descriptor_heap_gpu* gpu_heap = m_graphic_device.cbv_srv_uav_heap();
	gpu_heap->reset();

	HR(fr.allocator->Reset());
	HR(m_cmdlist->Reset(fr.allocator, nullptr));

	D3D12_RESOURCE_BARRIER b{};
	b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource   = m_backbuffers[m_backbuffer_index];
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	b.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
	m_cmdlist->ResourceBarrier(1, &b);
}

void dx_context_core::end_frame(bool vsync)
{
	D3D12_RESOURCE_BARRIER b{};
	b.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	b.Transition.pResource   = m_backbuffers[m_backbuffer_index];
	b.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	b.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	b.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
	m_cmdlist->ResourceBarrier(1, &b);

	HR(m_cmdlist->Close());
	ID3D12CommandList* lists[] = {m_cmdlist};
	m_queue->ExecuteCommandLists(1, lists);

	UINT sync_interval = vsync ? 1 : 0;
	UINT present_flags = (!vsync && m_allow_tearing) ? DXGI_PRESENT_ALLOW_TEARING : 0;
	// HRESULT hr           = m_swapchain->Present(syncInterval, presentFlags);
	HRESULT hr = m_swapchain->Present(sync_interval, present_flags);

	if(hr == DXGI_STATUS_OCCLUDED) {
		log_assert(0, "fatal error presnt into the deep");
	}
	HR(hr);

	frame_resources& fr = m_frames[m_frame_index];
	if(!fr.fence)
		HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fr.fence)));
	const UINT64 signalValue = fr.fence_value + 1;
	HR(m_queue->Signal(fr.fence, signalValue));
	fr.fence_value = signalValue;

	m_frame_index      = (m_frame_index + 1) % m_frames_in_flight;
	m_backbuffer_index = m_swapchain->GetCurrentBackBufferIndex();
}

void dx_context_core::wait_idle()
{
	if(!m_queue || !m_idle_fence || !m_fence_event) {
		return;
	}
	const uint64_t val = ++m_idle_value;
	HR(m_queue->Signal(m_idle_fence, val));
	HR(m_idle_fence->SetEventOnCompletion(val, m_fence_event));
	::WaitForSingleObject(m_fence_event, INFINITE);
}

D3D12_CPU_DESCRIPTOR_HANDLE dx_context_core::rtv_handle(UINT buffer_index) const
{
	D3D12_CPU_DESCRIPTOR_HANDLE h{};
	if(m_rtv_heap) {
		h = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		h.ptr += SIZE_T(buffer_index) * m_rtv_desc_size;
	}
	return h;
}

// ===== internal =====

bool dx_context_core::check_tearing_support() const
{
	BOOL           allow = FALSE;
	IDXGIFactory5* f5    = nullptr;
	if(SUCCEEDED(m_factory->QueryInterface(IID_PPV_ARGS(&f5)))) {
		if(FAILED(f5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allow, sizeof(allow))))
			allow = FALSE;
		f5->Release();
	}
	return !!allow;
}

void dx_context_core::create_device_and_queue()
{
	UINT factoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug* dbg = nullptr;
	if(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dbg)))) {
		dbg->EnableDebugLayer();
	}
	safe_release(dbg);
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	HR(CreateDXGIFactory2(factoryFlags, IID_PPV_ARGS(&m_factory)));

	IDXGIAdapter1* adapter = nullptr;
	for(UINT i = 0; m_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
			safe_release(adapter);
			continue;
		}
		if(SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			break;
		safe_release(adapter);
	}

	D3D_FEATURE_LEVEL candidates[] = {
#if defined(D3D_FEATURE_LEVEL_12_2)
	    D3D_FEATURE_LEVEL_12_2,
#endif
	    D3D_FEATURE_LEVEL_12_1,
	    D3D_FEATURE_LEVEL_12_0,
	    D3D_FEATURE_LEVEL_11_1,
	    D3D_FEATURE_LEVEL_11_0,
	};
	HRESULT hr = E_FAIL;
	for(auto fl : candidates) {
		hr = D3D12CreateDevice(adapter, fl, IID_PPV_ARGS(&m_device));
		if(SUCCEEDED(hr)) { /* fl이 실제 생성된 레벨 */
			break;
		}
	}

	HR(hr);
	safe_release(adapter);

	// HR(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));
	// safe_release(adapter);

	D3D12_COMMAND_QUEUE_DESC qd{};
	qd.Type  = D3D12_COMMAND_LIST_TYPE_DIRECT;
	qd.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	HR(m_device->CreateCommandQueue(&qd, IID_PPV_ARGS(&m_queue)));
}

void dx_context_core::create_frame_resources(UINT frames)
{
	destroy_frame_resources();
	m_frames           = new frame_resources[frames]{};
	m_frames_in_flight = frames;

	for(UINT i = 0; i < frames; ++i) {
		HR(m_device->CreateCommandAllocator(
		    D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_frames[i].allocator)));
		HR(m_device->CreateFence(
		    0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_frames[i].fence)));
		m_frames[i].fence_value = 0;
	}
}

void dx_context_core::create_swapchain(HWND hwnd, UINT w, UINT h, UINT buffer_count)
{
	m_backbuffer_count = buffer_count;

	// 기존 스왑체인 해제(있다면)
	safe_release(m_swapchain);

	DXGI_SWAP_CHAIN_DESC1 scd{};
	scd.Width       = w;
	scd.Height      = h;
	scd.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.SampleDesc  = sample1();
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = m_backbuffer_count;
	// scd.Scaling     = DXGI_SCALING_STRETCH;
	scd.Scaling    = DXGI_SCALING_NONE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.AlphaMode  = DXGI_ALPHA_MODE_IGNORE;
	scd.Flags      = m_allow_tearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	IDXGISwapChain1* sc1 = nullptr;
	HR(m_factory->CreateSwapChainForHwnd(m_queue, hwnd, &scd, nullptr, nullptr, &sc1));
	HR(sc1->QueryInterface(IID_PPV_ARGS(&m_swapchain)));
	safe_release(sc1);

	HR(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	// if(IDXGISwapChain2* sc2 = nullptr;
	//    SUCCEEDED(m_swapchain->QueryInterface(IID_PPV_ARGS(&sc2)))) {
	// 	sc2->SetMaximumFrameLatency(m_frames_in_flight);        // 보통 2~3
	// 	// WaitableObject 핸들은 DXGI 소유이므로 Close 금지
	// 	// 필요하다면 멤버로 저장해서 begin_frame 초반에 Wait 해도 됩니다.
	// 	m_frame_latency_waitable = sc2->GetFrameLatencyWaitableObject();
	// 	sc2->Release();
	// }

	// RTV heap & backbuffers
	m_rtv_desc_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
	rtvDesc.NumDescriptors = m_backbuffer_count;
	rtvDesc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvDesc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HR(m_device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&m_rtv_heap)));

	m_backbuffers = new ID3D12Resource* [m_backbuffer_count] {};
	for(UINT i = 0; i < m_backbuffer_count; ++i) {
		HR(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_backbuffers[i])));
		D3D12_CPU_DESCRIPTOR_HANDLE h = m_rtv_heap->GetCPUDescriptorHandleForHeapStart();
		h.ptr += SIZE_T(i) * m_rtv_desc_size;
		m_device->CreateRenderTargetView(m_backbuffers[i], nullptr, h);
	}

	m_backbuffer_index = m_swapchain->GetCurrentBackBufferIndex();
	m_frame_index      = 0;
}

void dx_context_core::destroy_frame_resources()
{
	if(m_frames) {
		for(UINT i = 0; i < m_frames_in_flight; ++i) {
			m_cmdlist->Reset(m_frames[i].allocator, nullptr);
			m_cmdlist->ClearState(nullptr);

			m_cmdlist->Close();
			safe_release(m_frames[i].allocator);
			safe_release(m_frames[i].fence);
		}
		delete[] m_frames;
		m_frames = nullptr;
	}
	m_frames_in_flight = 0;
}

void dx_context_core::destroy_swapchain_resources()
{
	if(m_backbuffers) {
		for(UINT i = 0; i < m_backbuffer_count; ++i) {
			safe_release(m_backbuffers[i]);
		}
		delete[] m_backbuffers;
		m_backbuffers = nullptr;
	}
	safe_release(m_rtv_heap);
	// m_swapchain은 ResizeBuffers 사용을 위해 유지
}

void dx_context_core::report_live_objects()
{
	if(!m_device || !m_factory)
		return;
	ID3D12DebugDevice* dbg_device{};
	if(SUCCEEDED(m_device->QueryInterface(IID_PPV_ARGS(&dbg_device)))) {
		// dbg_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
		dbg_device->ReportLiveDeviceObjects(D3D12_RLDO_SUMMARY | D3D12_RLDO_IGNORE_INTERNAL);
	}
	safe_release(dbg_device);

	IDXGIDebug1* dxgi_debug{};
	if(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_debug)))) {
		dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	}
	safe_release(dxgi_debug);
}

}        // namespace emt
