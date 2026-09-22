#pragma once

#include "DX12Texture.h"

namespace DX12
{
	class RenderTargetTexture
	{
	public:

		static RenderTargetTexture Create(DX12::Texture::Specs& specs);

		RenderTargetTexture(RenderTargetTexture&& other) noexcept;
		DEBUG_DESTRUCTOR(RenderTargetTexture)
		RenderTargetTexture& operator=(RenderTargetTexture&& other) noexcept;
		
		NO_COPY(RenderTargetTexture);

		constexpr unsigned int GetMipCount() const { return _mipCount; }
		D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(unsigned int mip) const;
		DX12::DescriptorHandleView GetSRVHandle() const { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() const { return _texture.GetResource(); }

	private:

		RenderTargetTexture(DX12::Texture::Specs& spec);

		DX12::Texture _texture;
		DX12::DescriptorHandle _rtvHandles[DX12::Texture::_k_maxMips];
		unsigned int _mipCount = 0;
	};
}