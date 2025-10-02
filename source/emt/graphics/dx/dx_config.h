#pragma once

#include <emt/core/typedef.h>
#include <emt/core/config.h>

#include "directx/d3d12.h"
#include "directx/d3dx12.h"
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <stdio.h>
#include <iostream>
// DXC
#include <dxcapi.h>

#define HR(x) __hr(x, __FILE__, __LINE__)

inline void __hr(HRESULT hr, LPCSTR filename, int line)
{
	if(SUCCEEDED(hr))
		return;

	// 1) 시스템 에러 문자열 가져오기 (LocalAlloc으로 할당됨)
	LPSTR sysMsg = nullptr;
	DWORD fmFlags =
	    FORMAT_MESSAGE_ALLOCATE_BUFFER |
	    FORMAT_MESSAGE_FROM_SYSTEM |
	    FORMAT_MESSAGE_IGNORE_INSERTS |
	    FORMAT_MESSAGE_MAX_WIDTH_MASK;        // 줄바꿈 억제

	DWORD len = ::FormatMessageA(
	    fmFlags,
	    nullptr,
	    static_cast<DWORD>(hr),
	    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	    reinterpret_cast<LPSTR>(&sysMsg),
	    0,
	    nullptr);

	// 2) 파일명만 추출
	const char* base = filename ? filename : "";
	if(const char* p = std::strrchr(base, '\\'))
		base = p + 1;
	if(const char* p = std::strrchr(base, '/'))
		base = p + 1;        // 혹시 모를 POSIX 경로도 처리

	// 3) 최종 메시지 구성
	char        totalBuffer[1024];
	const char* sysText = (len && sysMsg) ? sysMsg : "Unknown error";
	_snprintf_s(totalBuffer, sizeof(totalBuffer), _TRUNCATE,
	            "HRESULT failed: 0x%08X\n%s\nfile: %s\nline: %d\n",
	            static_cast<unsigned>(hr),
	            sysText,
	            base,
	            line);

	// 4) 시스템 메시지 해제
	if(sysMsg)
		::LocalFree(sysMsg);

	// 5) 보여주기
	::MessageBoxA(nullptr, totalBuffer, "HRESULT Error", MB_OK | MB_ICONERROR);

#if defined(_MSC_VER)
// 디버그에선 중단점(있으면)
#	if defined(_DEBUG)
	__debugbreak();
#	endif
#endif
	// 릴리스 기본 동작: 프로세스 종료(취향에 따라 변경 가능)
	std::abort();        // 또는 ExitProcess(1);
}