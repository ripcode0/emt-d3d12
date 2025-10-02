#pragma once

#include <emt/engine/scene.h>

namespace emt
{
class render_scene : public scene
{
public:
	render_scene(dx_context_core* context);

	void init() final;
	void update_frame(float dt) final;
	void render_frame() final;
	void release();

	dx_buffer* m_vtx_buffer{};
	dx_buffer* m_idx_buffer{};

	dx_shader* m_vs_shader;

	struct float3
	{
		float x, y, z;
	};

	struct vertex
	{
		float3 pos;
		float3 color;
	};
};

}        // namespace emt
