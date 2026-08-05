#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <dxgi1_6.h>
#include <d3d12.h>
#include <wrl.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")

#include "Utils.h"

namespace DX12
{
	static constexpr unsigned int k_frameBuffers = 3;
	static constexpr int k_allPending = -1;
	static constexpr DXGI_FORMAT k_defaultRenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	class DescriptorHeap
	{
	public:

		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int capacity, bool isShaderVisible);
		~DescriptorHeap() { Release(); };

		NO_COPY(DescriptorHeap);
		NO_MOVE(DescriptorHeap);

		struct DescriptorHandle
		{
			D3D12_CPU_DESCRIPTOR_HANDLE _cpu{ 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE _gpu{ 0 };

			constexpr bool IsValid() const { return _cpu.ptr != 0; }
			constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }

#ifdef _DEBUG
		private:
			friend class DescriptorHeap;

			DescriptorHeap* _owner = nullptr;
			unsigned int _index = 0;
#endif
		};

		void Release();

		[[nodiscard]] DescriptorHandle Allocate();
		void Free(DescriptorHandle& handle);
		void ProcessPendingResourceFreeing(const int currentFrame = k_allPending);

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return _type; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPtr() const { return _mainHandles._cpu; }
		constexpr D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPtr() const { return _mainHandles._gpu; }
		ID3D12DescriptorHeap* GetHeap() const { return _heap; }
		constexpr unsigned int GetCapacity() const { return _capacity; }
		constexpr unsigned int GetSize() const { return _size; }
		constexpr unsigned int GetDescriptorSize() const { return _descriptorSize; }
		
		constexpr bool IsShaderVisible() const { return _mainHandles.IsShaderVisible(); }

	private:

		ID3D12DescriptorHeap* _heap = nullptr; // non-tied lifetime (deferred free), hence raw
		DescriptorHandle _mainHandles;
		std::unique_ptr<unsigned int[]> _freeHandles = nullptr;
		unsigned int _capacity = 0;
		unsigned int _size = 0;
		unsigned int _descriptorSize = 0;
		const D3D12_DESCRIPTOR_HEAP_TYPE _type;
		std::mutex _mutex;
		std::vector<unsigned int> _deferredResourceFreeing[DX12::k_frameBuffers];
	};

	class RenderSurface
	{
	
	public:

		RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue* commandQueue);
		~RenderSurface() { Release(); }

		NO_COPY(RenderSurface)

		void Resize();
		void Present() const;

		constexpr HWND GetWindow() const { return _window; }
		RECT GetWindowDimensions() const;
		constexpr unsigned int GetWidth() const { return _viewport.Width; }
		constexpr unsigned int GetHeight() const { return _viewport.Height; }
		ID3D12Resource* GetBackbuffer() const { return _renderTargetData[_currentBackBufferIndex]._resource.Get(); }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetBackbufferCPUHandle() const { return _renderTargetData[_currentBackBufferIndex]._handle._cpu;  }
		constexpr const D3D12_VIEWPORT& GetViewport() const { return _viewport; }
		constexpr const D3D12_RECT& GetScissor() const { return _scissorRect; }

	private:

		void Release();
		void Finalize(DXGI_FORMAT format);
		
		Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
		HWND _window = NULL;
		mutable unsigned int _currentBackBufferIndex = 0;
		D3D12_VIEWPORT _viewport;
		D3D12_RECT _scissorRect;

		struct RenderTargetData
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
			DX12::DescriptorHeap::DescriptorHandle _handle;

			void Release();
		};

		RenderTargetData _renderTargetData[DX12::k_frameBuffers];
	};
}