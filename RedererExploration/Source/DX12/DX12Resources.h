#pragma once

#include "DX12Renderer.h"
#include <memory>
#include <mutex>
#include <vector>

namespace DX12
{
	class DX12DescriptorHeap
	{
	public:

		DX12DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type) : _type(type) {};

		NO_COPY(DX12DescriptorHeap);
		NO_MOVE(DX12DescriptorHeap);

		struct DX12DescriptorHandle
		{
			D3D12_CPU_DESCRIPTOR_HANDLE _cpu{ 0 };
			D3D12_GPU_DESCRIPTOR_HANDLE _gpu{ 0 };

			constexpr bool IsValid() const { return _cpu.ptr != 0; }
			constexpr bool IsShaderVisible() const { return _gpu.ptr != 0; }

#ifdef _DEBUG
		private:
			friend class DX12DescriptorHeap;

			DX12DescriptorHeap* _owner = nullptr;
			unsigned int _index = 0;
#endif
		};

		bool Setup(unsigned int capacity, bool isShaderVisible);
		void Release();

		[[nodiscard]] DX12DescriptorHandle Allocate();
		void Free(DX12DescriptorHandle& handle);

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return _type; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPtr() const { return _mainHandles._cpu; }
		constexpr D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPtr() const { return _mainHandles._gpu; }
		ID3D12DescriptorHeap* GetHeap() const { return _heap.Get(); }
		constexpr unsigned int GetCapacity() const { return _capacity; }
		constexpr unsigned int GetSize() const { return _size; }
		constexpr unsigned int GetDescriptorSize() const { return _descriptorSize; }
		
		constexpr bool IsShaderVisible() const { return _mainHandles.IsShaderVisible(); }

	private:

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _heap = nullptr;
		DX12DescriptorHandle _mainHandles;
		std::unique_ptr<unsigned int[]> _freeHandles = nullptr;
		unsigned int _capacity = 0;
		unsigned int _size = 0;
		unsigned int _descriptorSize = 0;
		const D3D12_DESCRIPTOR_HEAP_TYPE _type;
		std::mutex _mutex;
		std::vector<unsigned int> _deferredResourceFreeing[DX12::k_frameBuffers];
	};
}