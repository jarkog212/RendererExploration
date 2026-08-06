#pragma once

#include "DX12Texture.h"

namespace DX12
{
	class RenderTargetTexture
	{
	public:

		RenderTargetTexture(DX12::TextureSpecs& spec);
		~RenderTargetTexture() { Release(); };
		RenderTargetTexture(RenderTargetTexture&& other) noexcept { MoveImpl(std::move(other)); }
		void operator=(RenderTargetTexture&& other) noexcept { MoveImpl(std::move(other)); }
		
		NO_COPY(RenderTargetTexture);

		constexpr unsigned int GetMipCount() const { return _mipCount; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(unsigned int mip) const;
		DX12::DescriptorHandle GetSRVHandle() const { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() const { return _texture.GetResource(); }

	private:
		
		void Release();
		void MoveImpl(RenderTargetTexture&& other);

		DX12::Texture _texture;
		DX12::DescriptorHandle _rtvHandles[DX12::Texture::_k_maxMips];
		unsigned int _mipCount = 0;
	};
}