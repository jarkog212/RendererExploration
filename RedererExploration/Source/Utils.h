#pragma once

#include <assert.h>

#define DISPLAY_ERROR(format, ...) {\
	wchar_t buffer[1000] = L"";\
	swprintf(buffer, 1000, format, __VA_ARGS__);\
	MessageBoxW(nullptr, buffer, L"Error", MB_ICONERROR);\
}

#define HRESULT_CHECKED(instruction, format, ...) {\
	HRESULT hr = ##instruction;\
	if (FAILED(hr))\
	{\
		DISPLAY_ERROR(L"[%u]"##format, hr, __VA_ARGS__)\
		assert(false);\
	}\
}

namespace Utils
{
	namespace COM
	{
		template<typename T> 
		void SafeRelease(T*& resource)
		{
			if (!resource)
				return;

			resource->Release();
			resource = nullptr;
		}
	}
}