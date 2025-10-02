#include "dx_shader.h"
#include <filesystem>

namespace emt
{
void dx_shader_cache::initialize()
{
	if(inited)
		return;
	HR(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)));
	HR(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
	m_utils->CreateDefaultIncludeHandler(&m_includes);
	inited = true;
}

void dx_shader_cache::deinitialize()
{
	safe_release(m_includes);
	safe_release(m_compiler);
	safe_release(m_utils);
	inited = false;
}

void utf8_to_utf16(const char* str, wchar_t* wstr, size_t cap)
{
	size_t len = std::strlen(str) + 1;
	if(len > cap)
		len = cap;
	// std::mbstowcs(wstr, str, len);
	size_t converted = 0;
	mbstowcs_s(&converted, wstr, len, str, _TRUNCATE);
}

const wchar_t* get_stage_str(shader_stage stage)
{
	switch(stage) {
		case shader_stage::vertex: return L"vs_6_7";
		case shader_stage::pixel: return L"ps_6_7";
		case shader_stage::geometry: return L"gs_6_7";
		case shader_stage::hull: return L"hs_6_7";
		case shader_stage::domain: return L"ds_6_7";
		case shader_stage::compute: return L"cs_6_7";
		default: return L"vs_6_7";
	}
}

void dx_shader_cache::compile_from_file(shader_stage stage,
                                        const char*  file,
                                        const char*  entry,
                                        dx_shader**  pp_shader)
{
	std::string filename(EMT_DATA_DIR);
	filename.append(file);
	if(!std::filesystem::exists(filename.c_str())) {
		log_error("failed to find file : %s", filename.c_str());
	}

	wchar_t wide_file[128]{};
	wchar_t wide_entry[32]{};

	utf8_to_utf16(filename.c_str(), wide_file, std::size(wide_file));
	utf8_to_utf16(entry, wide_entry, std::size(wide_entry));

	IDxcBlobEncoding* encode{};

	HR(m_utils->LoadFile(wide_file, nullptr, &encode));

	DxcBuffer buffer{};
	buffer.Ptr      = encode->GetBufferPointer();
	buffer.Size     = encode->GetBufferSize();
	buffer.Encoding = DXC_CP_UTF8;

	// includer
	// IDxcIncludeHandler* includer{};
	// HR(m_utils->CreateDefaultIncludeHandler(&includer));

	// args
	const wchar_t* args[] = {
	    L"-E",
	    wide_entry,
	    L"-T",
	    get_stage_str(stage),
	};

	IDxcResult* res{};
	m_compiler->Compile(&buffer, args, std::size(args), m_includes, IID_PPV_ARGS(&res));

	IDxcBlobUtf8* err{};
	if(SUCCEEDED(res->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&err), nullptr)) && err && err->GetStringLength() > 0) {
		log_error("[dxc] %s", err->GetStringPointer());
	}
	safe_release(err);

	HRESULT status{};
	HR(res->GetStatus(&status));
	if(FAILED(status)) {
		log_error("[dxc] compile failed : %s (%s:%s)", file, entry, "DXC");
	}

	IDxcBlob* p_dxil_blob{};
	res->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&p_dxil_blob), nullptr);

	safe_release(res);
	safe_release(encode);

	dx_shader* p_shader = emt_new dx_shader;
	p_shader->blob      = p_dxil_blob;

	*pp_shader = p_shader;
	log_debug("[dxc] %s [%s] [%ls] (%zu bytes)",
	          file, entry, get_stage_str(stage),
	          static_cast<size_t>(p_dxil_blob->GetBufferSize()));
}

}        // namespace emt
