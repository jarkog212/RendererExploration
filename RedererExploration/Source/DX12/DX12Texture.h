#pragma once

#include "DX12Resources.h"

namespace DX12
{
	class Texture
	{
	public:

		struct Specs
		{
			ID3D12Resource* _resource = nullptr;
			ID3D12Heap1* _heap = nullptr;
			D3D12_SHADER_RESOURCE_VIEW_DESC* _srvDesc = nullptr;
			D3D12_RESOURCE_DESC* _resourceDesc = nullptr;
			D3D12_RESOURCE_ALLOCATION_INFO1 _allocationInfo = {};
			D3D12_RESOURCE_STATES _initialState = D3D12_RESOURCE_STATE_COMMON;
			D3D12_CLEAR_VALUE _clearValue = {};
		};

		struct LoadingRequest : public Specs
		{
			const wchar_t* _filename = L"";
		};

		static constexpr unsigned int _k_maxMips = 14;

		Texture(Specs& specs);
		Texture(LoadingRequest& request);
		~Texture();
		Texture(Texture&& other) noexcept;
		Texture& operator=(Texture&& other) noexcept;

		NO_COPY(Texture);

		ID3D12Resource* GetResource() const { return _resource; }
		DX12::DescriptorHandleView GetSRVHandle() const { return _srvHandle.CreateView(); }

	private:

		ID3D12Resource* _resource = nullptr; // non-owning (deferred free) so RAW
		DX12::DescriptorHandle _srvHandle = {};
	};
}

namespace DX12::TextureSpace
{
	std::shared_ptr<DX12::Texture> GetOrLoad(const wchar_t* name);
	void Release();
}