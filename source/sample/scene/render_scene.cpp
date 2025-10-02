#include "render_scene.h"
#include <emt/graphics/dx/dx_context_core.h>
#include <emt/graphics/dx/dx_device.h>
#include <emt/graphics/dx/dx_buffer.h>
#include <emt/graphics/dx/dx_shader.h>

namespace emt
{
render_scene::render_scene(dx_context_core* context) :
    scene(context)
{
}

void render_scene::init()
{
	log_info("render scene initializing ...");
	vertex vertices[] = {
	    {{1, 1, 0}, {1, 0, 0}},
	    {{1, 1, 0}, {1, 0, 0}},
	    {{1, 1, 0}, {1, 0, 0}},
	};

	buffer_create_info info{};
	info.data = vertices;
	info.size = std::size(vertices) * sizeof(vertex);
	info.type = buffer_type::vertex;

	m_device->create_buffer(&info, &m_vtx_buffer);

	uint32_t indices[] = {0, 1, 2};
	info.data          = indices;
	info.size          = std::size(indices) * sizeof(uint32_t);
	info.type          = buffer_type::index;

	m_device->create_buffer(&info, &m_idx_buffer);

	shader_create_info shader_info{};
	shader_info.entry    = "main";
	shader_info.stage    = shader_stage::vertex;
	shader_info.filename = "shader/vertex.hlsl";
	shader_info.includes = {};

	dx_shader_cache::compile_from_file(shader_stage::vertex, "shader/vertex.hlsl", "main", &m_vs_shader);
}

void render_scene::update_frame(float dt)
{
	unused(dt);
}

void render_scene::render_frame()
{
	auto     m_cmd              = m_context->get_current_command_list();
	uint32_t m_image_index      = m_context->backbuffer_index();
	auto     render_target_view = m_context->rtv_handle(m_image_index);

	const float color[] = {0.6, 0.6, 0.1, 1.f};
	int         x       = 150;
	int         y       = 30;
	D3D12_RECT  rc      = {x, y, ((LONG)m_context->width() / 2) + x, ((LONG)m_context->height() / 2) + y};
	m_cmd->ClearRenderTargetView(render_target_view, color, 1, &rc);
	m_cmd->OMSetRenderTargets(1, &render_target_view, TRUE, nullptr);
}

void render_scene::release()
{
	safe_delete(m_vtx_buffer);
	safe_delete(m_idx_buffer);
}

}        // namespace emt
