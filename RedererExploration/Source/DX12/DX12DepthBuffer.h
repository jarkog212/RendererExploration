#pragma once

#include "DX12Texture.h"

namespace DX12
{
	class DepthBuffer
	{
	public:

		static DepthBuffer Create(DX12::TextureSpecs& specs);

		~DepthBuffer() { Release(); }
		DepthBuffer(DepthBuffer&& other) noexcept;
		DepthBuffer& operator=(DepthBuffer&& other) noexcept;

		NO_COPY(DepthBuffer);
		
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() const { return _dsvHandle._cpu; }
		DX12::DescriptorHandle GetSRVHandle() const { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() const { return _texture.GetResource(); }

	private:

		DepthBuffer(DX12::TextureSpecs& specs, DXGI_FORMAT stencilFormat);
		void Release();
		
		DX12::Texture _texture;
		DX12::DescriptorHandle _dsvHandle;
	};
}