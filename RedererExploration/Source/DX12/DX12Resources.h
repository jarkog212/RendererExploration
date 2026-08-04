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

	class DescriptorHeap
	{
	public:

		DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type(type) {};
		~DescriptorHeap();

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

		bool Setup(unsigned int capacity, bool isShaderVisible);
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

		ID3D12DescriptorHeap* _heap = nullptr; // Release needs to be explicitly invoked, implicit destructor wont manage this.
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

		static constexpr int k_invalidId = -1;

		RenderSurface() = default;
		RenderSurface(const int id) : _id(id) {};

		void Resize() {};
		void Render() {};

		constexpr int GetId() const { return _id; }
		int GetWidth() const {};
		int GetHeight() const {};

		constexpr bool IsValid() const { return _id != k_invalidId; }
	
	private:

		int _id = k_invalidId;
	
	};
}