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

		Texture(TextureSpecs& specs);
		~Texture() { Release(); }
		Texture(Texture&& other) noexcept { MoveImpl(std::move(other)); };
		void operator=(Texture&& other) noexcept { MoveImpl(std::move(other)); };

		NO_COPY(Texture);

		void Release();

		ID3D12Resource* GetResource() const { return _resource; }
		DX12::DescriptorHandle GetSRVHandle() { return _srvHandle; }

	private:

		void MoveImpl(Texture&& other);

		ID3D12Resource* _resource = nullptr; // non-owning (deferred free) so RAW
		DX12::DescriptorHandle _srvHandle = {};
	};
}