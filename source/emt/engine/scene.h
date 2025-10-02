#pragma once

#include <emt/core/typedef.h>
#include <emt/graphics/context.h>

namespace emt
{
class scene
{
public:
	scene(dx_context_core* context);

	virtual ~scene()                 = default;
	virtual void init()              = 0;
	virtual void update_frame(float) = 0;
	virtual void render_frame()      = 0;
	virtual void release()           = 0;

	// private:
	dx_context_core* m_context{};
	dx_device*       m_device{};
};
}        // namespace emt
