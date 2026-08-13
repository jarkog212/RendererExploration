#pragma once

#include "DX12Texture.h"

namespace DX12
{
	class DepthBufferTexture
	{
	public:

		static DepthBufferTexture Create(DX12::TextureSpecs& specs);

		DepthBufferTexture(DepthBufferTexture&& other) noexcept;
		DepthBufferTexture& operator=(DepthBufferTexture&& other) noexcept;

		NO_COPY(DepthBufferTexture);
		
		D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpuHandle() const { return _dsvHandle._cpu; }
		DX12::DescriptorHandleView GetSRVHandle() const { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() const { return _texture.GetResource(); }

	private:

		DepthBufferTexture(DX12::TextureSpecs& specs, DXGI_FORMAT depthStencilFormat);
		
		DX12::Texture _texture;
		DX12::DescriptorHandle _dsvHandle;
	};
}