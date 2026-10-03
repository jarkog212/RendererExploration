#include "DX12Texture.h"
#include "DX12CommonDescriptors.h"
#include "DX12DescriptorHeap.h"
#include "DX12UploadRequest.h"

#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include "External/stb_image.h"

namespace
{
	typedef int STBI_Format;
	
	static std::filesystem::path k_textureFilePrefix(".\\Textures");

	std::unordered_map<std::wstring, std::weak_ptr<DX12::Texture>> g_textures;

	std::mutex g_textureMutex;

	inline int STBIChannelEnumToCount(::STBI_Format enumValue)
	{
		// enum values seem to match the count but just in case...
		switch (enumValue)
		{
			case STBI_grey:
				return 1;
			case STBI_grey_alpha:
				return 2;
			case STBI_rgb:
				return 3;
			case STBI_rgb_alpha:
				return 4;
		}

		return 0;
	}

	inline ::STBI_Format DXGIToSTBIFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
			case DXGI_FORMAT_R8_UNORM:
			case DXGI_FORMAT_A8_UNORM:
				return STBI_grey;
			case DXGI_FORMAT_R8G8_UNORM:
				return STBI_grey_alpha;
			case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
				return STBI_rgb_alpha;
	
		}

		CRITICAL_ASSERT(false, "Unexpected DXGI texture format %u", format);

		return 0;
	}

	const bool SafeReleaseTexture(const wchar_t* name)
	{
		std::lock_guard lock(::g_textureMutex);

		auto it = ::g_textures.find(name);
		if (it == ::g_textures.end())
			return false;

		CRITICAL_ASSERT(
			it->second.expired(),
			"Trying to remove texture with legitimate references. %ls",
			name);

		::g_textures.erase(it);
		return true;
	}
}

DX12::Texture::Texture(DX12::Texture::Specs& specs)
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
		_resource = std::exchange(specs._resource, nullptr);
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

DX12::Texture::Texture(DX12::Texture::LoadingRequest& request)
{
	CRITICAL_ASSERT(request._filename != L"", "Invalid filename for the texture loading.");
	CRITICAL_ASSERT(request._resourceDesc, "Cannot create texture without valid resource descriptor.");
	CRITICAL_ASSERT(request._srvDesc, "Cannot create texture without valid SRV descriptor.");

	std::filesystem::path path = ::k_textureFilePrefix;
	path.append(request._filename);

	::STBI_Format stbiFormat = ::DXGIToSTBIFormat(request._resourceDesc->Format);
	int width, height, channels;
	// so far we're limited to 8bits per channel always (other stbi_load versions support larger widths)
	unsigned char* imgBuffer = stbi_load(path.string().c_str(),	&width,	&height, &channels,	stbiFormat);
	CRITICAL_ASSERT(imgBuffer, "Invalid texture buffer fr %s", request._filename);

	const int imgBufferSize = width * height * ::STBIChannelEnumToCount(stbiFormat);
	request._resourceDesc->Height = height;
	request._resourceDesc->Width = width;
	
	_resource = DX12::UploadDataToNewGPUBuffer(imgBuffer, imgBufferSize, request._resourceDesc, false, request._initialState, request._resourceDesc->Flags);
	_srvHandle = DX12::GetSRVHeap()->Allocate();
	_name = request._filename;
	DX12::GetRendererDevice()->CreateShaderResourceView(_resource, request._srvDesc, _srvHandle._cpu);

	stbi_image_free(imgBuffer);
}

DX12::Texture::~Texture()
{
	DEBUG_DESTRUCTOR_BODY(Texture)

	if (_name.empty())
	{
		Utils::COM::SafeDeferredRenderThreadRelease(_resource);
		return;
	}
	
	// name means it has been read from a file and may be referenced else where
	::SafeReleaseTexture(_name.c_str());
	Utils::SafeDeferredRenderThreadReleaseViaLambda(
		[
			resource = _resource,
			handle = _srvHandle.MoveToRawHeap()
		]
		() mutable
		{
			Utils::COM::SafeRelease(resource);
			delete handle;
		});
}

DX12::Texture::Texture(Texture&& other) noexcept :
	_resource(std::exchange(other._resource, nullptr)),
	_srvHandle(std::move(other._srvHandle)),
	_name(std::move(other._name))
{
}

DX12::Texture& DX12::Texture::operator=(Texture&& other) noexcept
{
	CRITICAL_ASSERT(this != &other, "Moving Texture to itself.");

	std::swap(_resource, other._resource);
	std::swap(_srvHandle, other._srvHandle);
	std::swap(_name, other._name);

	return *this;
}

std::shared_ptr<DX12::Texture> DX12::TextureSpace::GetOrLoad(const wchar_t* name)
{
	std::scoped_lock lock(::g_textureMutex);

	if (auto texture = ::g_textures.find(name); texture != ::g_textures.end() && !texture->second.expired()) [[likely]]
		return texture->second.lock();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = DX12::CC::Desc::TextureRaw::k_basicSRVTemplate;
	D3D12_RESOURCE_DESC desc = DX12::CC::Desc::TextureRaw::k_basicTextureTemplate;
	DX12::Texture::LoadingRequest request;
	request._initialState |= D3D12_RESOURCE_STATE_COMMON;
	request._srvDesc = &srvDesc;
	request._resourceDesc = &desc;

	std::shared_ptr<DX12::Texture> out = std::make_shared<DX12::Texture>(request);
	::g_textures[name] = out;

	return out;
}

void DX12::TextureSpace::Release() 
{
	std::scoped_lock lock(::g_textureMutex);

	::g_textures.clear();
}
