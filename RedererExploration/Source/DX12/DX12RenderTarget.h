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

		void Release();

		constexpr unsigned int GetMipCount() { return _mipCount; }
		constexpr D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCPUHandle(unsigned int mip);
		DX12::DescriptorHandle GetSRVHandle() { return _texture.GetSRVHandle(); }
		ID3D12Resource* GetResource() { return _texture.GetResource(); }

	private:

		void MoveImpl(RenderTargetTexture&& other);

		DX12::Texture _texture;
		DX12::DescriptorHandle _rtvHandles[DX12::Texture::_k_maxMips];
		unsigned int _mipCount = 0;
	};
}