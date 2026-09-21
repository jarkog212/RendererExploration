#include "DX12Texture.h"
#include "DX12CommonDescriptors.h"
#include "DX12DescriptorHeap.h"

#include <unordered_map>

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
		HRESULT_CHECK_CRITICAL(
			device->CreatePlacedResource(
				specs._heap,
				specs._allocationInfo.Offset,
				specs._resourceDesc,
				specs._initialState,
				clearValue,
				IID_PPV_ARGS(&_resource)),
			"Could not create a placed texture resource.");
	}
	else
	{
		HRESULT_CHECK_CRITICAL(
			device->CreateCommittedResource(
				&DX12::CC::Desc::Heap::k_baseTemplate,
				D3D12_HEAP_FLAG_NONE,
				specs._resourceDesc,
				specs._initialState,
				clearValue,
				IID_PPV_ARGS(&_resource)),
			"Could not create a committed texture resource.");
	}

	_srvHandle = DX12::GetSRVHeap()->Allocate();
	device->CreateShaderResourceView(_resource, specs._srvDesc, _srvHandle._cpu);
}

namespace
{
	std::unordered_map<const wchar_t*, std::weak_ptr<DX12::Texture>> g_textures;

	std::mutex g_mutex;
}

DX12::Texture::~Texture()
{
	DEBUG_DESTRUCTOR_BODY(Texture)

	Utils::COM::SafeDeferredRenderThreadRelease(_resource);
}

DX12::Texture::Texture(Texture&& other) noexcept :
	_resource(std::exchange(other._resource, nullptr)),
	_srvHandle(std::move(other._srvHandle))
{
}

DX12::Texture& DX12::Texture::operator=(Texture&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving Texture to itself.");

	std::swap(_resource, other._resource);
	std::swap(_srvHandle, other._srvHandle);

	return *this;
}

std::shared_ptr<DX12::Texture> DX12::TextureSpace::GetOrLoad(const wchar_t* name)
{
	std::scoped_lock lock(::g_mutex);

	auto texture = ::g_textures.find(name);
	if (texture != ::g_textures.end()) [[likely]]
		return texture->second.lock();

	// TODO textures load from file.
	std::shared_ptr<DX12::Texture> out = std::make_shared<DX12::Texture>();
	::g_textures.emplace(name, out);

	// NRVO/move should protect against deletion?
	return out;
}

void DX12::TextureSpace::Release() 
{
	std::scoped_lock lock(::g_mutex);

	::g_textures.clear();
}
