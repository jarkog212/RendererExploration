#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include "DX12CommonConstants.h"
#include "Utils.h"

namespace DX12
{
	static constexpr int k_allPending = -1;

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

			DescriptorHandle() = default;
			DescriptorHandle(const DescriptorHandle&) = default;
			DescriptorHandle& operator=(const DescriptorHandle& other) = default;
			DescriptorHandle(DescriptorHandle&& other) noexcept { MoveImpl(std::move(other)); };
			void operator=(DescriptorHandle&& other) noexcept { MoveImpl(std::move(other)); };

			constexpr bool IsValid() const { return _cpu.ptr != 0; }
			constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }

		private:

			void MoveImpl(DescriptorHandle&& other);

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
		const D3D12_DESCRIPTOR_HEAP_TYPE _type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		std::mutex _mutex;
		std::vector<unsigned int> _deferredResourceFreeing[DX12::CC::k_frameBuffers];
	};

	class RenderSurface
	{
	public:

		RenderSurface(const HWND window, DXGI_FORMAT format, ID3D12CommandQueue* commandQueue);
		~RenderSurface() { Release(); }
		RenderSurface(RenderSurface&& other) noexcept { MoveImpl(std::move(other)); };
		void operator=(RenderSurface&& other) noexcept { MoveImpl(std::move(other)); };

		NO_COPY(RenderSurface);

		void Resize();
		void Present() const;

		constexpr HWND GetWindow() const { return _window; }
		RECT GetWindowDimensions() const;
		constexpr unsigned int GetWidth() const { return _viewport.Width; }
		constexpr unsigned int GetHeight() const { return _viewport.Height; }
		ID3D12Resource* GetBackbuffer() const { return _renderTargetData[_currentBackBufferIndex]._resource.Get(); }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetBackbufferCPUHandle() const { return _renderTargetData[_currentBackBufferIndex]._rtvHandle._cpu;  }
		constexpr const D3D12_VIEWPORT& GetViewport() const { return _viewport; }
		constexpr const D3D12_RECT& GetScissor() const { return _scissorRect; }

	private:

		void MoveImpl(RenderSurface&& other);

		void Release();
		void Finalize(DXGI_FORMAT format);
		
		static constexpr int _swapChainBuffers = 3;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> _swapChain = nullptr;
		HWND _window = NULL;
		mutable unsigned int _currentBackBufferIndex = 0;
		D3D12_VIEWPORT _viewport = {};
		D3D12_RECT _scissorRect = {};
		unsigned int _presentFlags = 0;
		unsigned int _allowTearing = 0;

		struct RenderTargetData
		{
			Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
			DX12::DescriptorHeap::DescriptorHandle _rtvHandle = {};

			RenderTargetData() = default;
			RenderTargetData(RenderTargetData&& other) noexcept { MoveImpl(std::move(other)); }
			void operator=(RenderTargetData&& other) noexcept { MoveImpl(std::move(other)); }

			NO_COPY(RenderTargetData)

			void Release();

		private:

			void MoveImpl(RenderTargetData&& other);
		};

		RenderTargetData _renderTargetData[DX12::CC::k_frameBuffers];
	};

	class Texture
	{
	public:

		struct TextureSpecs 
		{
			ID3D12Resource* _resource = nullptr;
			ID3D12Heap1* _heap = nullptr;
			D3D12_SHADER_RESOURCE_VIEW_DESC* _srvDesc = nullptr;
			D3D12_RESOURCE_DESC* _resourceDesc = nullptr;
			D3D12_RESOURCE_ALLOCATION_INFO1 _allocationInfo = {};
			D3D12_RESOURCE_STATES _initialState = D3D12_RESOURCE_STATE_COMMON;
			D3D12_CLEAR_VALUE _clearValue = {};
		};

		Texture(TextureSpecs& specs);
		~Texture() { Release(); }
		Texture(Texture&& other) noexcept { MoveImpl(std::move(other)); };
		void operator=(Texture&& other) noexcept { MoveImpl(std::move(other)); };

		NO_COPY(Texture);

		void Release();

		ID3D12Resource* GetResource() const { return _resource; }
		DX12::DescriptorHeap::DescriptorHandle GetSRVHandle() { return _srvHandle; }

	private:

		void MoveImpl(Texture&& other);

		ID3D12Resource* _resource = nullptr; // non-owning (deferred free) so RAW
		DX12::DescriptorHeap::DescriptorHandle _srvHandle = {};
	};
}