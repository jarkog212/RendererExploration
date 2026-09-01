#pragma once

#include <assert.h>
#include <cstdio>

#include <filesystem>

#include <windows.h>

#define DISPLAY_ERROR(format, ...) {\
	wchar_t buffer[1024] = L""; \
	swprintf(buffer, 1024, L""##format, __VA_ARGS__); \
	MessageBoxW(nullptr, buffer, L"Error", MB_ICONERROR); \
}

#define FALLBACK_ASSERT(instruction, premsg, fallback, format, ...)\
if (!(##instruction)) [[unlikely]] {\
	##premsg;\
	DISPLAY_ERROR(format, __VA_ARGS__); \
	##fallback; \
}
#define CRITICAL_ASSERT(instruction, format, ...)\
if (!(##instruction)) [[unlikely]] {\
	DISPLAY_ERROR(format, __VA_ARGS__); \
	assert(false); \
}

#define DISPLAY_RAW_OUTPUT(format, ...) {\
	char buffer[1024] = ""; \
	sprintf_s(buffer, 1024, format##"\n", __VA_ARGS__); \
	OutputDebugStringA(buffer); \
}

#define HRESULT_CHECK_CRITICAL(instruction, format, ...) {\
	HRESULT hr = ##instruction; \
	if (FAILED(hr)) [[unlikely]] \
	{\
		DISPLAY_ERROR("[%x] "##format, hr, __VA_ARGS__) \
		assert(false); \
	}\
}

#define HRESULT_CHECK_FALLBACK(instruction, premsg, fallback, format, ...) {\
	HRESULT hr = ##instruction; \
	if (FAILED(hr)) [[unlikely]] \
	{\
		##premsg; \
		DISPLAY_ERROR("[%x] "##format, hr, __VA_ARGS__); \
		##fallback; \
	}\
}

#define NO_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const  T&) = delete;

#define NO_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete;

#ifdef _DEBUG_DESTRUCTORS
#define DEBUG_DESTRUCTOR_BODY(clss, format, ...) DISPLAY_RAW_OUTPUT("["#clss"'s destructor][%p] "##format, this, __VA_ARGS__);
#define DEBUG_DESTRUCTOR(clss, format, ...) ~##clss() { DEBUG_DESTRUCTOR_BODY(clss, format, __VA_ARGS__); }
#else
#define DEBUG_DESTRUCTOR_BODY(...)
#define DEBUG_DESTRUCTOR(...)
#endif
// implement these within appropriate places
namespace UtilsImpl
{
	namespace COM
	{
		void Release(IUnknown* resource);
		void DeferredRenderThreadRelease(IUnknown* resource);
	} // COM

	namespace System
	{
		HWND CreateWindowsWindow(
			const wchar_t* title,
			const unsigned int width,
			const unsigned int height,
			WNDCLASSEXW* wc);

		bool FileExists(std::filesystem::path& path);
		unsigned long long int FileSizeOf(std::filesystem::path& path);
		void LoadFile(std::filesystem::path& path, char*& buffer, unsigned long long int& size);
		void SaveFile(std::filesystem::path& path, const char* buffer, const unsigned long long int size);
		void FileDelete(std::filesystem::path& path);
	} // System

	float GetFps();
} // UtilsImpl

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
	} // COM

	namespace System
	{
		inline HWND CreateWindowsWindow(
			const wchar_t* title,
			const unsigned int width,
			const unsigned int height,
			WNDCLASSEXW* wc)
		{
			return UtilsImpl::System::CreateWindowsWindow(title, width, height, wc);
		}

		inline bool FileExists(std::filesystem::path& path)
		{
			return UtilsImpl::System::FileExists(path);
		}

		inline bool TryDeleteFile(std::filesystem::path& path) 
		{
			if (!Utils::System::FileExists(path)) [[unlikely]]
				return false;

			UtilsImpl::System::FileDelete(path);
			return true;
		}

		inline unsigned long long int FileSizeOf(std::filesystem::path& path)
		{
			return UtilsImpl::System::FileSizeOf(path);
		}

		inline void LoadFile(std::filesystem::path& path, char*& buffer, unsigned long long int& size)
		{
			UtilsImpl::System::LoadFile(path, buffer, size);
		}

		inline void SaveFile(std::filesystem::path& path, const char* buffer, const unsigned long long int size)
		{
			UtilsImpl::System::SaveFile(path, buffer, size);
		}
	} // System

	inline float GetFPS() 
	{
		return UtilsImpl::GetFps();
	}

	template<unsigned long long int alignement>
	inline unsigned long long int AlignSizeUp(unsigned long long int size)
	{
		constexpr unsigned long long int mask(alignement - 1);
		CRITICAL_ASSERT(!(alignement & mask)); // alignement is power of 2
		return (size + mask) & ~mask;
	}

	template<unsigned long long int alignement>
	inline unsigned long long int AlignSizeDown(unsigned long long int size)
	{
		constexpr unsigned long long int mask(alignement - 1);
		CRITICAL_ASSERT(!(alignement & mask)); // alignement is power of 2
		return size & ~mask;
	}
} // Utils