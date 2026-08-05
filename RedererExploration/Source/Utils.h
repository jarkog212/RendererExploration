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

#define HRESULT_CHECKED_FALLBACK(instruction, fallback, format, ...) {\
	HRESULT hr = ##instruction;\
	if (FAILED(hr))\
	{\
		DISPLAY_ERROR(L"[%u]"##format, hr, __VA_ARGS__)\
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

		void DeferredRelease(IUnknown* resource);
	}

	HWND CreateWindowsWindow(
		const wchar_t* title,
		const unsigned int width,
		const unsigned int height,
		WNDCLASSEXW* wc);
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
		void SafeDeferredRelease(T*& resource)
		{
			if (!resource)
				return;

			UtilsImpl::COM::DeferredRelease(resource);
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
}