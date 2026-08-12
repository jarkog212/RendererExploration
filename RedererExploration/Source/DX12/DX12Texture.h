#pragma once

#include "DX12Resources.h"

namespace DX12
{
	struct TextureSpecs
	{
		ID3D12Resource* _resource = nullptr;
		ID3D12Heap1* _heap = nullptr;
		D3D12_SHADER_RESOURCE_VIEW_DESC* _srvDesc = nullptr;
		D3D12_RESOURCE_DESC* _resourceDesc = nullptr;
		D3D12_RESOURCE_ALLOCATION_INFO1 _allocationInfo = {};
		D3D12_RESOURCE_STATES _initialState = D3D12_RESOURCE_STATE_COMMON;
		D3D12_CLEAR_VALUE _clearValue = {};
	};

	class Texture
	{
	public:

		static constexpr unsigned int _k_maxMips = 14;

		Texture(TextureSpecs& specs);
		~Texture() { Release(); }
		Texture(Texture&& other) noexcept;
		Texture& operator=(Texture&& other) noexcept;

		NO_COPY(Texture);

		ID3D12Resource* GetResource() const { return _resource; }
		 DescriptorHandle GetSRVHandle() const { return _srvHandle; }

	private:

		void Release();

		ID3D12Resource* _resource = nullptr; // non-owning (deferred free) so RAW
		DX12::DescriptorHandle _srvHandle = {};
	};
}