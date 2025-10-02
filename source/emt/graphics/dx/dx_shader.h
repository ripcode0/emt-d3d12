#pragma once

#include "dx_config.h"

namespace emt
{
struct dx_shader
{
	IDxcBlob*             blob{};
	D3D12_SHADER_BYTECODE byte_code() const noexcept
	{
		D3D12_SHADER_BYTECODE out{};
		out.BytecodeLength  = blob->GetBufferSize();
		out.pShaderBytecode = blob->GetBufferPointer();
		return out;
	}
	~dx_shader() { release(); }
	void release()
	{
		safe_release(blob);
	}
};

struct dx_shader_cache
{
	static void initialize();
	static void deinitialize();

	static void compile_from_file(
	    shader_stage stage,
	    const char*  file,
	    const char*  entry,
	    dx_shader**  pp_shader);

private:
	inline static bool                inited = false;
	inline static IDxcUtils*          m_utils{};
	inline static IDxcCompiler3*      m_compiler{};
	inline static IDxcIncludeHandler* m_includes{};
};

}        // namespace emt
