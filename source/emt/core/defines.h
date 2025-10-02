#pragma once

// macro defines

// engine
#define ENGINE_NAME    "EMT-D3D12"
#define MAX_SYNC_FRAME 2

// clang-format off
#define unused(x) (void)(x)
#define safe_delete(p)      do { if(p){ delete (p); (p)=nullptr; } } while(0)
#define safe_delete_array(p) do { if(p){ delete[](p); (p)=nullptr; } } while(0)
#define safe_release(x) if(x) { x->Release(); x = nullptr; }


#if defined(_DEBUG) && defined(_MSC_VER) 
#include <crtdbg.h>
#define emt_new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define check_mem_leak() _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF)
#else
#define check_mem_leak() (void)0
#define emt_new new
#endif

// clang-format on

#include "logger.h"
#include "emt_path.h"
