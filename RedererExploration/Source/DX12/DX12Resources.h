#pragma once

#include "DX12CommonConstants.h"
#include "DX12Core.h"
#include "Utils.h"
#include <utility>

namespace DX12
{
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

	struct RenderTargetData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> _resource = nullptr;
		DX12::DescriptorHandle _rtvHandle = {};

		RenderTargetData() = default;
		RenderTargetData(RenderTargetData&& other) noexcept { MoveImpl(std::move(other)); }
		void operator=(RenderTargetData&& other) noexcept { MoveImpl(std::move(other)); }

		NO_COPY(RenderTargetData)

		void Release();

	private:

		void MoveImpl(RenderTargetData&& other);
	};
}