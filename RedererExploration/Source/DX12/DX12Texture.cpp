#include "DX12Texture.h"
#include "DX12CommonDescriptors.h"
#include "DX12DescriptorHeap.h"

DX12::Texture::Texture(TextureSpecs& specs)
{
	CRITICAL_ASSERT(specs._resourceDesc, "Cannot create texture without valid resource descriptor.");
	CRITICAL_ASSERT(specs._srvDesc, "Cannot create texture without valid SRV descriptor.");

	ID3D12Device8* device = DX12::GetRendererDevice();
	D3D12_CLEAR_VALUE* clearValue = nullptr;
	if (specs._resourceDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
		|| specs._resourceDesc->Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		clearValue = &specs._clearValue;
	}

	if (specs._resource)
	{
		_resource = specs._resource;
		specs._resource = nullptr;
	}
	else if (specs._heap)
	{
		HRESULT_CHECK_FALLBACK(
			device->CreatePlacedResource(
				specs._heap,
				specs._allocationInfo.Offset,
				specs._resourceDesc,
				specs._initialState,
				clearValue,
				IID_PPV_ARGS(&_resource)),,
			assert(false),
			"Could not create a placed texture resource.");
	}
	else
	{
		HRESULT_CHECK_FALLBACK(
			device->CreateCommittedResource(
				&DX12::CC::Desc::k_defaultHeapProperties,
				D3D12_HEAP_FLAG_NONE,
				specs._resourceDesc,
				specs._initialState,
				clearValue,
				IID_PPV_ARGS(&_resource)),,
			assert(false),
			"Could not create a committed texture resource.");
	}

	_srvHandle = DX12::GetSRVHeap()->Allocate();
	device->CreateShaderResourceView(_resource, specs._srvDesc, _srvHandle._cpu);
}

void DX12::Texture::Release()
{
	DX12::GetSRVHeap()->Free(_srvHandle);
	Utils::COM::SafeDeferredRenderThreadRelease(_resource);
}

DX12::Texture& DX12::Texture::MoveImpl(Texture&& other)
{
	CRITICAL_ASSERT(this != &other, "Moving Texture to itself.");

	_resource = other._resource;
	_srvHandle = std::move(other._srvHandle);

	other._resource = nullptr;

	return *this;
}