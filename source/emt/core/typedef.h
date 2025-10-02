#pragma once

// newmeric
#include <stdint.h>
#include <span>

// clang-format off
#ifndef _INC_WINDOWS
#ifndef CALLBACK
    #define CALLBACK __stdcall
#endif
#ifndef WINAPI
    #define WINAPI __stdcall
#endif
#ifndef APIENTRY
    #define APIENTRY WINAPI
#endif

typedef unsigned int    UINT;
typedef unsigned long   DWORD; // Windows(LLP64) 32-bit
typedef int             BOOL;
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef float           FLOAT;
typedef long            LONG;

typedef void            *LPVOID;
typedef const char      *LPCSTR;
typedef char            *LPSTR;

#if defined(_WIN64)
    #if defined(_MSC_VER)
        typedef __int64 INT_PTR;
        typedef unsigned __int64 UINT_PTR;
        typedef __int64 LONG_PTR;
        typedef unsigned __int64 ULONG_PTR;
    #else
        typedef long long INT_PTR;
        typedef unsigned long long UINT_PTR;
        typedef long long LONG_PTR;
        typedef unsigned long long ULONG_PTR;
    #endif
#else
    typedef int INT_PTR;
    typedef unsigned int UINT_PTR;
    typedef long LONG_PTR;
    typedef unsigned long ULONG_PTR;
#endif

typedef ULONG_PTR DWORD_PTR, *PDWORD_PTR;
typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;
typedef long HRESULT;

typedef void *HANDLE;

struct HWND__;
typedef HWND__ *HWND;
struct HINSTANCE__;
typedef HINSTANCE__ *HINSTANCE;
struct HDC__;
typedef HDC__ *HDC;
struct HBRUSH__;
typedef HBRUSH__ *HBRUSH;
struct HGLRC__;
typedef HGLRC__ *HGLRC;
struct HFONT__;
typedef HFONT__ *HFONT;
struct HPEN__;
typedef HPEN__ *HPEN;
#endif

// clang-format on

// Vulkan
typedef struct VkBuffer_T*       VkBuffer;
typedef struct VkDeviceMemory_T* VkDeviceMemory;
typedef struct VkDevice_T*       VkDevice;
typedef uint64_t                 VkDeviceSize;

// emt Forward Declare
namespace emt
{
// frontend
class context;
class scene;

// backend
// vulkan
class vk_context;
class vk_instance;
class vk_surface;
class vk_device;
class vk_swapchain;
class vk_sync;

// struct
struct vk_buffer;

// experimental
class vk_context_ext;

// D3D12
class dx_context_core;
class dx_device;
struct dx_buffer;
struct dx_shader;

// engine enum
enum class render_target_type {
	color,
	depth,
	stencil,
	depth_stencil
};

enum class load_operator {
	load,
	clear,
	discard
};

enum class store_operator {
	store,
	discard,
	no_access
};

enum class buffer_type : uint32_t {
	raw,
	vertex,
	index,
	uniform
};

struct buffer_create_info
{
	buffer_type type;
	const void* data;
	uint32_t    size;
};

// shader

enum class shader_stage : uint32_t {
	vertex,
	pixel,
	compute,
	geometry,
	hull,
	mesh,
	domain
};

struct shader_create_info
{
	shader_stage           stage;
	const char*            filename;
	const char*            entry;
	std::span<const char*> includes;
};

}        // namespace emt

#include "defines.h"