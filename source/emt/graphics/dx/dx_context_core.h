#pragma once

#include <emt/graphics/context.h>
#include "dx_config.h"
#include "dx_device.h"

namespace emt
{

class dx_context_core : public context
{
public:
	dx_context_core(uint32_t cx, uint32_t cy, HWND hwnd);
	~dx_context_core();

	// 리사이즈(스왑체인 재생성)
	void resize_frame(uint32_t cx, uint32_t cy) override;

	// 프레임 API
	void begin_frame() override;        // backbuffer index 반환
	void end_frame(bool vsync) override;        // tearing 지원 자동 적용

	// 대기
	void wait_idle();

	// getters
	IDXGIFactory4*             factory() const { return m_factory; }
	ID3D12Device*              device() const { return m_device; }
	ID3D12CommandQueue*        queue() const { return m_queue; }
	ID3D12GraphicsCommandList* get_current_command_list() const { return m_cmdlist; }
	IDXGISwapChain3*           swapchain() const { return m_swapchain; }

	ID3D12DescriptorHeap*       rtv_heap() const { return m_rtv_heap; }
	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle(UINT buffer_index) const;
	UINT                        rtv_descriptor_size() const { return m_rtv_desc_size; }
	UINT                        backbuffer_count() const { return m_backbuffer_count; }
	UINT                        frame_index() const { return m_frame_index; }
	uint32_t                    backbuffer_index() const { return m_backbuffer_index; }
	bool                        tearing_supported() const { return m_allow_tearing; }
	const dx_device*            graphic_device() const { return &m_graphic_device; }
	dx_device*                  graphic_device() { return &m_graphic_device; }

private:
	struct frame_resources
	{
		ID3D12CommandAllocator* allocator   = nullptr;
		ID3D12Fence*            fence       = nullptr;
		uint64_t                fence_value = 0;
	};
	dx_device m_graphic_device{};
	HANDLE    m_frame_latency_waitable = nullptr;

private:
	// 내부 유틸
	void initialize_core();
	void release();
	void create_swapchain(HWND hwnd, UINT w, UINT h, UINT buffer_count);
	bool check_tearing_support() const;
	void create_device_and_queue();
	void create_frame_resources(UINT frames);

	void destroy_frame_resources();
	void destroy_swapchain_resources();
	void report_live_objects();

private:
	// core
	IDXGIFactory4*      m_factory = nullptr;
	ID3D12Device*       m_device  = nullptr;
	ID3D12CommandQueue* m_queue   = nullptr;

	// command recording
	ID3D12GraphicsCommandList* m_cmdlist          = nullptr;
	frame_resources*           m_frames           = nullptr;
	uint32_t                   m_frames_in_flight = 0;
	uint32_t                   m_frame_index      = 0;
	uint32_t                   m_backbuffer_index = 0;

	// sync
	HANDLE       m_fence_event = nullptr;
	ID3D12Fence* m_idle_fence  = nullptr;
	uint64_t     m_idle_value  = 0;

	// swapchain & rtv
	IDXGISwapChain3*      m_swapchain        = nullptr;
	ID3D12Resource**      m_backbuffers      = nullptr;
	ID3D12DescriptorHeap* m_rtv_heap         = nullptr;
	uint32_t              m_rtv_desc_size    = 0;
	uint32_t              m_backbuffer_count = 0;
	uint32_t              m_width            = 0;
	uint32_t              m_height           = 0;

	// features
	bool m_allow_tearing = false;
};

}        // namespace emt
