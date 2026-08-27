#pragma once

#include "DX12Core.h"
#include "DX12CommonConstants.h"
#include "DX12Resources.h"
#include "Utils.h" 

#include <memory>
#include <mutex>
#include <vector>

namespace DX12
{
	class DescriptorHeap
	{
	public:

		DescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE type,
			unsigned int capacity,
			bool isShaderVisible,
			D3D12_DESCRIPTOR_RANGE_TYPE rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV);

		~DescriptorHeap() { DEBUG_DESTRUCTOR_BODY(DescriptorHeap) Release(); };

		NO_COPY(DescriptorHeap);
		NO_MOVE(DescriptorHeap);

		void Release();

		[[nodiscard]] DX12::DescriptorHandle Allocate();
		void Free(DX12::DescriptorHandle& handle);
		void ProcessPendingResourceFreeing(const int currentFrame = DX12::CC::k_forAllFrames);

		constexpr D3D12_DESCRIPTOR_HEAP_TYPE GetType() const { return _type; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetCpuPtr() const { return _mainHandles._cpu; }
		constexpr D3D12_GPU_DESCRIPTOR_HANDLE GetGpuPtr() const { return _mainHandles._gpu; }
		ID3D12DescriptorHeap* GetHeap() const { return _heap; }
		constexpr unsigned int GetCapacity() const { return _capacity; }
		constexpr unsigned int GetSize() const { return _size; }
		constexpr unsigned int GetDescriptorSize() const { return _descriptorSize; }

		constexpr bool IsShaderVisible() const { return _mainHandles.IsShaderVisible(); }
		DX12::DescriptorRange AsDescriptorRange() const;

	private:

		ID3D12DescriptorHeap* _heap = nullptr; // non-tied lifetime (deferred free), hence raw
		DX12::DescriptorHandle _mainHandles;
		std::unique_ptr<unsigned int[]> _freeHandles = nullptr;
		unsigned int _capacity = 0;
		unsigned int _size = 0;
		unsigned int _descriptorSize = 0;
		const D3D12_DESCRIPTOR_HEAP_TYPE _type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		const D3D12_DESCRIPTOR_RANGE_TYPE _rangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		std::mutex _mutex;
		std::vector<unsigned int> _deferredResourceFreeing[DX12::CC::k_rendererWorkerFrameBuffers];
	};
}