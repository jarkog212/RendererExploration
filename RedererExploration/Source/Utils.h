#pragma once

#include <assert.h>
#include <cstdio>
#include <windows.h>

#define DISPLAY_ERROR(format, ...) {\
	wchar_t buffer[1024] = L"";\
	swprintf(buffer, 1024, format, __VA_ARGS__);\
	MessageBoxW(nullptr, buffer, L"Error", MB_ICONERROR);\
}

#define DISPLAY_RAW_OUTPUT(format, ...) {\
	char buffer[1024] = "";\
	sprintf_s(buffer, 1024, format##"\n", __VA_ARGS__);\
	OutputDebugStringA(buffer);\
}

#define HRESULT_CHECKED(instruction, format, ...) {\
	HRESULT hr = ##instruction;\
	if (FAILED(hr))\
	{\
		DISPLAY_ERROR(L"[%x] "##format, hr, __VA_ARGS__)\
		assert(false);\
	}\
}

#define HRESULT_CHECKED_FALLBACK(instruction, fallback, format, ...) {\
	HRESULT hr = ##instruction;\
	if (FAILED(hr))\
	{\
		DISPLAY_ERROR(L"[%x] "##format, hr, __VA_ARGS__)\
		##fallback;\
	}\
}

#define NO_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const  T&) = delete;

#define NO_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete;

// implement these within appropriate places
namespace UtilsImpl
{
	namespace COM
	{
		void Release(IUnknown* resource);

		void DeferredRenderThreadRelease(IUnknown* resource);
	}

	HWND CreateWindowsWindow(
		const wchar_t* title,
		const unsigned int width,
		const unsigned int height,
		WNDCLASSEXW* wc);

	float GetFps();
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

			UtilsImpl::COM::Release(resource);
			resource = nullptr;
		}

		template<typename T>
		void SafeDeferredRenderThreadRelease(T*& resource)
		{
			if (!resource)
				return;

			UtilsImpl::COM::DeferredRenderThreadRelease(resource);
			resource = nullptr;
		}
	}

	inline HWND CreateWindowsWindow(
		const wchar_t* title,
		const unsigned int width,
		const unsigned int height,
		WNDCLASSEXW* wc)
	{
		return UtilsImpl::CreateWindowsWindow(title, width, height, wc); 
	}

	inline float GetFPS() 
	{
		return UtilsImpl::GetFps();
	}
}