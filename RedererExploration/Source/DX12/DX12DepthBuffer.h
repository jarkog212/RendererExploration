#pragma once

#include "DX12Texture.h"

namespace DX12
{
	class DepthBuffer
	{
	public:

		DepthBuffer(DX12::TextureSpecs& specs);
		~DepthBuffer() { Release(); }
		DepthBuffer(DepthBuffer&& other) noexcept { MoveImpl(std::move(other)); }
		void operator=(DepthBuffer&& other) noexcept { MoveImpl(std::move(other)); }

		NO_COPY(DepthBuffer);
		
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() const { return _dsvHandle._cpu; }
		DX12::DescriptorHandle GetSRVHandle() const { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() const { return _texture.GetResource(); }

	private:

		void Release();
		void MoveImpl(DepthBuffer&& other);

		DX12::Texture _texture;
		DX12::DescriptorHandle _dsvHandle;
	};
}