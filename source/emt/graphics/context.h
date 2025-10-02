#pragma once

#include <emt/core/typedef.h>

namespace emt
{
class context
{
public:
	context(uint32_t cx, uint32_t cy, void* hwnd) :
	    m_cx(cx),
	    m_cy(cy),
	    m_handle((HWND)hwnd) {};

	virtual ~context() {};

	virtual void resize_frame(uint32_t cx, uint32_t cy) = 0;

	virtual void begin_frame()                 = 0;
	virtual void end_frame(bool vsync = false) = 0;

	HWND window_handle() const
	{
		return m_handle;
	}
	uint32_t width() const
	{
		return m_cx;
	}
	uint32_t height() const
	{
		return m_cy;
	}

private:
	uint32_t m_cx{};
	uint32_t m_cy{};
	HWND     m_handle{nullptr};
};
}        // namespace emt
